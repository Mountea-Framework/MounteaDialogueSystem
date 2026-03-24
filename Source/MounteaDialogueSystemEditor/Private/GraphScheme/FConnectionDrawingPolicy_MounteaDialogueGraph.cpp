// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FConnectionDrawingPolicy_MounteaDialogueGraph.h"

#include "FMDSPathDrawer.h"
#include "SGraphPanel.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"

namespace MDSGraphWireTokens
{
	constexpr float WireThickness       = 2.0f;
	constexpr float WireStubOffset      = 16.0f;
	constexpr float MinDistanceForStyle = 24.0f;
	constexpr float WireGridSize        = 1.0f;
}

namespace MDSGraphWireHelpers
{
	static bool IsDirectNodeToNodeConnection(const FConnectionParams& Params)
	{
		if (!Params.AssociatedPin1 || !Params.AssociatedPin2)
		{
			return false;
		}

		const UEdGraphNode* owningNodeA = Params.AssociatedPin1->GetOwningNode();
		const UEdGraphNode* owningNodeB = Params.AssociatedPin2->GetOwningNode();
		const bool bEdgeNodeConnection = Cast<UEdNode_MounteaDialogueGraphEdge>(owningNodeA) || Cast<UEdNode_MounteaDialogueGraphEdge>(owningNodeB);
		return !bEdgeNodeConnection;
	}
}

FConnectionDrawingPolicy_MounteaDialogueGraph::FConnectionDrawingPolicy_MounteaDialogueGraph
(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
	  , GraphObj(InGraphObj)
{
	ArrowImage = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Graph.SimpleArrow"));
	ArrowRadius = ArrowImage ? ArrowImage->ImageSize * ZoomFactor * 0.5f : FVector2D(0.f);
	MidpointImage = nullptr;
	MidpointRadius = FVector2D::ZeroVector;
	HoverDeemphasisDarkFraction = 0.8f;
	BubbleImage = FAppStyle::GetBrush(TEXT("Graph.Arrow"));
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;
	Params.WireThickness  = MDSGraphWireTokens::WireThickness;
	Params.WireColor      = FMounteaDialogueGraphVisualTokens::GetWireColor();
	Params.bUserFlag1     = false;

	if (InputPin)
	{
		if (UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(InputPin->GetOwningNode()))
		{
			if (SelectedEdgeNodes.Contains(edgeNode))
			{
				Params.WireColor     = FMounteaDialogueGraphVisualTokens::GetPrimaryAccent();
				Params.WireThickness = MDSGraphWireTokens::WireThickness * 2.0f;
				Params.bUserFlag1    = true;
			}
		}
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
		ApplyHoverDeemphasis(OutputPin, InputPin, Params.WireThickness, Params.WireColor);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	NodeWidgetMap.Empty();
	SelectedEdgeNodes.Empty();

	for (int32 nodeIndex = 0; nodeIndex < ArrangedNodes.Num(); ++nodeIndex)
	{
		TSharedRef<SGraphNode> childNode = StaticCastSharedRef<SGraphNode>(ArrangedNodes[nodeIndex].Widget);
		NodeWidgetMap.Add(childNode->GetNodeObj(), nodeIndex);

		if (UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(childNode->GetNodeObj()))
		{
			const TSharedPtr<SGraphPanel> panel = childNode->GetOwnerPanel();
			if (panel.IsValid() && panel->SelectionManager.IsNodeSelected(edgeNode))
				SelectedEdgeNodes.Add(edgeNode);
		}
	}

	if (ArrangedNodes.Num() > 0)
	{
		TSharedRef<SGraphNode> firstNode = StaticCastSharedRef<SGraphNode>(ArrangedNodes[0].Widget);
		CachedGraphPanel = firstNode->GetOwnerPanel();
	}

	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	const FVector2D startCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D endCenter   = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D seedPoint   = (startCenter + endCenter) * 0.5f;
	const FVector2D startPoint  = FGeometryHelper::FindClosestPointOnGeom(StartGeom, seedPoint);
	const FVector2D endPoint    = FGeometryHelper::FindClosestPointOnGeom(EndGeom, seedPoint);

	DrawSubwayWireWithArrow(startPoint, endPoint, Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	DrawSubwayWireWithArrow(StartPoint, EndPoint, Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams previewParams;
	previewParams.WireColor     = FMounteaDialogueGraphVisualTokens::GetWireColor();
	previewParams.WireThickness = MDSGraphWireTokens::WireThickness;
	previewParams.bDrawBubbles  = false;
	DrawSubwayWireWithArrow(StartPoint, EndPoint, previewParams);
}

FVector2D FConnectionDrawingPolicy_MounteaDialogueGraph::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	return FVector2D(0.0f, MDSGraphWireTokens::WireStubOffset * ZoomFactor);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget, UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FArrangedWidget*& StartWidgetGeometry, FArrangedWidget*& EndWidgetGeometry)
{
	StartWidgetGeometry = nullptr;
	EndWidgetGeometry = nullptr;

	const auto resolveGeometryFromPin = [this](const UEdGraphPin* TargetPin) -> FArrangedWidget*
	{
		if (!TargetPin || !PinGeometries)
			return nullptr;

		if (TSharedPtr<SGraphPin>* targetWidget = PinToPinWidgetMap.Find(TargetPin))
		{
			TSharedRef<SGraphPin> pinWidget = (*targetWidget).ToSharedRef();
			return PinGeometries->Find(pinWidget);
		}

		return nullptr;
	};

	const auto resolveGeometryFromNode = [this, &ArrangedNodes](const UEdGraphNode* TargetNode) -> FArrangedWidget*
	{
		if (!TargetNode)
			return nullptr;

		if (int32* nodeIndex = NodeWidgetMap.Find(TargetNode))
			return &(ArrangedNodes[*nodeIndex]);

		return nullptr;
	};

	FString geometryPath = TEXT("missing");
	UEdNode_MounteaDialogueGraphEdge* edgeInputOwner = InputPin ? Cast<UEdNode_MounteaDialogueGraphEdge>(InputPin->GetOwningNode()) : nullptr;
	if (edgeInputOwner)
	{
		UEdNode_MounteaDialogueGraphNode* startNode = edgeInputOwner->GetStartNode();
		UEdNode_MounteaDialogueGraphNode* endNode = edgeInputOwner->GetEndNode();
		UEdGraphPin* startConnectorPin = startNode ? startNode->GetOutputPin() : nullptr;
		UEdGraphPin* endConnectorPin = endNode ? endNode->GetInputPin() : nullptr;

		StartWidgetGeometry = resolveGeometryFromPin(startConnectorPin);
		EndWidgetGeometry = resolveGeometryFromPin(endConnectorPin);

		if (StartWidgetGeometry && EndWidgetGeometry)
			geometryPath = TEXT("pin");
		else
		{
			StartWidgetGeometry = resolveGeometryFromNode(startNode);
			EndWidgetGeometry = resolveGeometryFromNode(endNode);
			if (StartWidgetGeometry && EndWidgetGeometry)
				geometryPath = TEXT("node-fallback");
		}
	}
	else
	{
		StartWidgetGeometry = PinGeometries ? PinGeometries->Find(OutputPinWidget) : nullptr;
		EndWidgetGeometry = resolveGeometryFromPin(InputPin);
		if (StartWidgetGeometry && EndWidgetGeometry)
			geometryPath = TEXT("pin");
	}

	if (!StartWidgetGeometry || !EndWidgetGeometry)
	{
		const FString outputNodeName = OutputPin && OutputPin->GetOwningNode() ? OutputPin->GetOwningNode()->GetName() : TEXT("None");
		const FString inputNodeName = InputPin && InputPin->GetOwningNode() ? InputPin->GetOwningNode()->GetName() : TEXT("None");

		static double lastMissingGeometryLogTime = 0.0;
		const double now = FPlatformTime::Seconds();
		if ((now - lastMissingGeometryLogTime) > 0.25)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[DetermineLinkGeometry] Missing geometry (Path=%s, OutputNode=%s, InputNode=%s)."), *geometryPath, *outputNodeName, *inputNodeName);
			lastMissingGeometryLogTime = now;
		}
	}
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSubwayWireWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	const float stubOffset = MDSGraphWireTokens::WireStubOffset * ZoomFactor;
	const float gridStep   = MDSGraphWireTokens::WireGridSize   * ZoomFactor;

	auto snapToGrid = [gridStep](const FVector2D& V) -> FVector2D
	{
		return FVector2D(
			FMath::RoundToFloat(V.X / gridStep) * gridStep,
			FMath::RoundToFloat(V.Y / gridStep) * gridStep
		);
	};

	FVector2D wireStart = snapToGrid(StartPoint + FVector2D(0.0f,  stubOffset));
	FVector2D wireEnd   = snapToGrid(EndPoint   - FVector2D(0.0f,  stubOffset));

	FSlateDrawElement::MakeDrawSpaceSpline(DrawElementsList, WireLayerID,
		StartPoint, FVector2D::ZeroVector, wireStart, FVector2D::ZeroVector,
		Params.WireThickness, ESlateDrawEffect::None, Params.WireColor);

	FSlateDrawElement::MakeDrawSpaceSpline(DrawElementsList, WireLayerID,
		wireEnd, FVector2D::ZeroVector, EndPoint, FVector2D::ZeroVector,
		Params.WireThickness, ESlateDrawEffect::None, Params.WireColor);

	FMDSPathDrawer drawer(WireLayerID, ZoomFactor, DrawElementsList, Params);

	const float dist = FVector2D::Distance(wireStart, wireEnd);
	if (dist < MDSGraphWireTokens::MinDistanceForStyle * ZoomFactor)
	{
		FSlateDrawElement::MakeDrawSpaceSpline(DrawElementsList, WireLayerID,
			wireStart, FVector2D::ZeroVector, wireEnd, FVector2D::ZeroVector,
			Params.WireThickness, ESlateDrawEffect::None, Params.WireColor);
	}
	else
	{
		drawer.DrawSubwayWire(wireStart, FVector2D(0.0f, 1.0f), wireEnd, FVector2D(0.0f, 1.0f));
	}

	if (ArrowImage)
	{
		const FVector2D arrowDirection    = FVector2D(0.0f, 1.0f);
		const FVector2D arrowSize         = ArrowImage->ImageSize * ZoomFactor;
		const FVector2D arrowCenter       = EndPoint - (arrowDirection * (arrowSize.X * 0.50f));
		const FVector2D arrowDrawPosition = arrowCenter - (arrowSize * 0.5f);
		const float arrowAngle            = FMath::Atan2(arrowDirection.Y, arrowDirection.X);

		FSlateDrawElement::MakeRotatedBox(
			DrawElementsList,
			ArrowLayerID,
			FPaintGeometry(arrowDrawPosition, arrowSize, 1.0f),
			ArrowImage,
			ESlateDrawEffect::None,
			arrowAngle,
			TOptional<FVector2D>(),
			FSlateDrawElement::RelativeToElement,
			Params.WireColor
		);
	}

	if (Params.bUserFlag1)
	{
		const float time      = static_cast<float>(FSlateApplication::Get().GetCurrentTime());
		const float speed     = 0.6f;
		const int32 dotCount  = 4;
		const float dotRadius = FMath::Clamp(5.0f * ZoomFactor, 2.0f, 8.0f);
		const FVector2D dotSize(dotRadius * 2.0f, dotRadius * 2.0f);
		const FSlateBrush* dotBrush = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Icon.BulletPoint"));

		const TArray<TPair<FVector2D, FVector2D>> dotSegments = {
			{ StartPoint, wireStart },
			{ wireEnd,    EndPoint  }
		};

		for (int32 i = 0; i < dotCount; ++i)
		{
			const float t         = FMath::Frac(time * speed + static_cast<float>(i) / static_cast<float>(dotCount));
			const FVector2D dotCenter = FMath::Lerp(StartPoint, EndPoint, t);

			FSlateDrawElement::MakeBox(
				DrawElementsList,
				ArrowLayerID,
				FPaintGeometry(dotCenter - dotSize * 0.5f, dotSize, 1.0f),
				dotBrush,
				ESlateDrawEffect::None,
				Params.WireColor
			);
		}
	}

	if (Params.AssociatedPin2)
	{
		if (UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(Params.AssociatedPin2->GetOwningNode()))
		{
			FVector2D screenMid = drawer.GetArcLengthMidpoint();
			if (screenMid.IsZero())
				screenMid = (wireStart + wireEnd) * 0.5f;

			const TSharedPtr<SGraphPanel> panel = CachedGraphPanel.Pin();
			if (panel.IsValid())
			{
				const FVector2D localMid = screenMid - FVector2D(panel->GetCachedGeometry().AbsolutePosition);
				const FVector2D graphMid = panel->PanelCoordToGraphCoord(localMid);
				edgeNode->CachedSplineMidpointGraph = graphMid;
				edgeNode->bHasCachedSplineMidpoint  = true;
				edgeNode->NodePosX = graphMid.X - 19.0f;
				edgeNode->NodePosY = graphMid.Y - 19.0f;
			}
		}
	}
}
