#include "SEdNode_MounteaDialogueGraphEdge.h"

#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "SGraphPanel.h"
#include "SNodePanel.h"

#include "Consts/MounteaDialogueEditorConsts.h"
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
	if (!EdgeNode)
		return;

	UEdNode_MounteaDialogueGraphNode* End = EdgeNode->GetEndNode();
	if (!End)
		return;

	const TSharedRef<SNode>* pToWidget = NodeToWidgetLookup.Find(End);
	if (!pToWidget)
		return;

	CachedEndNodeSize = (*pToWidget)->GetDesiredSize();
	GraphNode->NodePosX = End->NodePosX + CachedEndNodeSize.X * 0.5f - MounteaDialogueWireConsts::BubbleHalfSize;
	GraphNode->NodePosY = End->NodePosY - MounteaDialogueWireConsts::BubbleYOffset;
}

void SEdNode_MounteaDialogueGraphEdge::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (CachedEndNodeSize.IsZero())
		return;

	UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(GraphNode);
	if (!edgeNode)
		return;

	UEdNode_MounteaDialogueGraphNode* end = edgeNode->GetEndNode();
	if (!end)
		return;

	GraphNode->NodePosX = end->NodePosX + CachedEndNodeSize.X * 0.5f - MounteaDialogueWireConsts::BubbleHalfSize;
	GraphNode->NodePosY = end->NodePosY - MounteaDialogueWireConsts::BubbleYOffset;
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
					.WidthOverride(MounteaDialogueWireConsts::BubbleSize)
					.HeightOverride(MounteaDialogueWireConsts::BubbleSize)
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
