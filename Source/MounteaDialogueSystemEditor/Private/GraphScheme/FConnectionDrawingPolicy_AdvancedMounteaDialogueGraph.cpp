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
	const float DistanceY = End.Y - Start.Y;
	const float DistanceX = End.X - Start.X;

	// Vertical offset for the horizontal segment
	const float VerticalOffset = FMath::Min(abs(DistanceY) * 0.5f, 50.0f);

	// Calculate control points for the curve
	FVector2D ControlPoint1, ControlPoint2;

	if (DistanceY >= 0)
	{
		// Downward connection
		ControlPoint1 = FVector2D(Start.X, Start.Y + VerticalOffset);
		ControlPoint2 = FVector2D(End.X, End.Y - VerticalOffset);
	}
	else
	{
		// Upward connection
		ControlPoint1 = FVector2D(Start.X, Start.Y - VerticalOffset);
		ControlPoint2 = FVector2D(End.X, End.Y + VerticalOffset);
	}

	// Draw the spline
	FSlateDrawElement::MakeDrawSpaceSpline(
		DrawElementsList,
		StoredBackLayerID,
		Start,
		ControlPoint1,
		ControlPoint2,
		End,
		Params.WireThickness * WireThickness,
		ESlateDrawEffect::None,
		Params.WireColor
	);

	// Draw the arrow
	DrawArrow(ControlPoint2, End, Params);
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

void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	if (!ArrowImage)
	{
		return;
	}

	const FVector2D ArrowSize(16.0f, 16.0f);
	const FVector2D ArrowDirection = (EndPoint - StartPoint).GetSafeNormal();
	const FVector2D ArrowPosition = EndPoint - ArrowDirection * ArrowSize.X * 0.5f;

	const float AngleInRadians = FMath::Atan2(ArrowDirection.Y, ArrowDirection.X);

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