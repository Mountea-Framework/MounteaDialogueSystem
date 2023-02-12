// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_AutoCompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_AutoCompleteNode"

UMounteaDialogueGraphNode_AutoCompleteNode::UMounteaDialogueGraphNode_AutoCompleteNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeTitle", "Auto Complete Node");
	InternalName = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeInternalTitle", "Auto Complete Node");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeContextMenuName", "Auto Complete Node");
}

FText UMounteaDialogueGraphNode_AutoCompleteNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeDescription", "Auto Complete Node is a helper Node which automatically closes Dialogue upon reaching.");
}

#undef LOCTEXT_NAMESPACE