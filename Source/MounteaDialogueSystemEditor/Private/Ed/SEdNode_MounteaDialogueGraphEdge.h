#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "SNodePanel.h"
#include "SGraphNode.h"

class UEdNode_MounteaDialogueGraphEdge;
class SToolTip;

class SEdNode_MounteaDialogueGraphEdge : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SEdNode_MounteaDialogueGraphEdge){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphEdge* InNode);

	virtual bool RequiresSecondPassLayout() const override;
	virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const override;

	virtual void UpdateGraphNode() override;

	// Calculate position for multiple nodes to be placed between a start and end point, by providing this nodes index and max expected nodes 
	void PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const;

protected:
	FSlateColor GetEdgeColor() const;

private:
	TSharedPtr<STextEntryPopup> TextEntryWidget;
	
};
