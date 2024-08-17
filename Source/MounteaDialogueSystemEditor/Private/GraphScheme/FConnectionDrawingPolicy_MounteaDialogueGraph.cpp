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
	if (const UMounteaDialogueGraphEditorSettings* GraphEditorSettings = GetDefault<UMounteaDialogueGraphEditorSettings>())
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

	BubbleImage = FAppStyle::GetBrush( TEXT("Graph.Arrow") );
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
	
	if (const UMounteaDialogueGraphEditorSettings* MounteaDialogueGraphEditorSettings = GetDefault<UMounteaDialogueGraphEditorSettings>())
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
		
	const UMounteaDialogueGraphEditorSettings* GraphSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if (GraphSettings == nullptr || GraphSettings->AllowAdvancedWiring() == false)
	{
		DrawConnection(WireLayerID, StartPoint, EndPoint, Params);
		return;
	}

	const bool bIsAligned = FMath::IsNearlyEqual(EndPoint.X, StartPoint.X, GraphSettings->GetControlPointDistance());
		
	// Choose connection drawing method based on conditions
	if (bIsAligned)
	{
		DrawConnection(WireLayerID, StartPoint, EndPoint, Params);
	}
	else
	{
		DrawCurvedConnection(WireLayerID, StartPoint, EndPoint, Params);
	}

	// Draw the arrow
	if (ArrowImage)
	{
		const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
		
		const float AngleInRadians = FMath::DegreesToRadians(90.f); //FMath::Atan2(DeltaPos.Y, DeltaPos.X);

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

/*
void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnectionDown(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	// Constants for Bezier control points
	FVector2D ControlPoint1 = FVector2D(Start.X, Start.Y + (End.Y - Start.Y) / 3); // Adjust to create a smooth curve
	FVector2D ControlPoint2 = FVector2D(End.X, End.Y - (End.Y - Start.Y) / 3);

	FSlateDrawElement::MakeCubicBezierSpline(
		DrawElementsList,
		LayerId,
		FPaintGeometry(),
		Start,
		ControlPoint1,
		ControlPoint2,
		End,
		Params.WireThickness,
		ESlateDrawEffect::None,
		Params.WireColor
	);
}
*/

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawCurvedConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	const UMounteaDialogueGraphEditorSettings* GraphSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();

	FVector2D LeaveTangent = GraphSettings->GetAdvancedWiringConnectionTangent().GetAbs();
	FVector2D ArriveTangent = GraphSettings->GetAdvancedWiringConnectionTangent().GetAbs();
	
	const int32 SideValue = End.X >= 0 ? 1 : -1;

	LeaveTangent.X *= -SideValue;
	ArriveTangent.X *= SideValue;

	LeaveTangent.Y *= SideValue;
	ArriveTangent.Y *= -SideValue;
	
	// Control points derived from the tangents
	FVector2D ControlPoint1 = Start + (LeaveTangent * ZoomFactor);
	FVector2D ControlPoint2 = End + (ArriveTangent * ZoomFactor);

	FSlateDrawElement::MakeCubicBezierSpline(
		DrawElementsList,
		LayerId,
		FPaintGeometry(),
		Start,
		ControlPoint1,
		ControlPoint2,
		End,
		Params.WireThickness,
		ESlateDrawEffect::None,
		Params.WireColor
	);
}


void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	FKismetConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
}
