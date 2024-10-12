// All rights reserved Dominik Morse (Pavlicek) 2024


#include "FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph.h"

#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj)
	, StoredBackLayerID(InBackLayerID)
	, StoredFrontLayerID(InFrontLayerID)
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
	return FVector2D(0.f, 0.f);  // Not used for Manhattan-style lines
}

void FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph::DrawManhattanConnection(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	const float VerticalOffset = 20.0f;  // Adjust this value to control the vertical segment positioning
	const float HorizontalOffset = 0.0f;  // Adjust this value to control the horizontal segment positioning

	TArray<FVector2D> Points;
	Points.Add(StartPoint);

	// Determine if we need to go up or down first
	if (EndPoint.Y > StartPoint.Y)
	{
		// Go down first
		Points.Add(FVector2D(StartPoint.X, StartPoint.Y + VerticalOffset));
		Points.Add(FVector2D(EndPoint.X - HorizontalOffset, StartPoint.Y + VerticalOffset));
		Points.Add(FVector2D(EndPoint.X - HorizontalOffset, EndPoint.Y));
	}
	else
	{
		// Go up first
		Points.Add(FVector2D(StartPoint.X, EndPoint.Y - VerticalOffset));
		Points.Add(FVector2D(EndPoint.X - HorizontalOffset, EndPoint.Y - VerticalOffset));
		Points.Add(FVector2D(EndPoint.X - HorizontalOffset, EndPoint.Y));
	}

	Points.Add(EndPoint);

	// Draw the Manhattan connection
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		FSlateDrawElement::MakeLines(
			DrawElementsList,
			StoredBackLayerID,
			FPaintGeometry(),
			{ Points[i], Points[i + 1] },
			ESlateDrawEffect::None,
			Params.WireColor,
			false,
			Params.WireThickness
		);
	}

	float imageOffset = ArrowImage ? ArrowImage->GetImageSize().X / 2 : 8.f;
	FVector2D ArrowEndpoint = FVector2D(EndPoint.X - imageOffset, EndPoint.Y - VerticalOffset - imageOffset);

	// Draw the arrow at the end
	DrawArrow(Points[Points.Num() - 2], ArrowEndpoint, Params);
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