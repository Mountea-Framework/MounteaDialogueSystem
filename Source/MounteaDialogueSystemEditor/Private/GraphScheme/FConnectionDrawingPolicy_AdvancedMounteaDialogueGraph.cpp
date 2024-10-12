// All rights reserved Dominik Morse (Pavlicek) 2024


#include "FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph.h"

#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj)
	, StoredBackLayerID(InBackLayerID)
	, StoredFrontLayerID(InFrontLayerID)
	, ZoomFactor(InZoomFactor)
	, RoundRadius(FMath::Clamp(20.0f / InZoomFactor, 5.0f, 10.0f))
	, WireThickness(1.5f)
{
	if (const UMounteaDialogueGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
	{
		switch (GraphEditorSettings->GetArrowType())
		{
		case EArrowType::ERT_SimpleArrow:
			ArrowImage = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Graph.SimpleArrow"));
			break;
		case EArrowType::ERT_HollowArrow:
			ArrowImage = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Graph.HollowArrow"));
			break;
		case EArrowType::ERT_FancyArrow:
			ArrowImage = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Graph.FancyArrow"));
			break;
		case EArrowType::ERT_Bubble:
			ArrowImage = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Graph.Bubble"));
			break;
		case EArrowType::ERT_None:
		default:
			ArrowImage = nullptr;
		}
	}
	else
	{
		ArrowImage = FAppStyle::GetBrush( TEXT("GenericPlay") );
	}
	
	ArrowRadius = ArrowImage ? ArrowImage->ImageSize * ZoomFactor * 0.5f : FVector2D(0.f);
	MidpointImage = nullptr;
	MidpointRadius = FVector2D::ZeroVector;
	HoverDeemphasisDarkFraction = 0.8f;
}

void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	DrawManhattanConnection(StartPoint, EndPoint, Params);
}

void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	const FVector2D StartPoint = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndPoint = FGeometryHelper::CenterOf(EndGeom);
	DrawManhattanConnection(StartPoint, EndPoint, Params);
}

FVector2D FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	return FVector2D(0.0f, 1.0f);  // Default tangent for top-down connections
}

void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawManhattanConnection(const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	const FVector2D StartDirection(0.0f, 1.0f);
	const FVector2D EndDirection(0.0f, -1.0f);

	if (FMath::IsNearlyEqual((End - Start).SizeSquared(), 0.0f, KINDA_SMALL_NUMBER))
	{
		return;
	}

	const float DistanceY = End.Y - Start.Y;
	const float DistanceX = End.X - Start.X;

	if (FMath::IsNearlyZero(DistanceX, 1.0f))
	{
		// Straight vertical line
		FSlateDrawElement::MakeLines(
			DrawElementsList,
			StoredBackLayerID,
			FPaintGeometry(),
			{ Start, End },
			ESlateDrawEffect::None,
			Params.WireColor,
			false,
			Params.WireThickness * WireThickness
		);
	}
	else if (DistanceY < 2 * RoundRadius)
	{
		// Direct connection with a single curve
		FVector2D MidPoint = (Start + End) * 0.5f;
		FVector2D MidDirection = FVector2D(DistanceX, 0.0f).GetSafeNormal();

		DrawRadius(Start, StartDirection, MidPoint, MidDirection, 90);
		DrawRadius(MidPoint, MidDirection, End, EndDirection, 90);
	}
	else
	{
		// Connection with vertical segments and two curves
		FVector2D VerticalStart, VerticalStartDirection;
		FVector2D VerticalEnd, VerticalEndDirection;
		
		if (DistanceX > 0)
		{
			// Normal case: target is to the right
			DrawSimpleRadius(Start, StartDirection, 90, VerticalStart, VerticalStartDirection);
			DrawSimpleRadius(End, EndDirection, -90, VerticalEnd, VerticalEndDirection);
		}
		else
		{
			// Inverted case: target is to the left
			DrawSimpleRadius(Start, StartDirection, -90, VerticalStart, VerticalStartDirection);
			DrawSimpleRadius(End, EndDirection, 90, VerticalEnd, VerticalEndDirection);
		}

		// Draw vertical segment
		FSlateDrawElement::MakeLines(
			DrawElementsList,
			StoredBackLayerID,
			FPaintGeometry(),
			{ VerticalStart, FVector2D(VerticalStart.X, VerticalEnd.Y) },
			ESlateDrawEffect::None,
			Params.WireColor,
			false,
			Params.WireThickness * WireThickness
		);

		// Draw horizontal segment
		FSlateDrawElement::MakeLines(
			DrawElementsList,
			StoredBackLayerID,
			FPaintGeometry(),
			{ FVector2D(VerticalStart.X, VerticalEnd.Y), FVector2D(VerticalEnd.X, VerticalEnd.Y) },
			ESlateDrawEffect::None,
			Params.WireColor,
			false,
			Params.WireThickness * WireThickness
		);
	}

	// Draw the arrow
	DrawArrow(FVector2D(End.X, End.Y - GetRadiusOffset(90)), End, Params);
}
	
void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawSimpleRadius(const FVector2D& Start, const FVector2D& StartDirection, const int32& AngleDeg, FVector2D& out_End, FVector2D& out_EndDirection)
{
	float StartOffset = GetRadiusOffset(AngleDeg, false);
	float PerpendicularOffset = GetRadiusOffset(AngleDeg, true);
	FVector2D PerpendicularDirection = StartDirection.GetRotated(FMath::Sign(AngleDeg) * 90);
	out_EndDirection = StartDirection.GetRotated(AngleDeg);

	out_End = Start + (StartDirection * StartOffset + PerpendicularDirection * PerpendicularOffset);
	DrawRadius(Start, StartDirection, out_End, out_EndDirection, AngleDeg);
}

void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawRadius(const FVector2D& Start, const FVector2D& StartDirection, const FVector2D& End, const FVector2D& EndDirection, const int32& AngleDeg)
{
	const float Tangent = GetRadiusTangent(AngleDeg);

	FSlateDrawElement::MakeDrawSpaceSpline(
		DrawElementsList,
		StoredBackLayerID,
		Start, StartDirection * Tangent,
		End, EndDirection * Tangent,
		WireThickness
	);
}

float FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::GetRadiusOffset(const int32& AngleDeg, bool Perpendicular) const
{
	float RadiusOffset = 1.0f;
	int32 AbsAngle = FMath::Abs(AngleDeg);

	if (Perpendicular)
	{
		AbsAngle = 180 - AbsAngle;
	}

	switch (AbsAngle)
	{
		case 45:
			RadiusOffset *= FMath::Sqrt(2.0f) / 2.0f;
			break;
		case 90:
			RadiusOffset = 1.0f;  // Direct adjustment for 90-degree angles
			break;
		case 135:
			RadiusOffset *= (1.0f - (FMath::Sqrt(2.0f) / 2.0f));
			break;
		case 180:
			RadiusOffset *= 1.0f;  // Avoid extreme radius for straight lines
			break;
	}

	return RadiusOffset * ZoomFactor * FMath::Clamp(RoundRadius, 10.0f, 20.0f);  // Clamp the radius for stability
}

float FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::GetRadiusTangent(const int32& AngleDeg) const
{
	float Tangent = 2 * FMath::Sqrt(2.0f) - 1;

	switch (FMath::Abs(AngleDeg))
	{
		case 0:
			Tangent *= 4.0f / Tangent;
			break;
		case 45:
			Tangent *= 0.55166f;
			break;
		case 90:
			Tangent = 4 * (FMath::Sqrt(2.0f) - 1);
			break;
		case 135:
			Tangent *= 2.0f / Tangent;
			break;
		case 180:
			Tangent *= 4.0f / Tangent;
			break;
	}

	return Tangent * ZoomFactor * RoundRadius;
}

void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	if (!ArrowImage)
	{
		return;
	}

	const FVector2D ArrowSize(16.0f, 16.0f);  // Adjust this size as needed
	const FVector2D ArrowDirection = (EndPoint - StartPoint).GetSafeNormal();
	const FVector2D ArrowPosition = EndPoint - ArrowDirection * ArrowSize.X * 0.5f;

	//const float AngleInRadians = FMath::Atan2(ArrowDirection.Y, ArrowDirection.X);
	const float AngleInRadians = FMath::DegreesToRadians(90.f);

	FSlateDrawElement::MakeRotatedBox(
		DrawElementsList,
		StoredFrontLayerID,
		FPaintGeometry(ArrowPosition, ArrowSize, ZoomFactor),
		ArrowImage,
		ESlateDrawEffect::None,
		AngleInRadians,
		TOptional<FVector2D>(),
		FSlateDrawElement::RelativeToElement,
		Params.WireColor
	);
}