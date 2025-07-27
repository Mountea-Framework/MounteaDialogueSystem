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
	const FVector2f StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2f EndCenter = FGeometryHelper::CenterOf(EndGeom);

	DrawSplineWithArrow(StartCenter, EndCenter, Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FVector2f& StartPoint, const FVector2f& EndPoint, const FConnectionParams& Params)
{
	// bUserFlag1 indicates that we need to reverse the direction of connection (used by debugger)
	const FVector2f& P0 = Params.bUserFlag1 ? EndPoint : StartPoint;
	const FVector2f& P1 = Params.bUserFlag1 ? StartPoint : EndPoint;
	
	FConnectionParams NewParams = Params;
	
	if (const UMounteaDialogueGraphEditorSettings* MounteaDialogueGraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
		NewParams.WireThickness = MounteaDialogueGraphEditorSettings->GetWireWidth();
	
	Internal_DrawLineWithArrow(P0, P1, NewParams);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2f& StartPoint, const FVector2f& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams Params;
	DetermineWiringStyle(Pin, nullptr, /*inout*/ Params);

	if (Pin->Direction == EGPD_Output)
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, EndPoint), EndPoint, Params);
	else
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, StartPoint), StartPoint, Params);
}

FVector2f FConnectionDrawingPolicy_MounteaDialogueGraph::ComputeSplineTangent(const FVector2f& Start, const FVector2f& End) const
{
	const FVector2f Delta = End - Start;
	const FVector2f NormDelta = Delta.GetSafeNormal();

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

void FConnectionDrawingPolicy_MounteaDialogueGraph::Internal_DrawLineWithArrow(const FVector2f& StartAnchorPoint, const FVector2f& EndAnchorPoint, const FConnectionParams& Params)
{
	const FVector2f DeltaPos = EndAnchorPoint - StartAnchorPoint;
	const FVector2f UnitDelta = DeltaPos.GetSafeNormal();

	const FVector2f StartPoint = StartAnchorPoint;
	const FVector2f EndPoint = EndAnchorPoint - (ArrowRadius.X * UnitDelta);

	const float nodesDelta = abs(StartPoint.X - EndPoint.X);
	
	float OffsetValue = 0.f;
	const UMounteaDialogueGraphEditorSettings* GraphSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if (GraphSettings == nullptr || GraphSettings->AllowAdvancedWiring() == false || nodesDelta <= GraphSettings->GetControlPointDistance() * ZoomFactor)
	{
		DrawConnection(WireLayerID, StartPoint, EndPoint, Params);
	}
	else
	{
		OffsetValue = -3.f;
		const FVector2f ConnectionEndPoint = FVector2f(EndPoint.X, EndPoint.Y + OffsetValue);
		DrawCurvedConnection(WireLayerID, StartPoint, ConnectionEndPoint, Params);
		
	}

	// Draw the arrow
	if (ArrowImage)
	{
		FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
		ArrowDrawPos.Y += OffsetValue;
		const float AngleInRadians = FMath::DegreesToRadians(90.f);

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

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawCurvedConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params)
{
	const UMounteaDialogueGraphEditorSettings* GraphSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();

	FVector2f Tangent = GraphSettings->GetAdvancedWiringConnectionTangent().GetAbs();

	const int32 SideValue = (End.X > Start.X) ? 1 : -1;

	Tangent.X *= SideValue;
	Tangent.Y *= 0.5f;
	
	FVector2f ControlPoint1 = Start + (Tangent * ZoomFactor);
	FVector2f ControlPoint2 = End - (Tangent * ZoomFactor);
	
	ControlPoint1 = FMath::Lerp(Start, ControlPoint1, 0.6f);
	ControlPoint2 = FMath::Lerp(End, ControlPoint2, 0.6f);
	
	const FVector2D connectionEnd = FVector2D(End.X, End.Y + 5.f);

	FSlateDrawElement::MakeCubicBezierSpline(
		DrawElementsList,
		LayerId,
		FPaintGeometry(),
		Start,
		ControlPoint1,
		ControlPoint2,
		connectionEnd,
		Params.WireThickness,
		ESlateDrawEffect::None,
		Params.WireColor
	);
}


void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params)
{
	FKismetConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
}
