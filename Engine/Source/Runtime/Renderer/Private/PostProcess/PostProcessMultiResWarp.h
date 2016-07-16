// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessMultiResWarp.h: Post processing down sample implementation.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"
#include "PostProcessing.h"

// ePId_Input0: Input image
// derives from TRenderingCompositePassBase<InputCount, OutputCount> 
class FRCPassPostProcessVRProjectWarp : public TRenderingCompositePassBase<1, 1>
{
public:
	// constructor
	FRCPassPostProcessVRProjectWarp( FIntPoint InScaledRectSize, FIntPoint InUnscaledRectSize, const TCHAR *InDebugName = TEXT("MultiResWarp"));

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context) override;
	virtual void Release() override { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const override;

private:
	FIntPoint ScaledRectSize;
	FIntPoint UnscaledRectSize;
	const TCHAR *DebugName;
};

