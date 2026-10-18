// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FConnectionDrawingPolicy_MounteaDialogueGraph.h"

#include "MDSBoundedPath.h"
#include "SGraphPanel.h"
#include "Consts/MounteaDialogueEditorConsts.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"


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
	Params.WireThickness  = MounteaDialogueWireConsts::WireThickness;
	Params.WireColor      = FMounteaDialogueGraphVisualTokens::GetWireColor();
	Params.bUserFlag1     = false;

	if (InputPin)
	{
		if (UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(InputPin->GetOwningNode()))
		{
			if (SelectedEdgeNodes.Contains(edgeNode))
			{
				Params.WireColor     = FMounteaDialogueGraphVisualTokens::GetPrimaryAccent();
				Params.WireThickness = MounteaDialogueWireConsts::WireThickness * 2.0f;
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

	CachedArrangedNodes = &ArrangedNodes;

	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	if (TryDrawPreparedRoute(StartGeom, Params))
		return;

	// Culled nodes arrive with a synthesized, zero-size geometry (see ResolveCulledAnchors).
	const bool bSynthesized = StartGeom.GetLocalSize().IsNearlyZero() || EndGeom.GetLocalSize().IsNearlyZero();
	if (bSynthesized)
	{
		FVector2f culledStart, culledEnd;
		if (ResolveCulledAnchors(Params, culledStart, culledEnd))
			DrawSubwayWireWithArrow(culledStart, culledEnd, Params);
		return;
	}

	// Resolve exact top/bottom centers directly in absolute space.
	// This avoids extreme-value closest-point queries that introduce visible X drift at deep zoom-out.
	const FVector2f startPoint = StartGeom.LocalToAbsolute(FVector2f(StartGeom.GetLocalSize().X * 0.5f, StartGeom.GetLocalSize().Y));
	const FVector2f endPoint   = EndGeom.LocalToAbsolute(FVector2f(EndGeom.GetLocalSize().X * 0.5f, 0.0f));

	DrawSubwayWireWithArrow(startPoint, endPoint, Params);
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
	if (UEdNode_MounteaDialogueGraphEdge* edgeInputOwner = InputPin ? Cast<UEdNode_MounteaDialogueGraphEdge>(InputPin->GetOwningNode()) : nullptr)
	{
		UEdNode_MounteaDialogueGraphNode* startNode = edgeInputOwner->GetStartNode();
		UEdNode_MounteaDialogueGraphNode* endNode = edgeInputOwner->GetEndNode();

		StartWidgetGeometry = resolveGeometryFromNode(startNode);
		EndWidgetGeometry = resolveGeometryFromNode(endNode);
		if (StartWidgetGeometry && EndWidgetGeometry)
			geometryPath = TEXT("node");
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
			EditorLOG_WARNING(TEXT("[DetermineLinkGeometry] Missing geometry (Path=%s, OutputNode=%s, InputNode=%s)."), *geometryPath, *outputNodeName, *inputNodeName);
			lastMissingGeometryLogTime = now;
		}
	}
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSubwayWireWithArrow(const FVector2f& InStartPoint, const FVector2f& InEndPoint, const FConnectionParams& Params)
{
	// Slate hands us floats; everything below is double so extreme coordinates are clipped before any float conversion.
	const FVector2D StartPoint(InStartPoint);
	const FVector2D EndPoint(InEndPoint);
	const double stubOffset = MounteaDialogueWireConsts::WireStubOffset * ZoomFactor;

	const FVector2D wireStart = StartPoint + FVector2D(0.0, stubOffset);
	const FVector2D wireEnd   = EndPoint   - FVector2D(0.0, stubOffset);

	TArray<FVector2D> corners;
	corners.Add(StartPoint);
	corners.Add(wireStart);
	if (wireEnd.Y < wireStart.Y)
	{
		// Backward link: leave through an exterior lane beside both nodes.
		const double laneX = FMath::Max(wireStart.X, wireEnd.X) + stubOffset;
		corners.Add(FVector2D(laneX, wireStart.Y));
		corners.Add(FVector2D(laneX, wireEnd.Y));
	}
	else
	{
		const double midY = (wireStart.Y + wireEnd.Y) * 0.5;
		corners.Add(FVector2D(wireStart.X, midY));
		corners.Add(FVector2D(wireEnd.X, midY));
	}
	corners.Add(wireEnd);
	corners.Add(EndPoint);

	DrawWirePath(corners, Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawWirePath(TArray<FVector2D>& Corners, const FConnectionParams& Params)
{
	if (!MDSBoundedPath::Sanitize(Corners))
	{
		static double lastInvalidLogTime = 0.0;
		const double now = FPlatformTime::Seconds();
		if ((now - lastInvalidLogTime) > 1.0)
		{
			EditorLOG_WARNING(TEXT("[DrawWirePath] Invalid wire geometry (%d point(s)). Wire skipped."), Corners.Num());
			lastInvalidLogTime = now;
		}
		return;
	}

	const FVector2D endPoint = Corners.Last();
	const double radius = FMath::Max(MounteaDialogueWireConsts::RoundRadius * ZoomFactor, 1.0f);

	TArray<FVector2D> path;
	MDSBoundedPath::RoundCorners(Corners, radius, path);

	const FBox2D clipBox(FVector2D(ClippingRect.Left, ClippingRect.Top), FVector2D(ClippingRect.Right, ClippingRect.Bottom));
	MDSBoundedPath::Draw(DrawElementsList, WireLayerID, path, clipBox, Params.WireThickness, Params.WireColor);

	// Arrow direction and animation distance come from the displayed path.
	const double pathLength = MDSBoundedPath::Length(path);
	FVector2D pathEndDirection(0.0, 1.0);
	MDSBoundedPath::SampleAt(path, pathLength, pathEndDirection);

	if (ArrowImage)
	{
		const FVector2D arrowDirection    = pathEndDirection;
		const FVector2D rawArrowSize      = ArrowImage->ImageSize * ZoomFactor;
		const FVector2D arrowSize(
			FMath::Max(rawArrowSize.X, 4.0),
			FMath::Max(rawArrowSize.Y, 4.0)
		);
		const FVector2D arrowCenter       = endPoint - (arrowDirection * (arrowSize.X * 0.50f));
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

		for (int32 i = 0; i < dotCount; ++i)
		{
			const float t = FMath::Frac(time * speed + static_cast<float>(i) / static_cast<float>(dotCount));
			FVector2D dotDirection;
			const FVector2D dotCenter = MDSBoundedPath::SampleAt(path, pathLength * t, dotDirection);

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
}

bool FConnectionDrawingPolicy_MounteaDialogueGraph::TryDrawPreparedRoute(const FGeometry& StartGeom, const FConnectionParams& Params)
{
	// Connections are two links (node -> control, control -> node); the route covers the whole connection.
	const UEdNode_MounteaDialogueGraphEdge* edgeNode = Params.AssociatedPin2 ? Cast<UEdNode_MounteaDialogueGraphEdge>(Params.AssociatedPin2->GetOwningNode()) : nullptr;
	bool bControlToNodeLink = false;
	if (!edgeNode && Params.AssociatedPin1)
	{
		edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(Params.AssociatedPin1->GetOwningNode());
		bControlToNodeLink = edgeNode != nullptr;
	}

	const UEdGraph_MounteaDialogueGraph* dialogueGraph = Cast<UEdGraph_MounteaDialogueGraph>(GraphObj);
	const FPreparedDialogueRoute* route = dialogueGraph ? dialogueGraph->FindPreparedRoute(edgeNode) : nullptr;
	if (!route)
		return false;
	if (bControlToNodeLink)
		return true;

	// Graph -> screen with the panel's real zoom. The geometry's own scale cannot be used: for a culled source node
	// Slate synthesizes a scale of 1, which drew whole routes un-zoomed. Its absolute top-left is correct in both cases.
	TArray<FVector2D> corners;
	corners.Reserve(route->Points.Num());
	const FVector2D sourceOrigin(route->SourcePosition.X, route->SourcePosition.Y);
	const FVector2D screenOrigin(StartGeom.LocalToAbsolute(FVector2f::ZeroVector));
	for (const FVector2D& point : route->Points)
		corners.Add(screenOrigin + (point - sourceOrigin) * static_cast<double>(ZoomFactor));

	DrawWirePath(corners, Params);
	return true;
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FVector2f& StartPoint, const FVector2f& EndPoint, const FConnectionParams& Params)
{
	DrawSubwayWireWithArrow(StartPoint, EndPoint, Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params)
{
	DrawSubwayWireWithArrow(Start, End, Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2f& StartPoint, const FVector2f& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams previewParams;
	previewParams.WireColor     = FMounteaDialogueGraphVisualTokens::GetWireColor();
	previewParams.WireThickness = MounteaDialogueWireConsts::WireThickness;
	previewParams.bDrawBubbles  = false;
	DrawSubwayWireWithArrow(StartPoint, EndPoint, previewParams);
}

FVector2f FConnectionDrawingPolicy_MounteaDialogueGraph::ComputeSplineTangent(const FVector2f& Start, const FVector2f& End) const
{
	return FVector2f(0.0f, MounteaDialogueWireConsts::WireStubOffset * ZoomFactor);
}

#if UE_VERSION_OLDER_THAN(5, 7, 0)
void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	DrawSplineWithArrow(FVector2f(StartPoint), FVector2f(EndPoint), Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	DrawConnection(LayerId, FVector2f(Start), FVector2f(End), Params);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	DrawPreviewConnector(PinGeometry, FVector2f(StartPoint), FVector2f(EndPoint), Pin);
}

FVector2D FConnectionDrawingPolicy_MounteaDialogueGraph::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	return FVector2D(ComputeSplineTangent(FVector2f(Start), FVector2f(End)));
}
#endif

bool FConnectionDrawingPolicy_MounteaDialogueGraph::ResolveCulledAnchors(const FConnectionParams& Params, FVector2f& OutStart, FVector2f& OutEnd) const
{
	// Only the node -> control link is a real node-to-node connection; the control -> node stub is dropped.
	const UEdNode_MounteaDialogueGraphEdge* edgeNode = Params.AssociatedPin2 ? Cast<UEdNode_MounteaDialogueGraphEdge>(Params.AssociatedPin2->GetOwningNode()) : nullptr;
	const UEdNode_MounteaDialogueGraphNode* startNode = Params.AssociatedPin1 ? Cast<UEdNode_MounteaDialogueGraphNode>(Params.AssociatedPin1->GetOwningNode()) : nullptr;
	const UEdNode_MounteaDialogueGraphNode* endNode = edgeNode ? const_cast<UEdNode_MounteaDialogueGraphEdge*>(edgeNode)->GetEndNode() : nullptr;
	if (!startNode || !endNode || !CachedArrangedNodes)
		return false;

	const auto topLeftAndSize = [this](const UEdNode_MounteaDialogueGraphNode* Node, FVector2f& OutTopLeft, FVector2f& OutSize) -> bool
	{
		const int32* index = NodeWidgetMap.Find(Node);
		if (!index)
			return false;

		const FGeometry& geometry = (*CachedArrangedNodes)[*index].Geometry;
		OutTopLeft = geometry.LocalToAbsolute(FVector2f::ZeroVector);
		const FVector2f local = geometry.GetLocalSize();
		const FVector2f size = local.IsNearlyZero() ? FVector2f(UEdGraph_MounteaDialogueGraph::MeasureNode(Node)) : local;
		OutSize = size * ZoomFactor;
		return true;
	};

	FVector2f startTopLeft, startSize, endTopLeft, endSize;
	if (!topLeftAndSize(startNode, startTopLeft, startSize) || !topLeftAndSize(endNode, endTopLeft, endSize))
		return false;

	OutStart = FVector2f(startTopLeft.X + startSize.X * 0.5f, startTopLeft.Y + startSize.Y);
	OutEnd = FVector2f(endTopLeft.X + endSize.X * 0.5f, endTopLeft.Y);
	return true;
}

bool FConnectionDrawingPolicy_MounteaDialogueGraph::IsConnectionCulled(const FArrangedWidget& StartLink, const FArrangedWidget& EndLink) const
{
	// A routed wire can cross the viewport while both of its (zero-size, synthesized) endpoints are off to one side.
	// DrawWirePath clips to the viewport, so skipping the endpoint test costs nothing visible.
	const UEdGraph_MounteaDialogueGraph* dialogueGraph = Cast<UEdGraph_MounteaDialogueGraph>(GraphObj);
	if (dialogueGraph && dialogueGraph->HasPreparedRoutes())
		return false;

	return FKismetConnectionDrawingPolicy::IsConnectionCulled(StartLink, EndLink);
}
