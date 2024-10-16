// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_AutoCompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_AutoCompleteNode"

UMounteaDialogueGraphNode_AutoCompleteNode::UMounteaDialogueGraphNode_AutoCompleteNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeTitle", "Auto Complete Dialogue");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeInternalTitle", "Auto Complete Dialogue");
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeContextMenuName", "Auto Complete Dialogue");
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteTooltip", "* This Node will automatically complete Dialogue.\n* Same functionality as Node being left with no branching or children.\n* Does help performance, however, is not required.");

#endif

	bAutoStarts = true;
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_AutoCompleteNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeDescription", "Auto Complete Node is a helper Node which automatically closes Dialogue upon reaching.");
}

FText UMounteaDialogueGraphNode_AutoCompleteNode::GetNodeCategory_Implementation() const
{
	return Super::GetNodeCategory_Implementation();
}

#endif

#undef LOCTEXT_NAMESPACE
