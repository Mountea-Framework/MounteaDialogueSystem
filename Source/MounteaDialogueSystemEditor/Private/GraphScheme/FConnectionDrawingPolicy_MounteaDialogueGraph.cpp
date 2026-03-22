// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FConnectionDrawingPolicy_MounteaDialogueGraph.h"

#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Rendering/DrawElements.h"

namespace MDSGraphWireTokens
{
	constexpr float WireThickness = 2.0f;
	constexpr float ConnectorVisualRadius = 0.1f;
	constexpr float EdgeBubbleRadius = 16.0f;
	constexpr float EdgeBubbleIconSize = 16.0f;
	constexpr float MinimumControlDistance = 42.0f;
	constexpr float MaximumControlDistance = 220.0f;
	constexpr float HorizontalControlFactor = 0.22f;
	constexpr float VerticalControlFactor = 0.56f;
	constexpr float LowVerticalThreshold = 36.0f;
	constexpr float FlatCurveControlFactor = 0.45f;
	// ZoomFactor threshold — approximate mapping: Zoom-4≈0.5, Zoom-5≈0.375, Zoom-6≈0.25
	constexpr float FlatLineZoomThreshold = 0.28f;  // Zoom -6 and beyond: draw straight lines
}

namespace MDSGraphWireHelpers
{
	static FVector2D EvaluateCubicBezierPoint(const FVector2D& StartPoint, const FVector2D& ControlPointA, const FVector2D& ControlPointB, const FVector2D& EndPoint, const float T)
	{
		const float oneMinusT = 1.0f - T;
		const float oneMinusTSquared = oneMinusT * oneMinusT;
		const float oneMinusTCubed = oneMinusTSquared * oneMinusT;
		const float tSquared = T * T;
		const float tCubed = tSquared * T;

		return (oneMinusTCubed * StartPoint) +
			(3.0f * oneMinusTSquared * T * ControlPointA) +
			(3.0f * oneMinusT * tSquared * ControlPointB) +
			(tCubed * EndPoint);
	}

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

	BubbleImage = FAppStyle::GetBrush( TEXT("Graph.Arrow") );
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;
	Params.WireThickness = MDSGraphWireTokens::WireThickness;
	Params.WireColor = FMounteaDialogueGraphVisualTokens::GetWireColor();

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
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
	const FVector2D startCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D endCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D seedPoint = (startCenter + endCenter) * 0.5f;
	const FVector2D startPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, seedPoint);
	const FVector2D endPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, seedPoint);

	FConnectionParams adjustedParams = Params;
	adjustedParams.WireThickness = MDSGraphWireTokens::WireThickness;
	DrawBezierSplineWithArrow(startPoint, endPoint, adjustedParams);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	FConnectionParams adjustedParams = Params;
	adjustedParams.WireThickness = MDSGraphWireTokens::WireThickness;
	DrawBezierSplineWithArrow(StartPoint, EndPoint, adjustedParams);
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams previewParams;
	previewParams.WireColor = FMounteaDialogueGraphVisualTokens::GetWireColor();
	previewParams.WireThickness = MDSGraphWireTokens::WireThickness;
	previewParams.bDrawBubbles = false;
	DrawBezierSplineWithArrow(StartPoint, EndPoint, previewParams);
}

FVector2D FConnectionDrawingPolicy_MounteaDialogueGraph::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	FVector2D controlPointA;
	FVector2D controlPointB;
	CalculateBezierControlPoints(Start, End, ZoomFactor, controlPointA, controlPointB);
	return controlPointA - Start;
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget, UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FArrangedWidget*& StartWidgetGeometry, FArrangedWidget*& EndWidgetGeometry)
{
	StartWidgetGeometry = nullptr;
	EndWidgetGeometry = nullptr;

	const auto resolveGeometryFromPin = [this](UEdGraphPin* TargetPin) -> FArrangedWidget*
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

	const auto resolveGeometryFromNode = [this, &ArrangedNodes](UEdGraphNode* TargetNode) -> FArrangedWidget*
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

void FConnectionDrawingPolicy_MounteaDialogueGraph::DrawBezierSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params)
{
	FVector2D controlPointA;
	FVector2D controlPointB;
	CalculateBezierControlPoints(StartPoint, EndPoint, ZoomFactor, controlPointA, controlPointB);

	FVector2D arrowDirection = (EndPoint - controlPointB).GetSafeNormal();
	if (arrowDirection.IsNearlyZero())
		arrowDirection = FVector2D(0.0f, 1.0f);

	const float connectorInset = MDSGraphWireTokens::ConnectorVisualRadius * ZoomFactor;
	const FVector2D targetTipPoint = EndPoint - (arrowDirection * connectorInset);

	const float rawArrowInset = ArrowImage ? (ArrowImage->ImageSize.X * ZoomFactor * 0.50f) : 8.0f;
	const float maxArrowInset = FVector2D::Distance(StartPoint, EndPoint) * 0.45f;
	const float arrowInset = FMath::Min(rawArrowInset, maxArrowInset);
	const FVector2D splineEnd = targetTipPoint;
	const FVector2D adjustedControlPointB = controlPointB - (arrowDirection * (arrowInset * 0.15f));

	FSlateDrawElement::MakeCubicBezierSpline(
		DrawElementsList,
		WireLayerID,
		FPaintGeometry(),
		StartPoint,
		controlPointA,
		adjustedControlPointB,
		splineEnd,
		Params.WireThickness,
		ESlateDrawEffect::None,
		Params.WireColor
	);

	if (!ArrowImage)
		return;

	const FVector2D arrowSize = ArrowImage->ImageSize * ZoomFactor;
	const FVector2D arrowCenter = targetTipPoint - (arrowDirection * (arrowSize.X * 0.50f));
	const FVector2D arrowDrawPosition = arrowCenter - (arrowSize * 0.5f);
	const float arrowAngle = FMath::Atan2(arrowDirection.Y, arrowDirection.X);

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

	return;
}

void FConnectionDrawingPolicy_MounteaDialogueGraph::CalculateBezierControlPoints(const FVector2D& StartPoint, const FVector2D& EndPoint, float InZoomFactor, FVector2D& OutControlPointA, FVector2D& OutControlPointB)
{
	const FVector2D delta = EndPoint - StartPoint;
	const float absDeltaX = FMath::Abs(delta.X);
	const float absDeltaY = FMath::Abs(delta.Y);
	const float signX = (delta.X >= 0.0f) ? 1.0f : -1.0f;
	const float signY = (delta.Y >= 0.0f) ? 1.0f : -1.0f;

	// Zoom -6 and beyond: straight lines avoid the branch-switching inconsistency that
	// comes from the fixed LowVerticalThreshold pixel value changing meaning at extreme zoom.
	if (InZoomFactor < MDSGraphWireTokens::FlatLineZoomThreshold)
	{
		OutControlPointA = StartPoint + delta * (1.0f / 3.0f);
		OutControlPointB = StartPoint + delta * (2.0f / 3.0f);
		return;
	}

	const float totalDist = delta.Size();
	const float effectiveMin = FMath::Min(MDSGraphWireTokens::MinimumControlDistance, totalDist * 0.45f);
	const float effectiveMax = MDSGraphWireTokens::MaximumControlDistance;

	const bool isMostlyHorizontal = absDeltaY < MDSGraphWireTokens::LowVerticalThreshold && absDeltaX > MDSGraphWireTokens::LowVerticalThreshold;
	if (isMostlyHorizontal)
	{
		const float flatControlDistance = FMath::Clamp(absDeltaX * MDSGraphWireTokens::FlatCurveControlFactor, effectiveMin, effectiveMax);
		OutControlPointA = StartPoint + FVector2D(signX * flatControlDistance, 0.0f);
		OutControlPointB = EndPoint - FVector2D(signX * flatControlDistance, 0.0f);
		return;
	}

	const float verticalControlDistance = FMath::Clamp(
		(absDeltaY * MDSGraphWireTokens::VerticalControlFactor) + (absDeltaX * 0.08f),
		effectiveMin,
		effectiveMax
	);
	const float horizontalControlDistance = FMath::Clamp(absDeltaX * MDSGraphWireTokens::HorizontalControlFactor, 0.0f, FMath::Min(96.0f, totalDist * 0.225f));

	OutControlPointA = StartPoint + FVector2D(signX * horizontalControlDistance, signY * verticalControlDistance);
	OutControlPointB = EndPoint - FVector2D(signX * horizontalControlDistance * 0.55f, signY * verticalControlDistance);
}
