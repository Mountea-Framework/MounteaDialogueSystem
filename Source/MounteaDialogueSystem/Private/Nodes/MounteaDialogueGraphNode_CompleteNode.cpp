// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_CompleteNode"

UMounteaDialogueGraphNode_CompleteNode::UMounteaDialogueGraphNode_CompleteNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_CompleteNodeTitle", "Complete Node");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_CompleteNodeContextMenuName", "Complete Node");
	BackgroundColor = FLinearColor(1, 0, 0, 1);

	bAllowOutputNodes = false;
}

FText UMounteaDialogueGraphNode_CompleteNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_CompleteNodeDescription", "Complete Node is a helper visual Node which indicates ending of its dialogue branch. Is not required to use.");
}

#undef LOCTEXT_NAMESPACE
