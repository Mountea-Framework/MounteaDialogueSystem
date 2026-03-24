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

#include "FMDSPathDrawer.h"
#include "Rendering/DrawElements.h"

FMDSPathDrawer::FMDSPathDrawer(int32 InLayerId, float InZoomFactor, FSlateWindowElementList& InDrawElements, const FConnectionParams& InParams)
	: LayerId(InLayerId)
	, ZoomFactor(InZoomFactor)
	, MaxDepth(5)
	, DrawElements(&InDrawElements)
	, Params(&InParams)
{
}

void FMDSPathDrawer::DrawSubwayWire(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir)
{
	if (!MaxDepth--)
		return;

	if (FMath::IsNearlyZero((End - Start).SizeSquared(), KINDA_SMALL_NUMBER))
		return;

	const bool bStartDir90 = FMath::IsNearlyEqual(FMath::Abs(StartDir.X), 1.0f, KINDA_SMALL_NUMBER) || FMath::IsNearlyEqual(FMath::Abs(StartDir.Y), 1.0f, KINDA_SMALL_NUMBER);
	const bool bEndDir90   = FMath::IsNearlyEqual(FMath::Abs(EndDir.X),   1.0f, KINDA_SMALL_NUMBER) || FMath::IsNearlyEqual(FMath::Abs(EndDir.Y),   1.0f, KINDA_SMALL_NUMBER);

	const bool bSameDirection    = FVector2D::DistSquared(StartDir, EndDir) < KINDA_SMALL_NUMBER;
	const bool bStraightDirection= FMath::IsNearlyZero(FVector2D::CrossProduct(StartDir, EndDir), KINDA_SMALL_NUMBER);
	const bool bForwardDirection = FVector2D::DotProduct(End - Start, StartDir) > KINDA_SMALL_NUMBER;

	const float distanceOrtho    = FVector2D::CrossProduct(End - Start, StartDir);
	const float distanceStraight = FVector2D::DotProduct(End - Start, StartDir);

	const float directionOffset  = (FMath::Abs(End.X - Start.X) - FMath::Abs(End.Y - Start.Y))
		* (FMath::IsNearlyEqual(FMath::Abs(StartDir.X), 1.0f, KINDA_SMALL_NUMBER) ? 1.0f : -1.0f);

	const int32 directionAngle = FMath::Sign(distanceOrtho);

	FVector2D newStart = Start, newStartDir = StartDir;
	FVector2D newEnd   = End,   newEndDir   = EndDir;

	if (!bStraightDirection)
	{
		if (bStartDir90 && bEndDir90)
			DrawIntersectionDiagRadius(Start, StartDir, End, EndDir);
		else
			DrawIntersectionRadius(Start, StartDir, End, EndDir);
		return;
	}
	else if (bSameDirection)
	{
		if ((Start + StartDir * (float)FVector2D::Distance(Start, End) - End).IsNearlyZero(KINDA_SMALL_NUMBER))
		{
			DrawLine(Start, End);
			return;
		}
		else if ((float)FVector2D::Distance(Start, End) < 4.0f * GetRadiusOffset())
		{
			DrawManhattanWire(Start, StartDir, End, EndDir);
			return;
		}
		else if (FMath::IsNearlyEqual(FVector2D::DotProduct((End - Start).GetSafeNormal(), StartDir), -1.0f, KINDA_SMALL_NUMBER))
		{
			DrawSimpleRadius(Start, StartDir, -90, newStart, newStartDir, false);
			DrawSimpleRadius(End,   EndDir,    90, newEnd,   newEndDir,   true);
		}
		else if (bForwardDirection && directionOffset > 2.0f * GetIntersectionOffset(45, false))
		{
			if (FMath::Abs(distanceOrtho) < 2.0f * GetIntersectionOffset(45, true))
			{
				if ((FMath::Abs(distanceOrtho) < 2.0f * GetIntersectionOffset(45, false)) && (FMath::Abs(distanceStraight) > 2.0f * GetRadiusOffset()))
					DrawCorrectionOrtho(End, EndDir, distanceOrtho, newEnd, newEndDir, true);
			}
			else
				DrawSimpleRadius(End, EndDir, -45 * directionAngle, newEnd, newEndDir, true);
		}
		else
		{
			if (distanceStraight < 2.0f * GetRadiusOffset())
			{
				DrawSimpleRadius(Start, StartDir,  90 * (-FMath::Sign(distanceOrtho)), newStart, newStartDir, false);
				DrawSimpleRadius(End,   EndDir,    90 * (-FMath::Sign(distanceOrtho)), newEnd,   newEndDir,   true);
			}
			else if (directionOffset > 2.0f * GetIntersectionOffset(45, false))
			{
				DrawSimpleRadius(End, EndDir, -45 * directionAngle, newEnd, newEndDir, true);
			}
			else
			{
				DrawSimpleRadius(End, EndDir, -90 * directionAngle, newEnd, newEndDir, true);
			}
		}
	}
	else
	{
		if (FMath::Sign(distanceStraight) > 0)
			DrawSimpleRadius(End, EndDir,  90 * directionAngle, newEnd, newEndDir, true);
		else
			DrawSimpleRadius(Start, StartDir, -90 * directionAngle, newStart, newStartDir, false);
	}

	DrawSubwayWire(newStart, newStartDir, newEnd, newEndDir);
}

void FMDSPathDrawer::DrawManhattanWire(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir)
{
	if (!MaxDepth--)
		return;

	if (FMath::IsNearlyZero((End - Start).SizeSquared(), KINDA_SMALL_NUMBER))
		return;

	const bool bSameDirection    = FVector2D::DistSquared(StartDir, EndDir) < KINDA_SMALL_NUMBER;
	const bool bStraightDirection= FMath::IsNearlyZero(FVector2D::CrossProduct(StartDir, EndDir), KINDA_SMALL_NUMBER);
	const bool bForwardDirection = FVector2D::DotProduct(End - Start, StartDir) > KINDA_SMALL_NUMBER;

	const float distanceOrtho    = FVector2D::CrossProduct(End - Start, StartDir);
	const float distanceStraight = FVector2D::DotProduct(End - Start, StartDir);
	const int32 directionAngle   = FMath::Sign(distanceOrtho);

	FVector2D newStart = Start, newStartDir = StartDir;
	FVector2D newEnd   = End,   newEndDir   = EndDir;

	if (bSameDirection)
	{
		if ((Start + StartDir * (float)FVector2D::Distance(Start, End) - End).IsNearlyZero(KINDA_SMALL_NUMBER))
		{
			DrawLine(Start, End);
			return;
		}
		else if (FMath::IsNearlyEqual(FVector2D::DotProduct((End - Start).GetSafeNormal(), StartDir), -1.0f, KINDA_SMALL_NUMBER))
		{
			DrawSimpleRadius(Start, StartDir, -90, newStart, newStartDir, false);
			DrawSimpleRadius(End,   EndDir,    90, newEnd,   newEndDir,   true);
		}
		else if ((FMath::Abs(End.X - Start.X) < 2.0f * GetRadiusOffset()) && (FMath::Abs(End.Y - Start.Y) < 4.0f * GetRadiusOffset()))
		{
			const float multiplier = (float)FVector2D::Distance(Start, End) / 32.0f;
			const float tangent    = (End - Start).Size();
			FSlateDrawElement::MakeDrawSpaceSpline(*DrawElements, LayerId,
				Start, StartDir * multiplier * tangent, End, EndDir * multiplier * tangent,
				Params->WireThickness, ESlateDrawEffect::None, Params->WireColor);
			Segments.Add({ Start, End, (float)FVector2D::Distance(Start, End) });
			return;
		}
		else if (!bForwardDirection && (FMath::Abs(distanceOrtho) < 4.0f * GetRadiusOffset()))
		{
			DrawUTurn(Start, StartDir, static_cast<float>(directionAngle), newStart, newStartDir, false);
		}
		else if (FMath::Abs(End.Y - Start.Y) < 2.0f * GetRadiusOffset())
		{
			DrawCorrectionOrtho(End, EndDir, distanceOrtho, newEnd, newEndDir, true);
		}
		else if (FMath::Abs(End.X - Start.X) < 2.0f * GetRadiusOffset()
			&& FMath::IsNearlyEqual(StartDir.Y, 1.0f, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(EndDir.Y, 1.0f, KINDA_SMALL_NUMBER))
		{
			DrawCorrectionOrtho(End, EndDir, distanceOrtho, newEnd, newEndDir, true);
		}
		else
		{
			if (distanceStraight < 2.0f * GetRadiusOffset())
			{
				if (FMath::Abs(distanceOrtho) < 4.0f * GetRadiusOffset())
					DrawUTurn(Start, StartDir, static_cast<float>(directionAngle), newStart, newStartDir, false);
				else
					DrawUTurn(End, EndDir, static_cast<float>(directionAngle), newEnd, newEndDir, true);
			}
			else
			{
				const float direction = -FMath::Sign(distanceOrtho);
				DrawSimpleRadius(End, EndDir, static_cast<int32>(90.0f * direction), newEnd, newEndDir, true);
			}
		}
	}
	else if (!bStraightDirection)
	{
		DrawIntersectionRadius(Start, StartDir, End, EndDir);
		return;
	}
	else
	{
		if (FMath::Sign(distanceStraight) > 0)
			DrawSimpleRadius(End, EndDir,  90 * directionAngle, newEnd, newEndDir, true);
		else
			DrawSimpleRadius(Start, StartDir, -90 * directionAngle, newStart, newStartDir, false);
	}

	DrawManhattanWire(newStart, newStartDir, newEnd, newEndDir);
}

FVector2D FMDSPathDrawer::GetArcLengthMidpoint() const
{
	float totalLength = 0.0f;
	for (const FMDSSegment& seg : Segments)
		totalLength += seg.Length;

	if (FMath::IsNearlyZero(totalLength))
		return Segments.Num() > 0 ? (Segments[0].A + Segments[0].B) * 0.5f : FVector2D::ZeroVector;

	const float halfLength = totalLength * 0.5f;
	float accumulated = 0.0f;
	for (const FMDSSegment& seg : Segments)
	{
		if (accumulated + seg.Length >= halfLength)
		{
			const float t = (halfLength - accumulated) / seg.Length;
			return FMath::Lerp(seg.A, seg.B, t);
		}
		accumulated += seg.Length;
	}

	const FMDSSegment& last = Segments.Last();
	return FMath::Lerp(last.A, last.B, 1.0f);
}

void FMDSPathDrawer::DrawLine(const FVector2D& Start, const FVector2D& End)
{
	if (FMath::IsNearlyZero((End - Start).SizeSquared(), KINDA_SMALL_NUMBER))
		return;

	FSlateDrawElement::MakeDrawSpaceSpline(*DrawElements, LayerId,
		Start, FVector2D::ZeroVector, End, FVector2D::ZeroVector,
		Params->WireThickness, ESlateDrawEffect::None, Params->WireColor);

	Segments.Add({ Start, End, (float)FVector2D::Distance(Start, End) });
}

void FMDSPathDrawer::DrawRadius(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir, int32 AngleDeg)
{
	const float tangent = GetRadiusTangent(AngleDeg);

	FSlateDrawElement::MakeDrawSpaceSpline(*DrawElements, LayerId,
		Start, StartDir * tangent, End, EndDir * tangent,
		Params->WireThickness, ESlateDrawEffect::None, Params->WireColor);

	Segments.Add({ Start, End, (float)FVector2D::Distance(Start, End) });
}

void FMDSPathDrawer::DrawSimpleRadius(const FVector2D& Start, const FVector2D& StartDir, int32 AngleDeg, FVector2D& OutEnd, FVector2D& OutEndDir, bool Backward)
{
	const float startOffset        = GetRadiusOffset(AngleDeg, false);
	const float perpendicularOffset= GetRadiusOffset(AngleDeg, true);
	const FVector2D perpDir        = StartDir.GetRotated(FMath::Sign(AngleDeg) * 90.0f);
	OutEndDir = StartDir.GetRotated(static_cast<float>(AngleDeg));

	if (Backward)
	{
		OutEnd = Start - (StartDir * startOffset + perpDir * perpendicularOffset);
		DrawRadius(OutEnd, OutEndDir, Start, StartDir, AngleDeg);
	}
	else
	{
		OutEnd = Start + (StartDir * startOffset + perpDir * perpendicularOffset);
		DrawRadius(Start, StartDir, OutEnd, OutEndDir, AngleDeg);
	}
}

void FMDSPathDrawer::DrawUTurn(const FVector2D& Start, const FVector2D& StartDir, float Direction, FVector2D& OutEnd, FVector2D& OutEndDir, bool Backward)
{
	const float backwardDir         = Backward ? -1.0f : 1.0f;
	const FVector2D midDir          = StartDir.GetRotated(FMath::Sign(Direction) * 90.0f * backwardDir);
	const FVector2D mid             = Start + (StartDir + midDir) * GetRadiusOffset() * backwardDir;

	OutEndDir = -StartDir;
	OutEnd    = Start + midDir * 2.0f * GetRadiusOffset() * backwardDir;

	if (Backward)
	{
		DrawRadius(OutEnd, OutEndDir, mid, midDir, 90);
		DrawRadius(mid,    midDir,    Start, StartDir, 90);
	}
	else
	{
		DrawRadius(Start, StartDir, mid,    midDir,    90);
		DrawRadius(mid,   midDir,   OutEnd, OutEndDir, 90);
	}
}

void FMDSPathDrawer::DrawCorrectionOrtho(const FVector2D& Start, const FVector2D& StartDir, float Displacement, FVector2D& OutEnd, FVector2D& OutEndDir, bool Backward)
{
	OutEndDir = StartDir;
	const FVector2D orthoDir = StartDir.GetRotated(90.0f);

	if (Backward)
	{
		OutEnd = Start - 2.0f * StartDir * GetRadiusOffset() + orthoDir * Displacement;
		const float tangent = GetRadiusTangent();
		FSlateDrawElement::MakeDrawSpaceSpline(*DrawElements, LayerId,
			OutEnd, OutEndDir * tangent, Start, StartDir * tangent,
			Params->WireThickness, ESlateDrawEffect::None, Params->WireColor);
		Segments.Add({ OutEnd, Start, (float)FVector2D::Distance(OutEnd, Start) });
	}
	else
	{
		OutEnd = Start + 2.0f * StartDir * GetRadiusOffset() + orthoDir * Displacement;
		const float tangent = GetRadiusTangent();
		FSlateDrawElement::MakeDrawSpaceSpline(*DrawElements, LayerId,
			Start, StartDir * tangent, OutEnd, OutEndDir * tangent,
			Params->WireThickness, ESlateDrawEffect::None, Params->WireColor);
		Segments.Add({ Start, OutEnd, (float)FVector2D::Distance(Start, OutEnd) });
	}
}

void FMDSPathDrawer::DrawOffset(FVector2D& Start, FVector2D& StartDir, float Offset, bool Backward)
{
	FVector2D newStart = Start;

	if (Backward)
	{
		newStart -= StartDir * Offset;
		DrawLine(newStart, Start);
	}
	else
	{
		newStart += StartDir * Offset;
		DrawLine(Start, newStart);
	}

	Start = newStart;
}

void FMDSPathDrawer::DrawIntersectionRadius(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir)
{
	const int32 angleDeg    = FMath::RoundToInt(FMath::UnwindDegrees(
		FMath::RadiansToDegrees(FMath::Atan2(StartDir.Y, StartDir.X) - FMath::Atan2(EndDir.Y, EndDir.X))));
	const float startOffset = GetIntersectionOffset(angleDeg, false);
	const float endOffset   = GetIntersectionOffset(angleDeg, true);

	const float t             = (EndDir.X * (Start.Y - End.Y) - EndDir.Y * (Start.X - End.X))
		/ (-EndDir.X * StartDir.Y + StartDir.X * EndDir.Y);
	const FVector2D intersect = Start + t * StartDir;

	const FVector2D startStop = intersect - StartDir * startOffset;
	const FVector2D endStop   = intersect + EndDir   * endOffset;

	DrawLine(Start, startStop);
	DrawRadius(startStop, StartDir, endStop, EndDir, angleDeg);
	DrawLine(endStop, End);
}

void FMDSPathDrawer::DrawIntersectionDiagRadius(const FVector2D& Start, const FVector2D& StartDir, const FVector2D& End, const FVector2D& EndDir)
{
	const float directionOffset = FMath::Abs(End.X - Start.X) - FMath::Abs(End.Y - Start.Y);

	FVector2D newStart = Start;
	FVector2D newEnd   = End;

	FVector2D newStartClose, newStartCloseDir;
	FVector2D newEndClose,   newEndCloseDir;
	int32 direction;

	if (FMath::IsNearlyEqual(FMath::Abs(StartDir.X), 1.0f, KINDA_SMALL_NUMBER))
	{
		direction = FMath::RoundToInt(FMath::Sign(End.Y - Start.Y) * StartDir.X);
		if (directionOffset > 0.0f)
			newStart += FVector2D(1.0f, 0.0f) * directionOffset * StartDir.X;
		else
			newEnd   += FVector2D(0.0f, 1.0f) * directionOffset * FMath::Sign(End.Y - Start.Y);
	}
	else
	{
		direction = FMath::RoundToInt(FMath::Sign(Start.Y - End.Y) * EndDir.X);
		if (directionOffset > 0.0f)
			newEnd   -= FVector2D(1.0f, 0.0f) * directionOffset * EndDir.X;
		else
			newStart -= FVector2D(0.0f, 1.0f) * directionOffset * FMath::Sign(End.Y - Start.Y);
	}

	DrawLine(Start, newStart);
	DrawLine(newEnd, End);

	DrawSimpleRadius(newStart, StartDir,  45 * direction, newStartClose, newStartCloseDir, false);
	DrawSimpleRadius(newEnd,   EndDir,   -45 * direction, newEndClose,   newEndCloseDir,   true);

	DrawLine(newStartClose, newEndClose);
}

float FMDSPathDrawer::GetRadiusOffset(int32 AngleDeg, bool Perpendicular) const
{
	float offset  = 1.0f;
	int32 absAngle = FMath::Abs(AngleDeg);

	if (Perpendicular)
		absAngle = 180 - absAngle;

	switch (absAngle)
	{
	case 45:  offset *= FMath::Sqrt(2.0f) / 2.0f;                    break;
	case 90:  offset *= 1.0f;                                          break;
	case 135: offset *= (1.0f - (FMath::Sqrt(2.0f) / 2.0f));          break;
	case 180: offset *= 2.0f;                                          break;
	default:  break;
	}

	return offset * ZoomFactor * RoundRadius;
}

float FMDSPathDrawer::GetRadiusTangent(int32 AngleDeg) const
{
	float tangent = 2.0f * FMath::Sqrt(2.0f) - 1.0f;

	switch (FMath::Abs(AngleDeg))
	{
	case 0:   tangent *= 4.0f / tangent;       break;
	case 45:  tangent *= 0.55166f;             break;
	case 90:  tangent  = 4.0f * (FMath::Sqrt(2.0f) - 1.0f); break;
	case 135: tangent *= 2.0f / tangent;       break;
	case 180: tangent *= 4.0f / tangent;       break;
	default:  break;
	}

	return tangent * ZoomFactor * RoundRadius;
}

float FMDSPathDrawer::GetIntersectionOffset(int32 AngleDeg, bool Diagonal) const
{
	float offset = 1.0f;

	switch (FMath::Abs(AngleDeg))
	{
	case 45:
		offset *= Diagonal
			? (1.0f - FMath::Sqrt(2.0f) / 2.0f) * FMath::Sqrt(2.0f)
			: (FMath::Sqrt(2.0f) - 1.0f);
		break;
	case 90:
		offset *= 1.0f;
		break;
	default:
		break;
	}

	return offset * ZoomFactor * RoundRadius;
}
