#include "SEdNode_MounteaDialogueGraphEdge.h"

#include "ConnectionDrawingPolicy.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "SGraphPanel.h"
#include "SNodePanel.h"

#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"

#define LOCTEXT_NAMESPACE "SMounteaDialogueGraphEdge"


void SEdNode_MounteaDialogueGraphEdge::Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphEdge* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}

bool SEdNode_MounteaDialogueGraphEdge::RequiresSecondPassLayout() const
{
	return true;
}

const FSlateBrush* SEdNode_MounteaDialogueGraphEdge::GetShadowBrush(bool bSelected) const
{
	return FCoreStyle::Get().GetBrush(TEXT("NoBrush"));
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
		const TSharedRef<SNode>* pToWidget   = NodeToWidgetLookup.Find(End);
		if (pFromWidget != nullptr && pToWidget != nullptr)
		{
			CachedStartNodeSize = (*pFromWidget)->GetDesiredSize();
			CachedEndNodeSize   = (*pToWidget)->GetDesiredSize();

			StartGeom = FGeometry(FVector2D(Start->NodePosX, Start->NodePosY), FVector2D::ZeroVector, CachedStartNodeSize, 1.0f);
			EndGeom   = FGeometry(FVector2D(End->NodePosX,   End->NodePosY),   FVector2D::ZeroVector, CachedEndNodeSize,   1.0f);

			PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, 0, 1);
			EdgeNode->bHasCachedSplineMidpoint = false;
		}
	}
}

void SEdNode_MounteaDialogueGraphEdge::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(GraphNode);
	if (!edgeNode)
		return;

	if (edgeNode->bHasCachedSplineMidpoint)
	{
		const FVector2D halfSize = GetDesiredSize() * 0.5f;
		GraphNode->NodePosX = edgeNode->CachedSplineMidpointGraph.X - halfSize.X;
		GraphNode->NodePosY = edgeNode->CachedSplineMidpointGraph.Y - halfSize.Y;
		return;
	}

	if (CachedStartNodeSize.IsZero() || CachedEndNodeSize.IsZero())
		return;

	UEdNode_MounteaDialogueGraphNode* start = edgeNode->GetStartNode();
	UEdNode_MounteaDialogueGraphNode* end   = edgeNode->GetEndNode();
	if (!start || !end)
		return;

	const FGeometry startGeom(FVector2D(start->NodePosX, start->NodePosY), FVector2D::ZeroVector, CachedStartNodeSize, 1.0f);
	const FGeometry endGeom  (FVector2D(end->NodePosX,   end->NodePosY),   FVector2D::ZeroVector, CachedEndNodeSize,   1.0f);
	PositionBetweenTwoNodesWithOffset(startGeom, endGeom, 0, 1);
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
					.WidthOverride(38.0f)
					.HeightOverride(38.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						.Padding(FMargin(-5.0f))
						[
							SNew(SImage)
							.Image(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Edge.SelectionRing")))
							.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeSelectionOverlayColor)
							.Visibility(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeSelectionRingVisibility)
						]
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Icon.BulletPoint")))
							.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeOuterRingColor)
						]
						+ SOverlay::Slot()
						.Padding(FMargin(FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRingInset()))
						[
							SNew(SImage)
							.Image(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Edge.SelectionRing")))
							.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeSelectionRingColor)
							.Visibility(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeSelectionRingVisibility)
						]
						+ SOverlay::Slot()
						.Padding(FMargin(FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRingPadding()))
						[
							SNew(SImage)
							.Image(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Icon.BulletPoint")))
							.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphEdge::GetEdgeActionBackgroundColor)
						]
						+ SOverlay::Slot()
						.Padding(FMargin(6.0f))
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
		return FReply::Unhandled();

	GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
	return FReply::Handled();
}

void SEdNode_MounteaDialogueGraphEdge::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	const FVector2D startCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D endCenter   = FGeometryHelper::CenterOf(EndGeom);

	const FVector2D startAnchorGraph(startCenter.X, startCenter.Y + StartGeom.GetLocalSize().Y * 0.5f);
	const FVector2D endAnchorGraph  (endCenter.X,   endCenter.Y   - EndGeom.GetLocalSize().Y   * 0.5f);

	const FVector2D newCenter = (startAnchorGraph + endAnchorGraph) * 0.5f;

	const FVector2D desiredSize = GetDesiredSize();
	FVector2D deltaNormal = (endAnchorGraph - startAnchorGraph).GetSafeNormal();
	if (deltaNormal.IsNearlyZero())
		deltaNormal = FVector2D(1.0f, 0.0f);

	const float multiNodeStep   = 1.2f;
	const float multiNodeStart  = -((MaxNodes - 1) * multiNodeStep) / 2.0f;
	const float multiNodeOffset = multiNodeStart + (NodeIndex * multiNodeStep);

	const FVector2D newCorner = newCenter - (0.5f * desiredSize) + (deltaNormal * multiNodeOffset * desiredSize.Size());

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
		bubbleColor = bubbleColor + FLinearColor(0.08f, 0.08f, 0.08f, 0.0f);

	if (IsSelectedExclusively())
		bubbleColor = bubbleColor + FLinearColor(0.08f, 0.08f, 0.08f, 0.0f);

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

FSlateColor SEdNode_MounteaDialogueGraphEdge::GetEdgeOuterRingColor() const
{
	if (IsEdgeSelected())
		return FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRing();

	return GetEdgeActionIconColor();
}

FSlateColor SEdNode_MounteaDialogueGraphEdge::GetEdgeSelectionRingColor() const
{
	if (IsHovered())
		return FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRingHovered();

	return FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRing();
}

FSlateColor SEdNode_MounteaDialogueGraphEdge::GetEdgeSelectionOverlayColor() const
{
	if (IsHovered())
		return FMounteaDialogueGraphVisualTokens::GetEdgeSelectionOverlayHovered();

	return FMounteaDialogueGraphVisualTokens::GetEdgeSelectionOverlay();
}

EVisibility SEdNode_MounteaDialogueGraphEdge::GetEdgeSelectionRingVisibility() const
{
	return IsEdgeSelected() ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SEdNode_MounteaDialogueGraphEdge::IsEdgeSelected() const
{
	if (IsSelectedExclusively())
		return true;

	const TSharedPtr<SGraphPanel> ownerPanel = GetOwnerPanel();
	if (!ownerPanel.IsValid() || !GraphNode)
		return false;

	return ownerPanel->SelectionManager.IsNodeSelected(GraphNode);
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
