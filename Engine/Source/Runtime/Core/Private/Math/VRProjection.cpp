// CopyRight 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VRProjection.cpp: Implements the VR Projection related math functions.
=============================================================================*/

/* FBox2D structors
 *****************************************************************************/

#include "CorePrivatePCH.h"

#include "Math/VRProjection.h"


void FVRProjection::RoundSplitsToNearestPixel(
	const FIntRect*	OriginalViewport,
	const int		Width,
	const int		Height,
	float*			RefSplitsX,
	float*			RefSplitsY)
{
	for (int i = 0; i < Width - 1; ++i)
	{
		RefSplitsX[i] = FMath::RoundHalfToEven(RefSplitsX[i] * OriginalViewport->Width()) / float(OriginalViewport->Width());
	}
	for (int i = 0; i < Height - 1; ++i)
	{
		RefSplitsY[i] = FMath::RoundHalfToEven(RefSplitsY[i] * OriginalViewport->Height()) / float(OriginalViewport->Height());
	}
}

void FVRProjection::CalculateViewports(
	const FIntRect*	OriginalViewport,
	const float*	SplitsX,
	const float*	SplitsY,
	const float*	DensityScaleX,
	const float*	DensityScaleY,
	const int		Width,
	const int		Height,
	const int		Count,
	FloatRect*		RefViewports,
	FIntRect*		RefScissors,
	FIntRect*		RefBoundingRect)
{
	int ScissorMinX = OriginalViewport->Min.X;
	for (int i = 0; i < Width; ++i)
	{
		// Calculate the pixel Width of this column of Viewports, based on splits and density factor
		float LeftSplit = (i == 0) ? 0.0f : SplitsX[i - 1];
		float RightSplit = (i == Width - 1) ? 1.0f : SplitsX[i];
		int ScissorWidth = FMath::Max(1, int(FMath::RoundHalfToEven((RightSplit - LeftSplit) * DensityScaleX[i] * OriginalViewport->Width())));

		// Calculate corresponding Viewport position and size
		float ViewportWidth = float(ScissorWidth) / (RightSplit - LeftSplit);
		float ViewportMinX;
		if (i < Width - 1)
		{
			ViewportMinX = float(ScissorMinX) - ViewportWidth * LeftSplit;
		}
		else
		{
			// For the last Viewport in the row, calculate ViewportMinX slightly differently,
			// to prevent roundoff error from cutting off the last pixel.
			ViewportMinX = float(ScissorMinX + ScissorWidth) - ViewportWidth;
		}

		// Apply to all Viewports and Scissors in this column

		for (int j = i; j < Count; j += Width)
		{
			RefViewports[j].TopLeftX = ViewportMinX;
			RefViewports[j].Width = ViewportWidth;

			RefScissors[j].Min.X = ScissorMinX;
			RefScissors[j].Max.X = ScissorMinX + ScissorWidth;
		}

		ScissorMinX += ScissorWidth;
	}

	int ScissorMinY = OriginalViewport->Min.Y;
	for (int i = 0; i < Height; ++i)
	{
		// Calculate the pixel Width of this column of Viewports, based on splits and density factor
		float TopSplit = (i == 0) ? 0.0f : SplitsY[i - 1];
		float BottomSplit = (i == Height - 1) ? 1.0f : SplitsY[i];
		int ScissorHeight = FMath::Max(1, int(FMath::RoundHalfToEven((BottomSplit - TopSplit) * DensityScaleY[i] * OriginalViewport->Height())));

		// Calculate corresponding Viewport position and size
		float ViewportHeight = float(ScissorHeight) / (BottomSplit - TopSplit);
		float ViewportMinY;
		if (i < Height - 1)
		{
			ViewportMinY = float(ScissorMinY) - ViewportHeight * TopSplit;
		}
		else
		{
			// For the last Viewport in the column, calculate ViewportMinY slightly differently,
			// to prevent roundoff error from cutting off the last pixel.
			ViewportMinY = float(ScissorMinY + ScissorHeight) - ViewportHeight;
		}

		// Apply to all Viewports and Scissors in this row
		for (int j = i * Width; j < (i + 1) * Width; ++j)
		{
			RefViewports[j].TopLeftY = ViewportMinY;
			RefViewports[j].Height = ViewportHeight;

			RefScissors[j].Min.Y = ScissorMinY;
			RefScissors[j].Max.Y = ScissorMinY + ScissorHeight;
		}

		ScissorMinY += ScissorHeight;
	}

	// Set the bounding rect based on the accumulated Scissor widths and heights
	*RefBoundingRect = FIntRect(
		OriginalViewport->Min,
		FIntPoint(ScissorMinX, ScissorMinY));
}

void FVRProjection::CalculateFastGSCBData(
	const float*	SplitsX,
	const float*	SplitsY,
	const int		Width,
	const int		Height,
	FastGSCBData*	OutCBData)
{

	OutCBData->NDCSplitsX = FVector4();
	OutCBData->NDCSplitsY = FVector4();

	// Convert splits from UV (Y-down [0, 1]) to NDC (Y-up [-1, 1]) space, for GS culling usage
	for (int i = 0; i < Width - 1; ++i)
	{
		OutCBData->NDCSplitsX[i] = SplitsX[i] * 2.0f - 1.0f;
	}
	for (int i = 0; i < Height - 1; ++i)
	{
		OutCBData->NDCSplitsY[i] = SplitsY[i] * -2.0f + 1.0f;
	}
}


// Conservative Conf has splits at 25% in from each edge,
// and scales the outer Viewports to 70% of their original pixel density.
// Overall, reduces pixel Count by 28%.
CORE_API const FMultiRes::Configuration FMultiRes::Configuration_Rift_Conservative =
{
	0.63f, 0.62f,
	0.53f, 0.45f,
	{ 0.0f, 0.0f }, // SplitX are calculated by calling CalculateSplits()
	{ 0.0f, 0.0f },	// SplitY are calculated by calling CalculateSplits()
	{ 0.62f, 1.0f, 0.75f },
	{ 0.66f, 1.00f, 0.49f }
};

// Conservative Conf has splits at 25% in from each edge,
// and scales the outer Viewports to 70% of their original pixel density.
// Overall, reduces pixel Count by 28%.
CORE_API const FMultiRes::Configuration FMultiRes::Configuration_Rift_Aggressive =
{
	0.4f, 0.4f,
	0.57f, 0.41f,
	{ 0.0f, 0.0f }, // SplitX are calculated by calling CalculateSplits()
	{ 0.0f, 0.0f },	// SplitY are calculated by calling CalculateSplits()
	{ 0.64f, 1.0f, 0.64f },
	{ 0.64f, 1.0f, 0.64f }
};

// Conservative Conf has splits at 25% in from each edge,
// and scales the outer Viewports to 70% of their original pixel density.
// Overall, reduces pixel Count by 28%.
CORE_API const FMultiRes::Configuration FMultiRes::Configuration_Vive_Quality =
{
	0.55f, 0.52f,
	0.526f, 0.498f,
	{ 0.0f, 0.0f }, // SplitX are calculated by calling CalculateSplits()
	{ 0.0f, 0.0f },	// SplitY are calculated by calling CalculateSplits()
	{ 0.61f, 1.142f, 0.61f },
	{ 0.61f, 1.142f, 0.61f }
};

// Aggressive Conf has splits at 30% in from each edge,
// and scales the outer Viewports to 60% of their original pixel density.
// Overall, reduces pixel Count by 42%.
CORE_API const FMultiRes::Configuration FMultiRes::Configuration_Vive_Conservative =
{
	0.49f, 0.45f,
	0.526f, 0.498f,
	{ 0.0f, 0.0f }, // SplitX are calculated by calling CalculateSplits()
	{ 0.0f, 0.0f }, // SplitY are calculated by calling CalculateSplits()
	{ 0.6f, 1.0f, 0.6f },
	{ 0.6f, 1.0f, 0.6f }
};

// SuperAggressive Conf has splits at 30% in from each edge,
// and scales the outer Viewports to 40% of their original pixel density.
// Overall, reduces pixel Count by 60%.
CORE_API const FMultiRes::Configuration FMultiRes::Configuration_Vive_Aggressive =
{
	0.47f, 0.42f,
	0.526f, 0.498f,
	{ 0.0f, 0.0f }, // SplitX are calculated by calling CalculateSplits()
	{ 0.0f, 0.0f }, // SplitY are calculated by calling CalculateSplits()
	{ 0.54f, 1.0f, 0.54f },
	{ 0.54f, 1.0f, 0.54f }
};

// Merge two multires viewports into stereo multires viewports
FMultiRes::StereoViewports FMultiRes::Viewports::Merge(const FMultiRes::Viewports& InLeft, const FMultiRes::Viewports& InRight)
{
	FMultiRes::StereoViewports Out;

	Out.BoundingRect.Min.X = InLeft.BoundingRect.Min.X;
	Out.BoundingRect.Min.Y = InLeft.BoundingRect.Min.Y;
	Out.BoundingRect.Max.X = InRight.BoundingRect.Width() + InRight.BoundingRect.Min.X - InLeft.BoundingRect.Min.X;
	Out.BoundingRect.Max.Y = InRight.BoundingRect.Height() + InRight.BoundingRect.Min.Y - InLeft.BoundingRect.Min.Y;

	Out.Views[0] = InLeft.Views[0];		Out.Views[1] = InLeft.Views[1];		Out.Views[3] = InRight.Views[1];	Out.Views[4] = InRight.Views[2];
	Out.Views[5] = InLeft.Views[3];		Out.Views[6] = InLeft.Views[4];		Out.Views[8] = InRight.Views[4];	Out.Views[9] = InRight.Views[5];
	Out.Views[10] = InLeft.Views[6];	Out.Views[11] = InLeft.Views[7];	Out.Views[13] = InRight.Views[7];	Out.Views[14] = InRight.Views[8];

	Out.Views[3].TopLeftX -= Out.Views[3].Width; 	Out.Views[4].TopLeftX -= Out.Views[4].Width;
	Out.Views[8].TopLeftX -= Out.Views[8].Width; 	Out.Views[9].TopLeftX -= Out.Views[9].Width;
	Out.Views[13].TopLeftX -= Out.Views[13].Width;	Out.Views[14].TopLeftX -= Out.Views[14].Width;

	int32 MergedViewportSize = Out.BoundingRect.Max.X;
	Out.Views[0].Width *= MergedViewportSize / (float)InLeft.BoundingRect.Width();		Out.Views[1].Width *= MergedViewportSize / (float)InLeft.BoundingRect.Width();	
	Out.Views[5].Width *= MergedViewportSize / (float)InLeft.BoundingRect.Width();		Out.Views[6].Width *= MergedViewportSize / (float)InLeft.BoundingRect.Width();	
	Out.Views[10].Width *= MergedViewportSize / (float)InLeft.BoundingRect.Width();		Out.Views[11].Width *= MergedViewportSize / (float)InLeft.BoundingRect.Width();
	Out.Views[3].Width *= MergedViewportSize / (float)InRight.BoundingRect.Width();		Out.Views[4].Width *= MergedViewportSize / (float)InRight.BoundingRect.Width();
	Out.Views[8].Width *= MergedViewportSize / (float)InRight.BoundingRect.Width();		Out.Views[9].Width *= MergedViewportSize / (float)InRight.BoundingRect.Width();
	Out.Views[13].Width *= MergedViewportSize / (float)InRight.BoundingRect.Width();	Out.Views[14].Width *= MergedViewportSize / (float)InRight.BoundingRect.Width();

	Out.Scissors[0] = InLeft.Scissors[0];	Out.Scissors[1] = InLeft.Scissors[1];	Out.Scissors[3] = InRight.Scissors[1];	Out.Scissors[4] = InRight.Scissors[2];
	Out.Scissors[5] = InLeft.Scissors[3];	Out.Scissors[6] = InLeft.Scissors[4];	Out.Scissors[8] = InRight.Scissors[4];	Out.Scissors[9] = InRight.Scissors[5];
	Out.Scissors[10] = InLeft.Scissors[6];	Out.Scissors[11] = InLeft.Scissors[7];	Out.Scissors[13] = InRight.Scissors[7];	Out.Scissors[14] = InRight.Scissors[8];

	auto MergeViewport = [](const FloatRect& LeftView, const FIntRect& LeftScissor,
		const FloatRect& RightView, const FIntRect& RightScissor,
		FloatRect& MergedView, FIntRect& MergedScissor)
	{
		MergedView.TopLeftX = FMath::Min(LeftView.TopLeftX, RightView.TopLeftX);
		MergedView.TopLeftY = FMath::Min(LeftView.TopLeftY, RightView.TopLeftY);
		MergedView.Width = FMath::Max(LeftView.TopLeftX + LeftView.Width, RightView.TopLeftX + RightView.Width) - MergedView.TopLeftX;
		MergedView.Height = FMath::Max(LeftView.TopLeftY + LeftView.Height, RightView.TopLeftY + RightView.Height) - MergedView.TopLeftY;

		MergedScissor.Min.X = FMath::Min(LeftScissor.Min.X, RightScissor.Min.X);
		MergedScissor.Min.Y = FMath::Min(LeftScissor.Min.Y, RightScissor.Min.Y);
		MergedScissor.Max.X = FMath::Max(LeftScissor.Max.X, RightScissor.Max.X);
		MergedScissor.Max.Y = FMath::Max(LeftScissor.Max.Y, RightScissor.Max.Y);
	};

	MergeViewport(InLeft.Views[2], InLeft.Scissors[2], InRight.Views[0], InRight.Scissors[0], Out.Views[2], Out.Scissors[2]);
	MergeViewport(InLeft.Views[5], InLeft.Scissors[5], InRight.Views[3], InRight.Scissors[3], Out.Views[7], Out.Scissors[7]);
	MergeViewport(InLeft.Views[8], InLeft.Scissors[8], InRight.Views[6], InRight.Scissors[6], Out.Views[12], Out.Scissors[12]);

	return Out;
}

// Calculate the fraction of pixels a multi-res Conf will render,
// relative to ordinary non-multi-res rendering
float FMultiRes::CalculatePixelCountFraction(const FMultiRes::Configuration* Conf)
{
	float XFraction = 0.0f;
	for (int i = 0; i < Configuration::Width; ++i)
	{
		float LeftSplit = (i == 0) ? 0.0f : Conf->SplitsX[i - 1];
		float RightSplit = (i == Configuration::Width - 1) ? 1.0f : Conf->SplitsX[i];
		XFraction += (RightSplit - LeftSplit) * Conf->DensityScaleX[i];
	}

	float YFraction = 0.0f;
	for (int i = 0; i < Configuration::Height; ++i)
	{
		float TopSplit = (i == 0) ? 0.0f : Conf->SplitsY[i - 1];
		float BottomSplit = (i == Configuration::Height - 1) ? 1.0f : Conf->SplitsY[i];
		YFraction += (BottomSplit - TopSplit) * Conf->DensityScaleY[i];
	}

	return XFraction * YFraction;
}

// Calculate a Conf that's mirrored Left-to-Right; used for stereo rendering
void FMultiRes::CalculateMirroredConfig(const FMultiRes::Configuration* Conf, FMultiRes::Configuration* OutConfMirrored)
{
	OutConfMirrored->CenterWidth = Conf->CenterWidth;
	OutConfMirrored->CenterHeight = Conf->CenterHeight;

	// X dimension is mirrored Left-to-Right
	OutConfMirrored->CenterX = 1.0f - Conf->CenterX;
	for (int i = 0; i < Configuration::Width - 1; ++i)
	{
		OutConfMirrored->SplitsX[Configuration::Width - 2 - i] = 1.0f - Conf->SplitsX[i];
	}

	// Y dimension is just copied straight through
	OutConfMirrored->CenterY = Conf->CenterY;
	for (int i = 0; i < Configuration::Height - 1; ++i)
	{
		OutConfMirrored->SplitsY[i] = Conf->SplitsY[i];
	}

	for (int i = 0; i < Configuration::Width; ++i)
	{
		OutConfMirrored->DensityScaleX[Configuration::Width - 1 - i] = Conf->DensityScaleX[i];
	}
	for (int i = 0; i < Configuration::Height; ++i)
	{
		OutConfMirrored->DensityScaleY[i] = Conf->DensityScaleY[i];
	}
}

// computes 5x3 config that is mirrored horizontally
void FMultiRes::CalculateStereoConfig(const Configuration* Conf, const FIntRect* OriginalViewport, const int32 ViewportGap, StereoConfiguration* OutStereoConf)
{
	// Need to scale by the original Viewport Width divided by the final "virtual Width" where the gap is scaled by the inverse of the middle density
	float XScale = float(OriginalViewport->Max.X - OriginalViewport->Min.X) / (float(ViewportGap)/ Conf->DensityScaleX[2] + float(OriginalViewport->Max.X - OriginalViewport->Min.X) * 2.0f);

	// X dimension are mirrored
	OutStereoConf->CenterWidth[0] = Conf->CenterWidth * XScale;
	OutStereoConf->CenterWidth[1] = 1.0f - Conf->CenterWidth * XScale;
	OutStereoConf->CenterX[0] = Conf->CenterX * XScale;
	OutStereoConf->CenterX[1] = 1.0f - Conf->CenterX * XScale;
	for (int i = 0; i < Configuration::Width - 1; ++i)
	{
		OutStereoConf->SplitsX[i] = Conf->SplitsX[i] * XScale;
		OutStereoConf->SplitsX[StereoConfiguration::Width - i - 2] = 1.0f - Conf->SplitsX[i] * XScale;
	}

	OutStereoConf->CenterHeight = Conf->CenterHeight;
	OutStereoConf->CenterY = Conf->CenterY;
	for (int i = 0; i < Configuration::Height - 1; ++i)
	{
		OutStereoConf->SplitsY[i] = Conf->SplitsY[i];
	}

	for (int i = 0; i < Configuration::Width; ++i)
	{
		OutStereoConf->DensityScaleX[i] = Conf->DensityScaleX[i];
		OutStereoConf->DensityScaleX[StereoConfiguration::Width - i - 1] = Conf->DensityScaleX[i];
	}
	for (int i = 0; i < Configuration::Height; ++i)
	{
		OutStereoConf->DensityScaleY[i] = Conf->DensityScaleY[i];
	}

}

// Modify a Conf by rounding the split positions off to the nearest pixel
// (this ensures the center Viewport is exactly 1:1 with ordinary, non-multi-res rendering)
void FMultiRes::RoundSplitsToNearestPixel(const FIntRect* OriginalViewport, FMultiRes::Configuration* Conf)
{
	FVRProjection::RoundSplitsToNearestPixel(OriginalViewport, int(FMultiRes::Configuration::Width), int(FMultiRes::Configuration::Height), Conf->SplitsX, Conf->SplitsY);
}

// Calculate the viewport splits based on multi-res configuration
void FMultiRes::CalculateSplits(const FIntRect* OriginalViewport, Configuration* Conf)
{
	int CenterWidth = FMath::RoundToInt(FMath::Clamp(Conf->CenterWidth, 0.01f, 1.0f) * OriginalViewport->Width());
	int CenterHeight = FMath::RoundToInt(FMath::Clamp(Conf->CenterHeight, 0.01f, 1.0f) * OriginalViewport->Height());

	int HalfCenterWidth = CenterWidth / 2;
	int HalfCenterHeight = CenterHeight / 2;

	int MinLeft = HalfCenterWidth;
	int MaxRight = int(OriginalViewport->Width() - HalfCenterWidth);

	int MinTop = HalfCenterHeight;
	int MaxBottom = int(OriginalViewport->Height() - HalfCenterHeight);

	float FactorX = FMath::Clamp(Conf->CenterX, 0.0f, 1.0f);
	int CenterLocationX = FMath::Clamp(FMath::RoundToInt(FactorX * OriginalViewport->Width()), MinLeft, MaxRight);

	float FactorY = FMath::Clamp(Conf->CenterY, 0.0f, 1.0f);
	int CenterLocationY = FMath::Clamp(FMath::RoundToInt(FactorY * OriginalViewport->Height()), MinTop, MaxBottom);

	// Note that first split is min(value,1) and second split is max(value,size-1)
	// It's due to avoid incorrect viewport and scissor errors in the calling method
	int IntSplitsX[2];
	IntSplitsX[0] = FMath::Max(1, CenterLocationX - HalfCenterWidth);
	IntSplitsX[1] = FMath::Min(FMath::RoundToInt(OriginalViewport->Width()) - 1, IntSplitsX[0] + CenterWidth);

	int IntSplitsY[2];
	IntSplitsY[0] = FMath::Max(1, CenterLocationY - HalfCenterHeight);
	IntSplitsY[1] = FMath::Min(FMath::RoundToInt(OriginalViewport->Height()) - 1, IntSplitsY[0] + CenterHeight);

	float invTotalX = 1.0f / OriginalViewport->Width();
	float invTotalY = 1.0f / OriginalViewport->Height();

	Conf->SplitsX[0] = IntSplitsX[0] * invTotalX;
	Conf->SplitsX[1] = IntSplitsX[1] * invTotalX;

	Conf->SplitsY[0] = IntSplitsY[0] * invTotalY;
	Conf->SplitsY[1] = IntSplitsY[1] * invTotalY;
}

// Calculate the viewport splits based on multi-res configuration
void FMultiRes::CalculateStereoSplits(const FIntRect* OriginalViewport, StereoConfiguration* Conf)
{
	int CenterWidthL = FMath::RoundToInt(FMath::Clamp(Conf->CenterWidth[0], 0.01f, 1.0f) * OriginalViewport->Width());
	int CenterWidthR = FMath::RoundToInt(FMath::Clamp(Conf->CenterWidth[1], 0.01f, 1.0f) * OriginalViewport->Width());
	int CenterHeight = FMath::RoundToInt(FMath::Clamp(Conf->CenterHeight, 0.01f, 1.0f) * OriginalViewport->Height());

	int HalfCenterWidthL = CenterWidthL / 2;
	int HalfCenterWidthR = CenterWidthR / 2;
	int HalfCenterHeight = CenterHeight / 2;

	int MinLeftL = HalfCenterWidthL;
	int MinLeftR = HalfCenterWidthR;
	int MaxRightL = int(OriginalViewport->Width() - HalfCenterWidthL);
	int MaxRightR = int(OriginalViewport->Width() - HalfCenterWidthR);

	int MinTop = HalfCenterHeight;
	int MaxBottom = int(OriginalViewport->Height() - HalfCenterHeight);

	float FactorXL = FMath::Clamp(Conf->CenterX[0], 0.0f, 1.0f);
	float FactorXR = FMath::Clamp(Conf->CenterX[1], 0.0f, 1.0f);
	int CenterLocationXL = FMath::Clamp(FMath::RoundToInt(FactorXL * OriginalViewport->Width()), MinLeftL, MaxRightL);
	int CenterLocationXR = FMath::Clamp(FMath::RoundToInt(FactorXR * OriginalViewport->Width()), MinLeftR, MaxRightR);

	float FactorY = FMath::Clamp(Conf->CenterY, 0.0f, 1.0f);
	int CenterLocationY = FMath::Clamp(FMath::RoundToInt(FactorY * OriginalViewport->Height()), MinTop, MaxBottom);

	// Note that first split is min(value,1) and second split is max(value,size-1)
	// It's due to avoid incorrect viewport and scissor errors in the calling method

	int IntSplitsX[4];
	IntSplitsX[0] = FMath::Max(1, CenterLocationXL - HalfCenterWidthL);
	IntSplitsX[1] = FMath::Min(FMath::RoundToInt(OriginalViewport->Width()) - 1, IntSplitsX[0] + CenterWidthL);
	IntSplitsX[2] = FMath::Max(1, CenterLocationXR - HalfCenterWidthR);
	IntSplitsX[3] = FMath::Min(FMath::RoundToInt(OriginalViewport->Width()) - 1, IntSplitsX[2] + CenterWidthR);

	int IntSplitsY[2];
	IntSplitsY[0] = FMath::Max(1, CenterLocationY - HalfCenterHeight);
	IntSplitsY[1] = FMath::Min(FMath::RoundToInt(OriginalViewport->Height()) - 1, IntSplitsY[0] + CenterHeight);

	float InvTotalX = 1.0f / OriginalViewport->Width();
	float InvTotalY = 1.0f / OriginalViewport->Height();

	Conf->SplitsX[0] = IntSplitsX[0] * InvTotalX;
	Conf->SplitsX[1] = IntSplitsX[1] * InvTotalX;
	Conf->SplitsX[2] = IntSplitsX[2] * InvTotalX;
	Conf->SplitsX[3] = IntSplitsX[3] * InvTotalX;

	Conf->SplitsY[0] = IntSplitsY[0] * InvTotalY;
	Conf->SplitsY[1] = IntSplitsY[1] * InvTotalY;
}

// Calculate the actual Viewports and Scissor rects for a Conf
void FMultiRes::CalculateStereoViewports(const FIntRect* OriginalViewport, const FMultiRes::StereoConfiguration* Conf, FMultiRes::StereoViewports* OutViewports)
{
	FVRProjection::CalculateViewports(OriginalViewport, Conf->SplitsX, Conf->SplitsY, Conf->DensityScaleX, Conf->DensityScaleY, FMultiRes::StereoConfiguration::Width, FMultiRes::StereoConfiguration::Height, FMultiRes::StereoViewports::Count, OutViewports->Views, OutViewports->Scissors, &OutViewports->BoundingRect);
}

// Calculate the actual Viewports and Scissor rects for a Conf
void FMultiRes::CalculateViewports(const FIntRect* OriginalViewport, const FMultiRes::Configuration* Conf, FMultiRes::Viewports* OutViewports)
{
	FVRProjection::CalculateViewports(OriginalViewport, Conf->SplitsX, Conf->SplitsY, Conf->DensityScaleX, Conf->DensityScaleY, FMultiRes::Configuration::Width, FMultiRes::Configuration::Height, FMultiRes::Viewports::Count, OutViewports->Views, OutViewports->Scissors, &OutViewports->BoundingRect);
}



// Calculate FastGS constant buffer data for a Conf
void FMultiRes::CalculateFastGSCBData(const FMultiRes::Configuration* Conf, FastGSCBData* OutCBData)
{
	FVRProjection::CalculateFastGSCBData(Conf->SplitsX, Conf->SplitsY, FMultiRes::Configuration::Width, FMultiRes::Configuration::Height, OutCBData);
}

void FMultiRes::CalculateFastGSCBData(const FMultiRes::StereoConfiguration* Conf, FastGSCBData* OutCBData)
{
	FVRProjection::CalculateFastGSCBData(Conf->SplitsX, Conf->SplitsY, FMultiRes::StereoConfiguration::Width, FMultiRes::StereoConfiguration::Height, OutCBData);
}

// Calculate constant buffer data for a Conf
void FMultiRes::CalculateRemapCBData(const FMultiRes::Configuration* Conf, const FMultiRes::Viewports* Viewports, RemapCBData* OutCBData)
{
	for (int i = 0; i < Configuration::Width - 1; ++i)
	{
		OutCBData->LinearToVRProjectSplitsX[i] = Conf->SplitsX[i];
	}
	for (int i = 0; i < Configuration::Height - 1; ++i)
	{
		OutCBData->LinearToVRProjectSplitsY[i] = Conf->SplitsY[i];
	}

	// Calculate scale-biases for remapping from linear UV to multi-res render target's UV

	float DestMinU = 0.0f;
	for (int i = 0; i < Configuration::Width; ++i)
	{
		float LeftSplit = (i == 0) ? 0.0f : Conf->SplitsX[i - 1];
		float RightSplit = (i == Configuration::Width - 1) ? 1.0f : Conf->SplitsX[i];
		float DestWidth = float(Viewports->Scissors[i].Width()) / float(Viewports->BoundingRect.Width());

		float Scale = DestWidth / (RightSplit - LeftSplit);
		float Bias = -LeftSplit * Scale + DestMinU;
		OutCBData->LinearToVRProjectX[i] = ScaleBias{ Scale, Bias };

		DestMinU += DestWidth;
	}

	float DestMinV = 0.0f;
	for (int i = 0; i < Configuration::Height; ++i)
	{
		float TopSplit = (i == 0) ? 0.0f : Conf->SplitsY[i - 1];
		float BottomSplit = (i == Configuration::Height - 1) ? 1.0f : Conf->SplitsY[i];
		float DestHeight = float(Viewports->Scissors[i*Configuration::Width].Height()) / float(Viewports->BoundingRect.Height());

		float Scale = DestHeight / (BottomSplit - TopSplit);
		float Bias = -TopSplit * Scale + DestMinV;
		OutCBData->LinearToVRProjectY[i] = ScaleBias{ Scale, Bias };

		DestMinV += DestHeight;
	}

	// Calculate splits and scale-biases for remapping fom multi-res to linear (the inverse of the above).

	for (int i = 0; i < Configuration::Width - 1; ++i)
	{
		OutCBData->VRProjectToLinearSplitsX[i] = Conf->SplitsX[i] * OutCBData->LinearToVRProjectX[i].Scale + OutCBData->LinearToVRProjectX[i].Bias;
	}
	for (int i = 0; i < Configuration::Height - 1; ++i)
	{
		OutCBData->VRProjectToLinearSplitsY[i] = Conf->SplitsY[i] * OutCBData->LinearToVRProjectY[i].Scale + OutCBData->LinearToVRProjectY[i].Bias;
	}

	for (int i = 0; i < Configuration::Width; ++i)
	{
		float InverseScale = 1.0f / OutCBData->LinearToVRProjectX[i].Scale;
		OutCBData->VRProjectToLinearX[i] = ScaleBias{ InverseScale, -OutCBData->LinearToVRProjectX[i].Bias * InverseScale };
	}
	for (int i = 0; i < Configuration::Height; ++i)
	{
		float InverseScale = 1.0f / OutCBData->LinearToVRProjectY[i].Scale;
		OutCBData->VRProjectToLinearY[i] = ScaleBias{ InverseScale, -OutCBData->LinearToVRProjectY[i].Bias * InverseScale };
	}
}



// Remap UVs from linear to multi-res
FVector2D FMultiRes::MapLinearToMultiRes(const RemapCBData* CBData, const FVector2D& UV)
{
	// Scale-bias U and V based on which Viewport they're in
	FVector2D Result;

	if (UV.X < CBData->LinearToVRProjectSplitsX[0])
		Result.X = UV.X * CBData->LinearToVRProjectX[0].Scale + CBData->LinearToVRProjectX[0].Bias;
	else if (UV.X < CBData->LinearToVRProjectSplitsX[1])
		Result.X = UV.X * CBData->LinearToVRProjectX[1].Scale + CBData->LinearToVRProjectX[1].Bias;
	else
		Result.X = UV.X * CBData->LinearToVRProjectX[2].Scale + CBData->LinearToVRProjectX[2].Bias;

	if (UV.Y < CBData->LinearToVRProjectSplitsY[0])
		Result.Y = UV.Y * CBData->LinearToVRProjectY[0].Scale + CBData->LinearToVRProjectY[0].Bias;
	else if (UV.Y < CBData->LinearToVRProjectSplitsY[1])
		Result.Y = UV.Y * CBData->LinearToVRProjectY[1].Scale + CBData->LinearToVRProjectY[1].Bias;
	else
		Result.Y = UV.Y * CBData->LinearToVRProjectY[2].Scale + CBData->LinearToVRProjectY[2].Bias;

	return Result;
}

// Remap UVs from multi-res to linear
FVector2D FMultiRes::MapMultiResToLinear(const RemapCBData* CBData, const FVector2D& UV)
{
	// Scale-bias U and V based on which Viewport they're in
	FVector2D Result;

	if (UV.X < CBData->VRProjectToLinearSplitsX[0])
		Result.X = UV.X * CBData->VRProjectToLinearX[0].Scale + CBData->VRProjectToLinearX[0].Bias;
	else if (UV.X < CBData->VRProjectToLinearSplitsX[1])
		Result.X = UV.X * CBData->VRProjectToLinearX[1].Scale + CBData->VRProjectToLinearX[1].Bias;
	else
		Result.X = UV.X * CBData->VRProjectToLinearX[2].Scale + CBData->VRProjectToLinearX[2].Bias;

	if (UV.Y < CBData->VRProjectToLinearSplitsY[0])
		Result.Y = UV.Y * CBData->VRProjectToLinearY[0].Scale + CBData->VRProjectToLinearY[0].Bias;
	else if (UV.Y < CBData->VRProjectToLinearSplitsY[1])
		Result.Y = UV.Y * CBData->VRProjectToLinearY[1].Scale + CBData->VRProjectToLinearY[1].Bias;
	else
		Result.Y = UV.Y * CBData->VRProjectToLinearY[2].Scale + CBData->VRProjectToLinearY[2].Bias;

	return Result;
}


CORE_API const FLensMatchedShading::Configuration FLensMatchedShading::Configuration_Rift_Conservative =
{
	0.471f, 0.471f,
	0.471f, 0.471f,

	0.414f, 0.553f, // relative to CV1 render target size for one eye, 1332x1586
	0.534f, 0.368f
};

CORE_API const FLensMatchedShading::Configuration FLensMatchedShading::Configuration_Rift_Aggressive =
{
	0.9f, 0.9f,
	0.9f, 0.9f,

	0.361f, 0.481f, // relative to CV1 render target size for one eye, 1332x1586
	0.465f, 0.321f
};

CORE_API const FLensMatchedShading::Configuration FLensMatchedShading::Configuration_Vive_Quality =
{
	0.61f, 0.41f,
	0.59f, 0.62f,

	0.444f, 0.504f, // Relative to HTC recommended render target size for one eye, 1512 x 1680
	0.442f, 0.438f
};

CORE_API const FLensMatchedShading::Configuration FLensMatchedShading::Configuration_Vive_Conservative =
{
	0.9f, 0.67f,
	0.88f, 0.91f,

	0.40f, 0.451f, // Relative to HTC recommended render target size for one eye, 1512 x 1680
	0.395f, 0.391f
};

CORE_API const FLensMatchedShading::Configuration FLensMatchedShading::Configuration_Vive_Aggressive =
{
	1.0f, 0.76f,
	0.98f, 1.01f,

	0.347f, 0.394f, // Relative to HTC recommended render target size for one eye, 1512 x 1680
	0.345f, 0.341f
};

// Merge two lens matched shading viewports into stereo lens matched shading viewports
FLensMatchedShading::StereoViewports FLensMatchedShading::Viewports::Merge(const FLensMatchedShading::Viewports& InLeft, const FLensMatchedShading::Viewports& InRight)
{
	FLensMatchedShading::StereoViewports Out;

	FMemory::Memcpy(Out.Views, InLeft.Views, sizeof(InLeft.Views));
	FMemory::Memcpy(Out.Views + FLensMatchedShading::Viewports::Count, InRight.Views, sizeof(InRight.Views));
	FMemory::Memcpy(Out.Scissors, InLeft.Scissors, sizeof(InLeft.Scissors));
	FMemory::Memcpy(Out.Scissors + FLensMatchedShading::Viewports::Count, InRight.Scissors, sizeof(InRight.Scissors));

	Out.BoundingRect.Min.X = InLeft.BoundingRect.Min.X;
	Out.BoundingRect.Min.Y = InLeft.BoundingRect.Min.Y;
	Out.BoundingRect.Max.X = InRight.BoundingRect.Width() + InRight.BoundingRect.Min.X - InLeft.BoundingRect.Min.X;
	Out.BoundingRect.Max.Y = InRight.BoundingRect.Height() + InRight.BoundingRect.Min.Y - InLeft.BoundingRect.Min.Y;

	return Out;
}

void FLensMatchedShading::CalculateMirroredConfig(
	const FLensMatchedShading::Configuration* Conf,
	FLensMatchedShading::Configuration*	RefConfMirrored)
{
	*RefConfMirrored = *Conf;
	RefConfMirrored->WarpLeft = Conf->WarpRight;
	RefConfMirrored->WarpRight = Conf->WarpLeft;
	RefConfMirrored->RelativeSizeLeft = Conf->RelativeSizeRight;
	RefConfMirrored->RelativeSizeRight = Conf->RelativeSizeLeft;
}

void FLensMatchedShading::CalculateStereoConfig(const Configuration* Conf, const FIntRect* OriginalViewport, const int32 ViewportGap, StereoConfiguration* OutStereoConf)
{
	OutStereoConf->LeftConfig = *Conf;
	CalculateMirroredConfig(Conf, &OutStereoConf->RightConfig);
}

void FLensMatchedShading::RoundSplitsToNearestPixel(
	const FIntRect* OriginalViewport,
	FLensMatchedShading::Configuration*	RefConf)
{
	// No op.
}

void FLensMatchedShading::CalculateViewports(
	const FIntRect* OriginalViewport,
	const FLensMatchedShading::Configuration* Conf,
	FLensMatchedShading::Viewports* RefViewports)
{
	float SizeLeft = Conf->RelativeSizeLeft * OriginalViewport->Width();
	float SizeRight = Conf->RelativeSizeRight * OriginalViewport->Width();
	float SizeUp = Conf->RelativeSizeUp * OriginalViewport->Height();
	float SizeDown = Conf->RelativeSizeDown * OriginalViewport->Height();

	float WidthScale = (SizeLeft + SizeRight) / (float)OriginalViewport->Width();
	float HeightScale = (SizeUp + SizeDown) / (float)OriginalViewport->Height();

	FIntRect RealRTSize = *OriginalViewport;
	RealRTSize.Min.X = FMath::FloorToInt(float(RealRTSize.Min.X) * WidthScale);
	RealRTSize.Max.X = FMath::CeilToInt(float(RealRTSize.Max.X) * WidthScale);
	RealRTSize.Min.Y = FMath::FloorToInt(float(RealRTSize.Min.Y) * HeightScale);
	RealRTSize.Max.Y = FMath::CeilToInt(float(RealRTSize.Max.Y) * HeightScale);

	// this calculation can introduce off-by-one errors because of floating point math + floor/ceil, so correct for that
	RealRTSize.Max -= RealRTSize.Size() - FIntPoint(round(SizeLeft + SizeRight), round(SizeUp + SizeDown));

	FVector2D Center;
	Center.X = FMath::RoundHalfToEven(RealRTSize.Min.X + RealRTSize.Width() * SizeLeft / (SizeLeft + SizeRight));
	Center.Y = FMath::RoundHalfToEven(RealRTSize.Min.Y + RealRTSize.Height() * SizeUp / (SizeUp + SizeDown));

	float ViewportLeft = SizeLeft * (1.0f + Conf->WarpLeft);
	float ViewportRight = SizeRight * (1.0f + Conf->WarpRight);
	float ViewportUp = SizeUp * (1.0f + Conf->WarpUp);
	float ViewportDown = SizeDown * (1.0f + Conf->WarpDown);

	RefViewports->Views[0] = FloatRect{ Center.X - ViewportLeft, Center.Y - ViewportUp, ViewportLeft * 2, ViewportUp * 2 };
	RefViewports->Views[1] = FloatRect{ Center.X - ViewportRight, Center.Y - ViewportUp, ViewportRight * 2, ViewportUp * 2 };
	RefViewports->Views[2] = FloatRect{ Center.X - ViewportLeft, Center.Y - ViewportDown, ViewportLeft * 2, ViewportDown * 2 };
	RefViewports->Views[3] = FloatRect{ Center.X - ViewportRight, Center.Y - ViewportDown, ViewportRight * 2, ViewportDown * 2 };

	auto GetIntRect = [RealRTSize](float left, float top, float right, float bottom)
	{
		int iLeft = FMath::Max(RealRTSize.Min.X, int(FMath::FloorToInt(left)));
		int iTop = FMath::Max(RealRTSize.Min.Y, int(FMath::FloorToInt(top)));
		int iRight = FMath::Min(RealRTSize.Max.X, int(FMath::CeilToInt(right)));
		int iBottom = FMath::Min(RealRTSize.Max.Y, int(FMath::CeilToInt(bottom)));
		return FIntRect(iLeft, iTop, iRight, iBottom);
	};

	RefViewports->Scissors[0] = GetIntRect(Center.X - SizeLeft, Center.Y - SizeUp, Center.X, Center.Y);
	RefViewports->Scissors[1] = GetIntRect(Center.X, Center.Y - SizeUp, Center.X + SizeRight, Center.Y);
	RefViewports->Scissors[2] = GetIntRect(Center.X - SizeLeft, Center.Y, Center.X, Center.Y + SizeDown);
	RefViewports->Scissors[3] = GetIntRect(Center.X, Center.Y, Center.X + SizeRight, Center.Y + SizeDown);

	RefViewports->BoundingRect = GetIntRect(
		Center.X - SizeLeft,
		Center.Y - SizeUp,
		Center.X + SizeRight,
		Center.Y + SizeDown);
}

void FLensMatchedShading::CalculateStereoViewports(
	const FIntRect* OriginalViewport,
	const FLensMatchedShading::StereoConfiguration*	Conf,
	const int32 ViewportGap,
	FLensMatchedShading::StereoViewports* RefViewports)
{
	uint32 Width = OriginalViewport->Width();
	uint32 ScaledWidth = uint32(FMath::RoundHalfToEven((Width - ViewportGap) * 0.5f));

	FIntRect ViewLeft = FIntRect(
		OriginalViewport->Min.X,
		OriginalViewport->Min.Y,
		OriginalViewport->Min.X + ScaledWidth,
		OriginalViewport->Max.Y);

	FIntRect ViewRight = FIntRect(
		Width - ScaledWidth,
		OriginalViewport->Min.Y,
		Width,
		OriginalViewport->Max.Y);

	Viewports ViewportLeft;
	CalculateViewports(&ViewLeft, &Conf->LeftConfig, &ViewportLeft);
	Viewports ViewportRight;
	CalculateViewports(&ViewRight, &Conf->RightConfig, &ViewportRight);

	*RefViewports = Viewports::Merge(ViewportLeft, ViewportRight);
}

void FLensMatchedShading::CalculateFastGSCBData(
	const FLensMatchedShading::Configuration* Conf,
	FastGSCBData* RefCBData)
{
	float Split = 0.5f;
	FVRProjection::CalculateFastGSCBData(&Split, &Split, Configuration::Width, Configuration::Height, RefCBData);
}

void FLensMatchedShading::CalculateFastGSCBData(
	const FLensMatchedShading::StereoConfiguration*	Conf,
	FastGSCBData* RefCBData)
{
	float Split = 0.5f;
	FVRProjection::CalculateFastGSCBData(&Split, &Split, Configuration::Width, Configuration::Height, RefCBData);
}

void FLensMatchedShading::CalculateRemapCBData(
	const FLensMatchedShading::Configuration* Conf,
	const FLensMatchedShading::Viewports* Viewports,
	RemapCBData*		RefCBData)
{
	// Clip to window

	RefCBData->LinearToVRProjectSplitsX[0] = Conf->WarpLeft;
	RefCBData->LinearToVRProjectSplitsX[1] = Conf->WarpRight;
	RefCBData->LinearToVRProjectSplitsY[0] = Conf->WarpUp;
	RefCBData->LinearToVRProjectSplitsY[1] = Conf->WarpDown;

	for (int i = 0; i < 2; ++i)
	{
		float Scale = Viewports->Views[i].Width * 0.5f;
		float Bias = Viewports->Views[i].TopLeftX + Scale;
		RefCBData->LinearToVRProjectX[i] = ScaleBias{ Scale, Bias };
	}

	for (int i = 0; i < 2; ++i)
	{
		int j = 1 - i;
		float Scale = -Viewports->Views[j * 2].Height * 0.5f;
		float Bias = Viewports->Views[j * 2].TopLeftY - Scale;
		RefCBData->LinearToVRProjectY[i] = ScaleBias{ Scale, Bias };
	}

	// Window to clip

	RefCBData->VRProjectToLinearSplitsX[0] = float(Viewports->Scissors[1].Min.X);
	RefCBData->VRProjectToLinearSplitsX[1] = 0.0f;
	RefCBData->VRProjectToLinearSplitsY[0] = float(Viewports->Scissors[2].Min.Y);
	RefCBData->VRProjectToLinearSplitsY[1] = 0.0f;

	for (int i = 0; i < 2; ++i)
	{
		float Scale = 2.0f / Viewports->Views[i].Width;
		float Bias = -Viewports->Views[i].TopLeftX * Scale - 1.0f;
		RefCBData->VRProjectToLinearX[i] = ScaleBias{ Scale, Bias };
	}

	for (int i = 0; i < 2; ++i)
	{
		int j = i * 2;
		float Scale = -2.0f / Viewports->Views[j].Height;
		float Bias = -Viewports->Views[j].TopLeftY * Scale + 1.0f;
		RefCBData->VRProjectToLinearY[i] = ScaleBias{ Scale, Bias };
	}

	// Bounding rect
	RefCBData->BoundingRectOrigin.X = float(Viewports->BoundingRect.Min.X);
	RefCBData->BoundingRectOrigin.Y = float(Viewports->BoundingRect.Min.Y);
	RefCBData->BoundingRectSize.X = float(Viewports->BoundingRect.Width());
	RefCBData->BoundingRectSize.Y = float(Viewports->BoundingRect.Height());
	RefCBData->BoundingRectSizeInv.X = 1.0f / float(Viewports->BoundingRect.Width());
	RefCBData->BoundingRectSizeInv.Y = 1.0f / float(Viewports->BoundingRect.Height());
}

bool IsFastGSNeeded()
{
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.MultiRes"));
	static const bool bMultiResShaders = CVar->GetValueOnAnyThread() != 0;
	static const auto CVarSPS = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.SinglePassStereo"));
	static const bool bSPSShaders = CVarSPS->GetValueOnAnyThread() != 0;
	static const auto CVarLMS = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.LensMatchedShading"));
	static const bool bLMSShaders = CVarLMS->GetValueOnAnyThread() != 0;
	const bool bFastGS = (bMultiResShaders || bSPSShaders || bLMSShaders);
	return bFastGS;
}