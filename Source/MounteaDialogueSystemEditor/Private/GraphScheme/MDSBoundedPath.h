// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not 
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"

class FSlateWindowElementList;

/**
 * Bounded polyline wire helpers. Replaces Slate splines (unbounded subdivision on large coordinates)
 * with validated, clipped line segments. All maths in double; conversion to float happens after clipping.
 */
namespace MDSBoundedPath
{
	constexpr int32 MaxCornerSegments = 12;
	/** Coordinates beyond this are treated as invalid geometry. */
	constexpr double MaxCoordinate = 1.0e7;

	/** Removes non-finite / duplicate points. Returns false if the path is unusable (invalid coordinate or < 2 points). */
	bool Sanitize(TArray<FVector2D>& Points);

	/** Replaces interior corners with arcs (<= MaxCornerSegments each). Radius shrinks where clearance is short. */
	void RoundCorners(const TArray<FVector2D>& Corners, double Radius, TArray<FVector2D>& OutPoints);

	double Length(const TArray<FVector2D>& Points);

	/** Point at arc-length Distance along path (clamped); OutDirection receives the segment direction. */
	FVector2D SampleAt(const TArray<FVector2D>& Points, double Distance, FVector2D& OutDirection);

	/** Clips the path to ClipRect and submits it as lines. */
	void Draw(FSlateWindowElementList& Elements, int32 LayerId, const TArray<FVector2D>& Points, const FBox2D& ClipRect, float Thickness, const FLinearColor& Color);
}
