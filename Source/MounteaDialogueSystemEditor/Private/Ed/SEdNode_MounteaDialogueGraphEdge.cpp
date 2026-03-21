#include "SEdNode_MounteaDialogueGraphEdge.h"

#include "ConnectionDrawingPolicy.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "SGraphPanel.h"
#include "SNodePanel.h"

#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"

#define LOCTEXT_NAMESPACE "SMounteaDialogueGraphEdge"

namespace
{
	FVector2D EvaluateBezierPoint(const FVector2D& StartPoint, const FVector2D& ControlPointA, const FVector2D& ControlPointB, const FVector2D& EndPoint, const float T)
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

	void CalculateEdgeBezierControlPoints(const FVector2D& StartPoint, const FVector2D& EndPoint, FVector2D& OutControlPointA, FVector2D& OutControlPointB)
	{
		const FVector2D delta = EndPoint - StartPoint;
		const float absDeltaX = FMath::Abs(delta.X);
		const float absDeltaY = FMath::Abs(delta.Y);
		const float signX = (delta.X >= 0.0f) ? 1.0f : -1.0f;
		const float signY = (delta.Y >= 0.0f) ? 1.0f : -1.0f;

		const bool isMostlyHorizontal = absDeltaY < 36.0f && absDeltaX > 36.0f;
		if (isMostlyHorizontal)
		{
			const float flatControlDistance = FMath::Clamp(absDeltaX * 0.45f, 56.0f, 220.0f);
			OutControlPointA = StartPoint + FVector2D(signX * flatControlDistance, 0.0f);
			OutControlPointB = EndPoint - FVector2D(signX * flatControlDistance, 0.0f);
			return;
		}

		const float verticalControlDistance = FMath::Clamp((absDeltaY * 0.56f) + (absDeltaX * 0.08f), 42.0f, 220.0f);
		const float horizontalControlDistance = FMath::Clamp(absDeltaX * 0.22f, 0.0f, 96.0f);
		OutControlPointA = StartPoint + FVector2D(signX * horizontalControlDistance, signY * verticalControlDistance);
		OutControlPointB = EndPoint - FVector2D(signX * horizontalControlDistance * 0.55f, signY * verticalControlDistance);
	}
}

void SEdNode_MounteaDialogueGraphEdge::Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphEdge* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}

bool SEdNode_MounteaDialogueGraphEdge::RequiresSecondPassLayout() const
{
	return true;
}

void SEdNode_MounteaDialogueGraphEdge::PerformSecondPassLayout(const TMap<UObject*, TSharedRef<SNode>>& NodeToWidgetLookup) const
{
	UEdNode_MounteaDialogueGraphEdge* EdgeNode = CastChecked<UEdNode_MounteaDialogueGraphEdge>(GraphNode);

	FGeometry StartGeom;
	FGeometry EndGeom;

	UEdNode_MounteaDialogueGraphNode* Start = EdgeNode->GetStartNode();
	UEdNode_MounteaDialogueGraphNode* End = EdgeNode->GetEndNode();
	if (Start != nullptr && End != nullptr)
	{
		const TSharedRef<SNode>* pFromWidget = NodeToWidgetLookup.Find(Start);
		const TSharedRef<SNode>* pToWidget = NodeToWidgetLookup.Find(End);
		if (pFromWidget != nullptr && pToWidget != nullptr)
		{
			const TSharedRef<SNode>& FromWidget = *pFromWidget;
			const TSharedRef<SNode>& ToWidget = *pToWidget;

			StartGeom = FGeometry(FVector2D(Start->NodePosX, Start->NodePosY), FVector2D::ZeroVector, FromWidget->GetDesiredSize(), 1.0f);
			EndGeom = FGeometry(FVector2D(End->NodePosX, End->NodePosY), FVector2D::ZeroVector, ToWidget->GetDesiredSize(), 1.0f);
		}
	}

	PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, 0, 1);
}

void SEdNode_MounteaDialogueGraphEdge::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	
	
	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
				.Padding(FMargin(0.0f))
				.OnMouseButtonDown(this, &SEdNode_MounteaDialogueGraphEdge::OnMouseButtonDown)
				[
					SNew(SBox)
					.WidthOverride(30.0f)
					.HeightOverride(30.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Icon.BulletPoint")))
							.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeActionIconColor)
						]
						+ SOverlay::Slot()
						.Padding(FMargin(1.0f))
						[
							SNew(SImage)
							.Image(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Icon.BulletPoint")))
							.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeActionBackgroundColor)
						]
						+ SOverlay::Slot()
						.Padding(FMargin(5.0f))
						[
							SNew(SImage)
							.Image(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Edge.MidpointIcon")))
							.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeActionIconColor)
						]
					]
				]
			]
		];
}

FReply SEdNode_MounteaDialogueGraphEdge::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!GraphNode || !GetOwnerPanel())
	{
		return FReply::Unhandled();
	}

	GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
	return FReply::Handled();
}

void SEdNode_MounteaDialogueGraphEdge::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D startCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D endCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D seedPoint = (startCenter + endCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D startAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, seedPoint);
	const FVector2D endAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, seedPoint);
	FVector2D controlPointA;
	FVector2D controlPointB;
	CalculateEdgeBezierControlPoints(startAnchorPoint, endAnchorPoint, controlPointA, controlPointB);
	const FVector2D newCenter = EvaluateBezierPoint(startAnchorPoint, controlPointA, controlPointB, endAnchorPoint, 0.5f);
	const FVector2D desiredNodeSize = GetDesiredSize();
	FVector2D deltaNormal = (endAnchorPoint - startAnchorPoint).GetSafeNormal();
	if (deltaNormal.IsNearlyZero())
		deltaNormal = FVector2D(1.0f, 0.0f);
	
	// Calculate node offset in the case of multiple transitions between the same two nodes
	// MultiNodeOffset: the offset where 0 is the centre of the transition, -1 is 1 <size of node>
	// towards the PrevStateNode and +1 is 1 <size of node> towards the NextStateNode.

	const float mutliNodeSpace = 0.2f; // Space between multiple transition nodes (in units of <size of node> )
	const float multiNodeStep = (1.f + mutliNodeSpace); //Step between node centres (Size of node + size of node spacer)

	const float multiNodeStart = -((MaxNodes - 1) * multiNodeStep) / 2.f;
	const float multiNodeOffset = multiNodeStart + (NodeIndex * multiNodeStep);

	// Now we need to adjust the new center by the node size, zoom factor and multi node offset
	const FVector2D newCorner = newCenter - (0.5f * desiredNodeSize) + (deltaNormal * multiNodeOffset * desiredNodeSize.Size());

	GraphNode->NodePosX = newCorner.X;
	GraphNode->NodePosY = newCorner.Y;
}

FSlateColor SEdNode_MounteaDialogueGraphEdge::GetEdgeColor() const
{
	return IsSelectedExclusively()
		? FMounteaDialogueGraphVisualTokens::GetPrimaryAccent()
		: FMounteaDialogueGraphVisualTokens::GetWireColor();
}

FSlateColor SEdNode_MounteaDialogueGraphEdge::GetEdgeActionBackgroundColor() const
{
	FLinearColor bubbleColor = FMounteaDialogueGraphVisualTokens::GetCanvasBackground();
	bubbleColor.A = 1.0f;

	if (IsHovered())
	{
		bubbleColor = bubbleColor + FLinearColor(0.08f, 0.08f, 0.08f, 0.0f);
	}

	if (IsSelectedExclusively())
	{
		bubbleColor = bubbleColor + FLinearColor(0.08f, 0.08f, 0.08f, 0.0f);
	}

	return bubbleColor;
}

FSlateColor SEdNode_MounteaDialogueGraphEdge::GetEdgeActionIconColor() const
{
	FLinearColor iconColor = FLinearColor::White;
	if (HasValidConditions())
		iconColor = FMounteaDialogueGraphVisualTokens::GetPrimaryAccent();

	if (IsHovered())
		iconColor.A = 1.0f;
	else
		iconColor.A = 0.90f;

	return iconColor;
}

bool SEdNode_MounteaDialogueGraphEdge::HasValidConditions() const
{
	const UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(GraphNode);
	if (!edgeNode || !edgeNode->MounteaDialogueGraphEdge)
		return false;

	const FMounteaDialogueEdgeConditions edgeConditions = edgeNode->MounteaDialogueGraphEdge->GetEdgeConditions();
	for (const FMounteaDialogueCondition& condition : edgeConditions.Rules)
	{
		if (IsValid(condition.ConditionClass))
			return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
