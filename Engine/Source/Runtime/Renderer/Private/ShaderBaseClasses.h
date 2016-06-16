// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderBaseClasses.h: Shader base classes
=============================================================================*/

#pragma once

#include "MeshMaterialShader.h"
#include "ShaderParameterUtils.h"

/** The uniform shader parameters associated with a LOD fade. */
// This was moved out of ScenePrivate.h to workaround MSVC vs clang template issue (it's used in this header file, so needs to be declared earlier)
// Z is the dither fade value (-1 = just fading in, 0 no fade, 1 = just faded out)
// W is unused and zero
BEGIN_UNIFORM_BUFFER_STRUCT(FDistanceCullFadeUniformShaderParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX(FVector2D,FadeTimeScaleBias, EShaderPrecisionModifier::Half)
END_UNIFORM_BUFFER_STRUCT(FDistanceCullFadeUniformShaderParameters)

typedef TUniformBufferRef< FDistanceCullFadeUniformShaderParameters > FDistanceCullFadeUniformBufferRef;

/** Base Hull shader for drawing policy rendering */
class FBaseHS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FBaseHS,MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		if (!RHISupportsTessellation(Platform))
		{
			return false;
		}

		if (VertexFactoryType && !VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;	
		}

		if (!Material || Material->GetTessellationMode() == MTM_NoTessellation) 
		{
			// Material controls use of tessellation
			return false;	
		}

		return true;
	}

	FBaseHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
	}

	FBaseHS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, (FHullShaderRHIParamRef)GetHullShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(View.GetFeatureLevel()), View, ESceneRenderTargetsMode::SetTextures);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement, const FMeshDrawingRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, (FHullShaderRHIParamRef)GetHullShader(),VertexFactory,View,Proxy,BatchElement,DrawRenderState);
	}
};

/** Base Domain shader for drawing policy rendering */
class FBaseDS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FBaseDS,MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		if (!RHISupportsTessellation(Platform))
		{
			return false;
		}

		if (VertexFactoryType && !VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;	
		}

		if (!Material || Material->GetTessellationMode() == MTM_NoTessellation) 
		{
			// Material controls use of tessellation
			return false;	
		}

		return true;		
	}

	FBaseDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		NeedsInstancedStereoBiasParameter.Bind(Initializer.ParameterMap, TEXT("bNeedsInstancedStereoBias"));
		IsSinglePassStereoParameter.Bind(Initializer.ParameterMap, TEXT("bIsSinglePassStereo"));
	}

	FBaseDS() {}

	virtual bool Serialize(FArchive& Ar) override
	{
		const bool result = FMeshMaterialShader::Serialize(Ar);
		Ar << NeedsInstancedStereoBiasParameter;
		Ar << IsSinglePassStereoParameter;
		return result;
	}

	void SetParameters(FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View,
													const bool bNeedsInstancedStereoBias = false, const bool bIsSinglePassStereo = false)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, (FDomainShaderRHIParamRef)GetDomainShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(View.GetFeatureLevel()), View, ESceneRenderTargetsMode::SetTextures);

		if (NeedsInstancedStereoBiasParameter.IsBound())
		{
			SetShaderValue(RHICmdList, (FDomainShaderRHIParamRef)GetDomainShader(), NeedsInstancedStereoBiasParameter, bNeedsInstancedStereoBias);
		}

		if (IsSinglePassStereoParameter.IsBound())
		{
			SetShaderValue(RHICmdList, (FDomainShaderRHIParamRef)GetDomainShader(), IsSinglePassStereoParameter, bIsSinglePassStereo);
		}
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement, const FMeshDrawingRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, (FDomainShaderRHIParamRef)GetDomainShader(),VertexFactory,View,Proxy,BatchElement,DrawRenderState);
	}

private:
	FShaderParameter NeedsInstancedStereoBiasParameter;
	FShaderParameter IsSinglePassStereoParameter;
};

