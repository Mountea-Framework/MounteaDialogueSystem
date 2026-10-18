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

#include "MDSBoundedPath.h"

#include "Rendering/DrawElements.h"

namespace
{
	bool IsUsable(const FVector2D& P)
	{
		return FMath::IsFinite(P.X) && FMath::IsFinite(P.Y)
			&& FMath::Abs(P.X) <= MDSBoundedPath::MaxCoordinate
			&& FMath::Abs(P.Y) <= MDSBoundedPath::MaxCoordinate;
	}

	// Liang-Barsky
	bool ClipSegment(FVector2D& A, FVector2D& B, const FBox2D& Rect)
	{
		double t0 = 0.0;
		double t1 = 1.0;
		const FVector2D delta = B - A;

		const auto clip = [&t0, &t1](double P, double Q) -> bool
		{
			if (FMath::IsNearlyZero(P))
				return Q >= 0.0;
			const double r = Q / P;
			if (P < 0.0)
			{
				if (r > t1) return false;
				t0 = FMath::Max(t0, r);
			}
			else
			{
				if (r < t0) return false;
				t1 = FMath::Min(t1, r);
			}
			return true;
		};

		if (!clip(-delta.X, A.X - Rect.Min.X) || !clip(delta.X, Rect.Max.X - A.X)
			|| !clip(-delta.Y, A.Y - Rect.Min.Y) || !clip(delta.Y, Rect.Max.Y - A.Y))
			return false;

		const FVector2D newB = A + delta * t1;
		A = A + delta * t0;
		B = newB;
		return true;
	}
}

bool MDSBoundedPath::Sanitize(TArray<FVector2D>& Points)
{
	TArray<FVector2D> clean;
	clean.Reserve(Points.Num());
	for (const FVector2D& point : Points)
	{
		if (!IsUsable(point))
			return false;
		if (clean.Num() == 0 || !(point - clean.Last()).IsNearlyZero(KINDA_SMALL_NUMBER))
			clean.Add(point);
	}
	Points = MoveTemp(clean);
	return Points.Num() >= 2;
}

void MDSBoundedPath::RoundCorners(const TArray<FVector2D>& Corners, double Radius, TArray<FVector2D>& OutPoints)
{
	OutPoints.Reset();
	if (Corners.Num() == 0)
		return;

	OutPoints.Add(Corners[0]);
	for (int32 i = 1; i < Corners.Num() - 1; ++i)
	{
		const FVector2D& prev = Corners[i - 1];
		const FVector2D& corner = Corners[i];
		const FVector2D& next = Corners[i + 1];

		const double lenIn = (corner - prev).Size();
		const double lenOut = (next - corner).Size();
		const FVector2D dirIn = lenIn > UE_DOUBLE_SMALL_NUMBER ? (corner - prev) / lenIn : FVector2D::ZeroVector;
		const FVector2D dirOut = lenOut > UE_DOUBLE_SMALL_NUMBER ? (next - corner) / lenOut : FVector2D::ZeroVector;

		// Half of each adjacent segment at most, so neighbouring arcs never overlap.
		const double r = FMath::Min3(Radius, lenIn * 0.5, lenOut * 0.5);
		if (r < 0.5 || FMath::Abs(FVector2D::CrossProduct(dirIn, dirOut)) < 1.0e-4)
		{
			OutPoints.Add(corner);
			continue;
		}

		const FVector2D entry = corner - dirIn * r;
		const FVector2D exit = corner + dirOut * r;
		const int32 segments = FMath::Clamp(FMath::CeilToInt(r * 0.5), 2, MaxCornerSegments);
		for (int32 s = 0; s <= segments; ++s)
		{
			const double t = static_cast<double>(s) / segments;
			const double u = 1.0 - t;
			OutPoints.Add(entry * (u * u) + corner * (2.0 * u * t) + exit * (t * t));
		}
	}
	if (Corners.Num() > 1)
		OutPoints.Add(Corners.Last());
}

double MDSBoundedPath::Length(const TArray<FVector2D>& Points)
{
	double length = 0.0;
	for (int32 i = 1; i < Points.Num(); ++i)
		length += (Points[i] - Points[i - 1]).Size();
	return length;
}

FVector2D MDSBoundedPath::SampleAt(const TArray<FVector2D>& Points, double Distance, FVector2D& OutDirection)
{
	OutDirection = FVector2D(0.0, 1.0);
	if (Points.Num() == 0)
		return FVector2D::ZeroVector;

	double remaining = FMath::Max(Distance, 0.0);
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		const double segLen = (Points[i] - Points[i - 1]).Size();
		if (segLen <= UE_DOUBLE_SMALL_NUMBER)
			continue;
		OutDirection = (Points[i] - Points[i - 1]) / segLen;
		if (remaining <= segLen)
			return Points[i - 1] + OutDirection * remaining;
		remaining -= segLen;
	}
	return Points.Last();
}

void MDSBoundedPath::Draw(FSlateWindowElementList& Elements, int32 LayerId, const TArray<FVector2D>& Points, const FBox2D& ClipRect, float Thickness, const FLinearColor& Color)
{
	const auto flush = [&](TArray<FVector2f>& Run)
	{
		if (Run.Num() >= 2)
			FSlateDrawElement::MakeLines(Elements, LayerId, FPaintGeometry(), MoveTemp(Run), ESlateDrawEffect::None, Color, true, Thickness);
		Run.Reset();
	};

	const FBox2D rect = ClipRect.ExpandBy(Thickness * 2.0);
	TArray<FVector2f> run;
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		FVector2D a = Points[i - 1];
		FVector2D b = Points[i];
		if (!ClipSegment(a, b, rect))
		{
			flush(run);
			continue;
		}

		const FVector2f fa(a);
		const FVector2f fb(b);
		if (run.Num() > 0 && !run.Last().Equals(fa, 0.01f))
			flush(run);
		if (run.Num() == 0)
			run.Add(fa);
		run.Add(fb);
	}
	flush(run);
}
