#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "SNodePanel.h"
#include "SGraphNode.h"

class UEdNode_MounteaDialogueGraphEdge;
class UEdNode_MounteaDialogueGraphNode;
class SToolTip;

class SEdNode_MounteaDialogueGraphEdge : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SEdNode_MounteaDialogueGraphEdge){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphEdge* InNode);

	virtual bool RequiresSecondPassLayout() const override;
	virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;

	virtual void UpdateGraphNode() override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

protected:
	FSlateColor GetEdgeColor() const;
	FSlateColor GetEdgeActionBackgroundColor() const;
	FSlateColor GetEdgeActionIconColor() const;
	FSlateColor GetEdgeOuterRingColor() const;
	FSlateColor GetEdgeSelectionRingColor() const;
	FSlateColor GetEdgeSelectionOverlayColor() const;
	EVisibility GetEdgeSelectionRingVisibility() const;
	bool IsEdgeSelected() const;
	bool HasValidConditions() const;

private:
	TSharedPtr<STextEntryPopup> TextEntryWidget;

	mutable FVector2D CachedEndNodeSize;
	
};
