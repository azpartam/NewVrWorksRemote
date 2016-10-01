// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EmptyRenderTarget.cpp: Empty render target implementation.
=============================================================================*/

#include "EmptyRHIPrivate.h"
#include "ScreenRendering.h"


void FEmptyDynamicRHI::RHICopyToResolveTarget(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, bool bKeepOriginalSurface, const FResolveParams& ResolveParams)
{

}

void FEmptyDynamicRHI::RHICopyResourceToGPU(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, uint32 DestGPUIndex, uint32 SrcGPUIndex, const FResolveParams& ResolveParams)
{

}

void FEmptyDynamicRHI::RHIReadSurfaceData(FTextureRHIParamRef TextureRHI, FIntRect Rect, TArray<FColor>& OutData, FReadSurfaceDataFlags InFlags)
{

}

void FEmptyDynamicRHI::RHIMapStagingSurface(FTextureRHIParamRef TextureRHI,void*& OutData,int32& OutWidth,int32& OutHeight)
{

}

void FEmptyDynamicRHI::RHIUnmapStagingSurface(FTextureRHIParamRef TextureRHI)
{

}

void FEmptyDynamicRHI::RHIReadSurfaceFloatData(FTextureRHIParamRef TextureRHI, FIntRect Rect, TArray<FFloat16Color>& OutData, ECubeFace CubeFace,int32 ArrayIndex,int32 MipIndex)
{

}

void FEmptyDynamicRHI::RHIRead3DSurfaceFloatData(FTextureRHIParamRef TextureRHI,FIntRect InRect,FIntPoint ZMinMax,TArray<FFloat16Color>& OutData)
{

}
