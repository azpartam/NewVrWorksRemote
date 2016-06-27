// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessDownsample.cpp: Post processing down sample implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessMultiResWarp.h"
#include "PostProcessing.h"
#include "PostProcessWeightedSampleSum.h"
#include "SceneUtils.h"

/** Encapsulates a simple copy pixel shader. */
class FPostProcessMultiResWarpPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessMultiResWarpPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	/** Default constructor. */
	FPostProcessMultiResWarpPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;

	/** Initialization constructor. */
	FPostProcessMultiResWarpPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(Context.RHICmdList, ShaderRHI, Context.View);

		PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
	}
};

IMPLEMENT_SHADER_TYPE(, FPostProcessMultiResWarpPS, TEXT("PostProcessMultiResWarp"), TEXT("MainPS"), SF_Pixel);

FRCPassPostProcessMultiResWarp::FRCPassPostProcessMultiResWarp(FIntPoint InScaledRectSize, FIntPoint InUnscaledRectSize, const TCHAR *InDebugName)
	: ScaledRectSize(InScaledRectSize)
	, UnscaledRectSize(InUnscaledRectSize)
	, DebugName(InDebugName)
{
}

void FRCPassPostProcessMultiResWarp::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(Context.RHICmdList, PassThrough);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if (!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;

	check(View.bVRProjectEnabled);

	// we assume the input and output is full resolution
	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	FIntPoint InputScaleFactor = SceneContext.GetBufferSizeXY() / SrcSize;
	FIntPoint OutputScaleFactor = SceneContext.GetLinearBufferSizeXY() / DestSize;

	FIntRect SrcRect = View.ViewRect / InputScaleFactor;
	FIntRect DestRect = View.NonVRProjectViewRect / OutputScaleFactor;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	SetRenderTarget(Context.RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef());
	Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f);

	// set the state
	Context.RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

	Context.RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	Context.RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

	Context.RHICmdList.Clear(true, FLinearColor(0, 0, 0, 0), false, 1.0f, false, 0, DestRect);

	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	TShaderMapRef<FPostProcessMultiResWarpPS> PixelShader(Context.GetShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(Context.RHICmdList, Context.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	VertexShader->SetParameters(Context);
	PixelShader->SetParameters(Context);

	DrawPostProcessPass(
		Context.RHICmdList,
		DestRect.Min.X, DestRect.Min.Y,
		DestRect.Width(), DestRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y,
		SrcRect.Width(), SrcRect.Height(),
		DestSize,
		SrcSize,
		*VertexShader,
		View.StereoPass,
		Context.HasHmdMesh(),
		EDRF_UseTriangleOptimization);

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessMultiResWarp::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret;

	
	Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();

	// need to recompute desired size
	FIntPoint ScaleFactor = ScaledRectSize / Ret.Extent;
	Ret.Extent = UnscaledRectSize / ScaleFactor;
	Ret.DebugName = DebugName;

	return Ret;
}
