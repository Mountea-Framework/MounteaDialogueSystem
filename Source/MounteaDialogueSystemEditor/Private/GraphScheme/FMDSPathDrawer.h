// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
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
#include "ConnectionDrawingPolicy.h"

class FMDSPathDrawer
{
public:
	FMDSPathDrawer(int32 InLayerId, float InZoomFactor, FSlateWindowElementList& InDrawElements, const FConnectionParams& InParams);

	void DrawSubwayWire(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir);
	void DrawManhattanWire(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir);

	FVector2D GetArcLengthMidpoint() const;

private:
	void DrawLine(const FVector2D& Start, const FVector2D& End);
	void DrawRadius(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir, int32 AngleDeg);
	void DrawSimpleRadius(const FVector2D& Start, const FVector2D& StartDir, int32 AngleDeg, FVector2D& OutEnd, FVector2D& OutEndDir, bool Backward = false);
	void DrawUTurn(const FVector2D& Start, const FVector2D& StartDir, float Direction, FVector2D& OutEnd, FVector2D& OutEndDir, bool Backward = false);
	void DrawCorrectionOrtho(const FVector2D& Start, const FVector2D& StartDir, float Displacement, FVector2D& OutEnd, FVector2D& OutEndDir, bool Backward = false);
	void DrawOffset(FVector2D& Start, FVector2D& StartDir, float Offset, bool Backward = false);
	void DrawIntersectionRadius(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir);
	void DrawIntersectionDiagRadius(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir);

	float GetRadiusOffset(int32 AngleDeg = 0, bool Perpendicular = false) const;
	float GetRadiusTangent(int32 AngleDeg = 0) const;
	float GetIntersectionOffset(int32 AngleDeg = 0, bool Diagonal = false) const;

	struct FMDSSegment
	{
		FVector2D A;
		FVector2D B;
		float Length;
	};

	TArray<FMDSSegment> Segments;

	int32 LayerId;
	float ZoomFactor;
	int32 MaxDepth;

	static constexpr float RoundRadius = 10.0f;

	FSlateWindowElementList* DrawElements;
	const FConnectionParams* Params;
};
