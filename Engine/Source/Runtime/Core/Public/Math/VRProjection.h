// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


struct FVRProjection
{
	// Constant buffer data to supply the FastGS for culling primitives per-viewport
	// Note: needs to be 16-byte-aligned when placed in a constant buffer.
	struct FastGSCBData
	{
		// Split positions in NDC space (Y-up, [-1,1] range)
		FVector4 NDCSplitsX;
		FVector4 NDCSplitsY;
	};

	struct ScaleBias
	{
		float Scale;
		float Bias;

		operator FVector2D() { return FVector2D(Scale, Bias); }
	};

	// Constant buffer data to supply the UV-remapping helper functions
	// Note: needs to be 16-byte-aligned when placed in a constant buffer.
	struct RemapCBData
	{
		FVector2D	LinearToVRProjectSplitsX;
		FVector2D	LinearToVRProjectSplitsY;
		ScaleBias	LinearToVRProjectX[3];
		ScaleBias	LinearToVRProjectY[3];

		FVector2D	VRProjectToLinearSplitsX;
		FVector2D	VRProjectToLinearSplitsY;
		ScaleBias	VRProjectToLinearX[3];
		ScaleBias	VRProjectToLinearY[3];

		FVector2D	BoundingRectOrigin;
		FVector2D	BoundingRectSize;
		FVector2D	BoundingRectSizeInv;
	};

	struct FloatRect
	{
		float TopLeftX;
		float TopLeftY;
		float Width;
		float Height;
	};

	static CORE_API void RoundSplitsToNearestPixel(
		const FIntRect*	OriginalViewport,
		const int		Width,
		const int		Height,
		float*			RefSplitsX,
		float*			RefSplitsY);

	static CORE_API void CalculateViewports(
		const FIntRect*	OriginalViewport,
		const float*		SplitsX,
		const float*		SplitsY,
		const float*		DensityScaleX,
		const float*		DensityScaleY,
		const int		Width,
		const int		Height,
		const int		Count,
		FloatRect*		RefViewports,
		FIntRect*		RefScissors,
		FIntRect*		RefBoundingRect);

	static CORE_API void CalculateFastGSCBData(
		const float*		SplitsX,
		const float*		SplitsY,
		const int		Width,
		const int		Height,
		FastGSCBData*	OutCBData);
};

struct FMultiRes : public FVRProjection
{

	// Struct to define a multi-res viewport Config
	struct Configuration
	{
		// Currently hardcoded for 3x3 viewports; later to be generalized
		enum { Width = 3, Height = 3 };

		// Size of the central viewport, ranging 0.01..1, where 1 is full original viewport size
		float CenterWidth;
		float CenterHeight;

		// Location of the central viewport, ranging 0..1, where 0.5 is the center of the screen
		float CenterX;
		float CenterY;

		// Split positions: where the splits between multi-res viewports occur, as a fractional
		// position within the full viewport (0 to 1), measuring from the top left
		float SplitsX[Width - 1];
		float SplitsY[Height - 1];

		// Pixel density scale factors: how much the linear pixel density is scaled within each
		// row and column (1.0 = full density)
		float DensityScaleX[Width];
		float DensityScaleY[Height];
	};

	// Struct to define a multi-res viewport Config when mirroring in one viewport
	struct StereoConfiguration
	{
		// Currently hardcoded for 5x3 viewports; later to be generalized
		enum { Width = 5, Height = 3 };

		// Size of the central viewport, ranging 0.01..1, where 1 is full original viewport size
		float CenterWidth[2];
		float CenterHeight;

		// Location of the central viewport, ranging 0..1, where 0.5 is the center of the screen
		float CenterX[2];
		float CenterY;

		// Split positions: where the splits between multi-res viewports occur, as a fractional
		// position within the full viewport (0 to 1), measuring from the top left
		float SplitsX[Width - 1];
		float SplitsY[Height - 1];

		// Pixel density scale factors: how much the linear pixel density is scaled within each
		// row and column (1.0 = full density)
		float DensityScaleX[Width];
		float DensityScaleY[Height];
	};

	// Viewport and scissor rectangles for a multi-res Config, to be sent to the graphics API
	struct Viewports
	{
		// Currently hardcoded for 3x3 viewports; later to be generalized
		enum { Count = Configuration::Width* Configuration::Height };

		FloatRect	Views[Count];
		FIntRect	Scissors[Count];

		// Rectangle enclosing all the viewports and scissors (for sizing render targets, etc.)
		FIntRect	BoundingRect;
	};

	// Viewport and scissor rectangles for a multi-res Config, to be sent to the graphics API
	struct StereoViewports
	{
		// Currently hardcoded for 3x3 viewports; later to be generalized
		enum { Count = StereoConfiguration::Width* StereoConfiguration::Height };

		FloatRect	Views[Count];
		FIntRect	Scissors[Count];

		// Gap between the stereo viewports
		int32		ViewportGap;

		// Rectangle enclosing all the viewports and scissors (for sizing render targets, etc.)
		FIntRect	BoundingRect;
	};

	// A couple of preset configurations, for convenience
	static CORE_API const Configuration Configuration_Conservative;
	static CORE_API const Configuration Configuration_Aggressive;
	static CORE_API const Configuration Configuration_SuperAggressive;

	// Calculate the fraction of pixels a multi-res configuration will render,
	// relative to ordinary non-multi-res rendering
	CORE_API float CalculatePixelCountFraction(const Configuration* Conf);

	// Calculate a configuration that's mirrored left-to-right; used for stereo rendering
	static CORE_API void CalculateMirroredConfig(const Configuration* Conf, Configuration* OutConfMirrored);

	// Calculate a stereo configuration that is mirrored on the centerline
	static CORE_API void CalculateStereoConfig(const Configuration* Conf, const FIntRect* OriginalViewport, const int32 ViewportGap, StereoConfiguration* OutStereoConf);


	// Modify a configuration by rounding the split positions off to the nearest pixel
	// (this ensures the center viewport is exactly 1:1 with ordinary, non-multi-res rendering)
	static CORE_API void RoundSplitsToNearestPixel(const FIntRect* OriginalViewport, Configuration* Conf);

	static CORE_API void CalculateSplits(const FIntRect* OriginalViewport, Configuration* Conf);
	static CORE_API void CalculateStereoSplits(const FIntRect* OriginalViewport, StereoConfiguration* Conf);

	// Calculate the actual viewports and scissor rects for a configuration
	static CORE_API void CalculateViewports(const FIntRect* OriginalViewport, const Configuration* Conf, Viewports* OutViewports);
	static CORE_API void CalculateStereoViewports(const FIntRect* OriginalViewport, const StereoConfiguration* Conf, StereoViewports* OutViewports);

	// Calculate FastGS constant buffer data for a configuration
	static CORE_API void CalculateFastGSCBData(const Configuration* Conf, FastGSCBData* OutCBData);
	static CORE_API void CalculateFastGSCBData(const StereoConfiguration* Conf, FastGSCBData* OutCBData);

	// Calculate constant buffer data for a configuration
	static CORE_API void CalculateRemapCBData(const Configuration* Conf, const Viewports* Viewports, RemapCBData* OutCBData);

	// Remap UVs from linear to multi-res
	static CORE_API FVector2D MapLinearToMultiRes(const RemapCBData* CBData, const FVector2D& UV);

	// Remap UVs from multi-res to linear
	static CORE_API FVector2D MapMultiResToLinear(const RemapCBData* CBData, const FVector2D& UV);

};

struct FLensMatchedShading : public FVRProjection
{
	struct Configuration
	{
		// Currently hardcoded for 2x2 viewports; later to be generalized
		enum { Width = 2, Height = 2 };

		float WarpLeft;
		float WarpRight;
		float WarpUp;
		float WarpDown;

		float SizeLeft;
		float SizeRight;
		float SizeUp;
		float SizeDown;
	};

	struct StereoConfiguration
	{
		// Currently hardcoded for 4x2 viewports; later to be generalized
		enum { Width = 4, Height = 2 };

		Configuration LeftConfig;
		Configuration RightConfig;
	};

	struct StereoViewports
	{
		// Currently hardcoded for 4x2 viewports; later to be generalized
		enum { Count = StereoConfiguration::Width* StereoConfiguration::Height };

		FloatRect	Views[Count];
		FIntRect	Scissors[Count];
		FIntRect	BoundingRect;
	};

	struct Viewports
	{
		// Currently hardcoded for 2x2 viewports; later to be generalized
		enum { Count = Configuration::Width* Configuration::Height };

		FloatRect	Views[Count];
		FIntRect	Scissors[Count];
		FIntRect	BoundingRect;

		static StereoViewports Merge(const Viewports& InLeft, const Viewports& InRight)
		{
			StereoViewports Out;

			FMemory::Memcpy(Out.Views, InLeft.Views, sizeof(InLeft.Views));
			FMemory::Memcpy(Out.Views + Count, InRight.Views, sizeof(InRight.Views));
			FMemory::Memcpy(Out.Scissors, InLeft.Scissors, sizeof(InLeft.Scissors));
			FMemory::Memcpy(Out.Scissors + Count, InRight.Scissors, sizeof(InRight.Scissors));

			Out.BoundingRect.Min.X = InLeft.BoundingRect.Min.X;
			Out.BoundingRect.Min.Y = InLeft.BoundingRect.Min.Y;
			Out.BoundingRect.Max.X = InRight.BoundingRect.Width() + InRight.BoundingRect.Min.X - InLeft.BoundingRect.Min.X;
			Out.BoundingRect.Max.Y = InRight.BoundingRect.Height() + InRight.BoundingRect.Min.Y - InLeft.BoundingRect.Min.Y;

			return Out;
		}

	};


	static CORE_API const Configuration Configuration_CrescentBay;

	static CORE_API void CalculateMirroredConfig(const Configuration* Conf, Configuration* RefConfMirrored);

	// Calculate a stereo configuration that is mirrored on the centerline
	static CORE_API void CalculateStereoConfig(const Configuration* Conf, const FIntRect* OriginalViewport, const int32 ViewportGap, StereoConfiguration* OutStereoConf);

	static CORE_API void RoundSplitsToNearestPixel(const FIntRect* OriginalViewport, Configuration* RefConf);

	static CORE_API void CalculateViewports(const FIntRect* OriginalViewport, const Configuration* Config, Viewports* RefViewports);
	static CORE_API void CalculateStereoViewports(const FIntRect* OriginalViewport, const StereoConfiguration* Conf, const int32 ViewportGap, StereoViewports* OutViewports);

	static CORE_API void CalculateFastGSCBData(const Configuration*	Conf, FastGSCBData* RefCBData);
	static CORE_API void CalculateFastGSCBData(const StereoConfiguration*	Conf, FastGSCBData* RefCBData);

	static CORE_API void CalculateRemapCBData(const Configuration*	Conf, const Viewports* Viewports, RemapCBData* RefCBData);
};