// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FConnectionDrawingPolicy_MounteaDialogueGraph.h"

#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

FConnectionDrawingPolicy_MounteaDialogueGraph::FConnectionDrawingPolicy_MounteaDialogueGraph
(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
	  , GraphObj(InGraphObj)
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
		ArrowImage = FEditorStyle::GetBrush( TEXT("GenericPlay") );
	}
	
	ArrowRadius = ArrowImage ? ArrowImage->ImageSize * ZoomFactor * 0.5f : FVector2D(0.f);
	MidpointImage = nullptr;
	MidpointRadius = FVector2D::ZeroVector;
	HoverDeemphasisDarkFraction = 0.8f;

	BubbleImage = FEditorStyle::GetBrush( TEXT("Graph.Arrow") );
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;

	const UMounteaDialogueGraphEditorSettings* MounteaDialogueGraphEditorSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if (MounteaDialogueGraphEditorSettings)
	{
		Params.WireThickness = MounteaDialogueGraphEditorSettings->GetWireWidth();
	}
	else
	{
		Params.WireThickness = 1.f;
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build an acceleration structure to quickly find geometry for the nodes
	NodeWidgetMap.Empty();
	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		FArrangedWidget& CurWidget = ArrangedNodes[NodeIndex];
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
		NodeWidgetMap.Add(ChildNode->GetNodeObj(), NodeIndex);
	}

	// Now draw
	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	/*
	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);
	*/

	DrawSplineWithArrow(StartCenter, EndCenter, Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	// bUserFlag1 indicates that we need to reverse the direction of connection (used by debugger)
	const FVector2D& P0 = Params.bUserFlag1 ? EndPoint : StartPoint;
	const FVector2D& P1 = Params.bUserFlag1 ? StartPoint : EndPoint;

	EditorLOG_INFO(TEXT("%s I %s"), *P0.ToString(), *P1.ToString() )
	
	FConnectionParams NewParams = Params;
	//NewParams.bDrawBubbles = true;
	
	if (const UMounteaDialogueGraphEditorSettings* MounteaDialogueGraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
	{
		NewParams.WireThickness = MounteaDialogueGraphEditorSettings->GetWireWidth();
	}
	
	Internal_DrawLineWithArrow(P0, P1, NewParams);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams Params;
	DetermineWiringStyle(Pin, nullptr, /*inout*/ Params);

	if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, EndPoint), EndPoint, Params);
	}
	else
	{
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, StartPoint), StartPoint, Params);
	}
}

FVector2D FConnectionDrawingPolicy_MounteaDialogueGraph::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	const FVector2D Delta = End - Start;
	const FVector2D NormDelta = Delta.GetSafeNormal();

	return NormDelta;
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget, UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FArrangedWidget*& StartWidgetGeometry, FArrangedWidget*& EndWidgetGeometry)
{
	if (UEdNode_MounteaDialogueGraphEdge* EdgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(InputPin->GetOwningNode()))
	{
		UEdNode_MounteaDialogueGraphNode* Start = EdgeNode->GetStartNode();
		UEdNode_MounteaDialogueGraphNode* End = EdgeNode->GetEndNode();
		if (Start != nullptr && End != nullptr)
		{
			int32* StartNodeIndex = NodeWidgetMap.Find(Start);
			int32* EndNodeIndex = NodeWidgetMap.Find(End);
			if (StartNodeIndex != nullptr && EndNodeIndex != nullptr)
			{
				StartWidgetGeometry = &(ArrangedNodes[*StartNodeIndex]);
				EndWidgetGeometry = &(ArrangedNodes[*EndNodeIndex]);
			}
		}
	}
	else
	{
		StartWidgetGeometry = PinGeometries->Find(OutputPinWidget);

		if (TSharedPtr<SGraphPin>* pTargetWidget = PinToPinWidgetMap.Find(InputPin))
		{
			TSharedRef<SGraphPin> InputWidget = (*pTargetWidget).ToSharedRef();
			EndWidgetGeometry = PinGeometries->Find(InputWidget);
		}
	}
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params)
{
	const float LineSeparationAmount = 4.5f;

	const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
	const FVector2D UnitDelta = DeltaPos.GetSafeNormal();
	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	// Come up with the final start/end points
	const FVector2D DirectionBias = Normal * LineSeparationAmount;
	const FVector2D LengthBias = ArrowRadius.X * UnitDelta;
	const FVector2D StartPoint = StartAnchorPoint + DirectionBias + LengthBias;
	const FVector2D EndPoint = EndAnchorPoint + DirectionBias - LengthBias;

	// this will be exposed as settings variable
	const float HorizontalThreshold = 10.f;
	const float VerticalThreshold = 10.f;
	
	const bool ConditionForScenario1 = 
	FMath::Abs(StartPoint.X - EndPoint.X) <= HorizontalThreshold;

	const bool ConditionForScenario2 = 
		EndPoint.Y < StartPoint.Y &&
		FMath::Abs(StartPoint.X - EndPoint.X) > HorizontalThreshold;

	const bool ConditionForScenario3 = 
		EndPoint.Y >= StartPoint.Y;

	
	// Choose connection drawing method based on conditions
	if (ConditionForScenario1)
	{
		DrawConnection(WireLayerID, StartPoint, EndPoint, Params);
	}
	else if (ConditionForScenario2)
	{
		DrawConnectionDown(WireLayerID, StartPoint, EndPoint, Params);
	}
	else if (ConditionForScenario3)
	{
		DrawConnectionUp(WireLayerID, StartPoint, EndPoint, Params);
	}
	else
	{
		DrawConnection(WireLayerID, StartPoint, EndPoint, Params);
	}

	// Draw the arrow
	if (ArrowImage)
	{
		const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
		const float AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);

		FSlateDrawElement::MakeRotatedBox(
			DrawElementsList,
			ArrowLayerID,
			FPaintGeometry(ArrowDrawPos, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
			ArrowImage,
			ESlateDrawEffect::None,
			AngleInRadians,
			TOptional<FVector2D>(),
			FSlateDrawElement::RelativeToElement,
			Params.WireColor
		);
	}
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnectionDown(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	// Constants which might need fine-tuning
	constexpr float NodeBoxOffset = 20.0f;

	const FVector2D UnitDelta = (End - Start).GetSafeNormal();
	const FVector2D StartOffset = FVector2D(0, NodeBoxOffset * UnitDelta.Y).GetSafeNormal();
	const FVector2D EndOffset = -StartOffset;

	FVector2D AdjustedStart = Start + StartOffset;
	FVector2D AdjustedEnd = End + EndOffset;

	// Calculate the control points for the bezier curve.
	FVector2D ControlPoint1 = AdjustedStart;
	FVector2D ControlPoint2 = FVector2D(End.X, AdjustedEnd.Y + 5.f); // a slight offset for better curve appeal

	// Draw the bezier curve with the control points.
	FSlateDrawElement::MakeCubicBezierSpline(
		DrawElementsList,
		LayerId,
		FPaintGeometry(),
		AdjustedStart,
		ControlPoint1,
		ControlPoint2,
		AdjustedEnd,
		Params.WireThickness,
		ESlateDrawEffect::None,
		Params.WireColor
	);

	// TODO: Add bubbles and midpoint image logic here if needed
}


void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnectionUp(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	// Constants which might need fine-tuning
	constexpr float NodeBoxOffset = 20.0f;
	constexpr float ControlPointDistance = 50.0f;  // The distance the control points are from the start and end, can adjust for a tighter or looser loop

	const FVector2D UnitDelta = (End - Start).GetSafeNormal();
	FVector2D StartOffset = FVector2D(0, NodeBoxOffset * UnitDelta.Y).GetSafeNormal();
	FVector2D EndOffset = -StartOffset;

	FVector2D AdjustedStart = Start + StartOffset;
	FVector2D AdjustedEnd = End + EndOffset;

	// Calculate control points for the cubic bezier spline.
	FVector2D ControlPoint1 = AdjustedStart - FVector2D(0, ControlPointDistance);
	FVector2D ControlPoint2 = AdjustedEnd - FVector2D(0, ControlPointDistance);

	// Further control points to smooth the loop
	FVector2D ControlPointMid1 = (AdjustedStart + ControlPoint1) * 0.5f;
	FVector2D ControlPointMid2 = (AdjustedEnd + ControlPoint2) * 0.5f;

	// Draw the bezier curve with the control points.
	FSlateDrawElement::MakeCubicBezierSpline(
		DrawElementsList,
		LayerId,
		FPaintGeometry(),
		AdjustedStart,
		ControlPoint1,
		ControlPoint2,
		AdjustedEnd,
		Params.WireThickness,
		ESlateDrawEffect::None,
		Params.WireColor
	);

	// [Optional] Add bubbles and midpoint image logic here if needed, as in the original function.
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	FKismetConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
}

/*
void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	const UMounteaDialogueGraphEditorSettings* EditorSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if (EditorSettings == nullptr || EditorSettings->AllowAdvancedWiring() == false)
	{
		FKismetConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
	}
	else
	{
		//TODO: maybe parametrize those values?
		constexpr float HorizontalThreshold = 20.0f;
		constexpr float VerticalThreshold = 20.0f;

		if (FMath::Abs(Start.X - End.X) <= HorizontalThreshold || FMath::Abs(Start.Y - End.Y) <= VerticalThreshold)
		{
			FKismetConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
			return;
		}
		
		constexpr float NodeBoxOffset = 20.0f;

		const FVector2D UnitDelta = (End - Start).GetSafeNormal();

		FVector2D StartOffset = FVector2D(0, NodeBoxOffset * UnitDelta.Y).GetSafeNormal();
		FVector2D EndOffset = -StartOffset;

		FVector2D AdjustedStart = Start + StartOffset;
		FVector2D AdjustedEnd = End + EndOffset;

		FVector2D ControlPoint1;
		FVector2D ControlPoint2;

		// Calculate middle X
		float MiddleX = (AdjustedStart.X + AdjustedEnd.X) / 2;

		// If the child node is above the source node in terms of Y coordinate.
		if (End.Y < Start.Y)
		{
			ControlPoint1 = FVector2D(AdjustedStart.X, AdjustedStart.Y);
			ControlPoint2 = FVector2D(End.X, AdjustedEnd.Y + 5.f);
		}
		else
		{
			ControlPoint1 = FVector2D(MiddleX, AdjustedStart.Y);
			ControlPoint2 = FVector2D(MiddleX, AdjustedEnd.Y + 5.f);
		}

		FVector2D MiddleControlStart = FVector2D((AdjustedStart.X + ControlPoint1.X) / 2, ControlPoint1.Y);
		FVector2D MiddleControlEnd = FVector2D((AdjustedEnd.X + ControlPoint2.X) / 2, ControlPoint2.Y);


		FSlateDrawElement::MakeLines(
			DrawElementsList,
			LayerId,
			FPaintGeometry(),
			TArray<FVector2D>{ AdjustedStart, MiddleControlStart, ControlPoint1, ControlPoint2, MiddleControlEnd, AdjustedEnd },
			ESlateDrawEffect::None,
			Params.WireColor,
			true,   // bAntialias
			Params.WireThickness
		);

	    if (Params.bDrawBubbles || (MidpointImage != nullptr))
	    {
	        // This table maps distance along curve to alpha
	        FInterpCurve<float> SplineReparamTable;
	        const float SplineLength = (End - Start).Size(); // Manhattan connections are straight, so just use the distance

	        // Draw bubbles on the spline
	        if (Params.bDrawBubbles)
	        {
	            const float BubbleSpacing = 64.f * ZoomFactor;
	            const float BubbleSpeed = 192.f * ZoomFactor;
	            const FVector2D BubbleSize = BubbleImage->ImageSize * ZoomFactor * 0.2f * Params.WireThickness;

	            float Time = (FPlatformTime::Seconds() - GStartTime);
	            const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
	            const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
	            for (int32 i = 0; i < NumBubbles; ++i)
	            {
	                const float Distance = ((float)i * BubbleSpacing) + BubbleOffset;
	                if (Distance < SplineLength)
	                {
	                    const float Alpha = Distance / SplineLength; // Since it's a straight line
	                    FVector2D BubblePos = FMath::Lerp(Start, End, Alpha) - (BubbleSize * 0.5f);

	                    FSlateDrawElement::MakeBox(
	                        DrawElementsList,
	                        LayerId,
	                        FPaintGeometry(BubblePos, BubbleSize, ZoomFactor),
	                        BubbleImage,
	                        ESlateDrawEffect::None,
	                        Params.WireColor
	                    );
	                }
	            }
	        }

	        // Draw the midpoint image
	        if (MidpointImage != nullptr)
	        {
	            const FVector2D Midpoint = FMath::Lerp(Start, End, 0.5f); // Midpoint of a straight line
	            const FVector2D MidpointDrawPos = Midpoint - MidpointRadius;

	            FSlateDrawElement::MakeBox(
	                DrawElementsList,
	                LayerId,
	                FPaintGeometry(MidpointDrawPos, MidpointImage->ImageSize * ZoomFactor, ZoomFactor),
	                MidpointImage,
	                ESlateDrawEffect::None,
	                Params.WireColor
	            );
	        }
	    }
	}
}
*/
