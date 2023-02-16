// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_AutoCompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_AutoCompleteNode"

UMounteaDialogueGraphNode_AutoCompleteNode::UMounteaDialogueGraphNode_AutoCompleteNode()
{
#if WITH_EDITORONLY_DATA
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeTitle", "Auto Complete Dialogue");
	InternalName = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeInternalTitle", "Auto Complete Dialogue");
	
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeContextMenuName", "Auto Complete Dialogue");
#endif
}

FText UMounteaDialogueGraphNode_AutoCompleteNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AutoCompleteNodeDescription", "Auto Complete Node is a helper Node which automatically closes Dialogue upon reaching.");
}

#undef LOCTEXT_NAMESPACE