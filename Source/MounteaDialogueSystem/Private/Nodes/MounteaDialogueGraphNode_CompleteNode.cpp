// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_CompleteNode"

UMounteaDialogueGraphNode_CompleteNode::UMounteaDialogueGraphNode_CompleteNode()
{
	bAllowOutputNodes = false;
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_CompleteNode", "DialogueCompleteNode");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_CompleteNode", "Complete Node");
	BackgroundColor = FLinearColor(1, 0, 0, 1);
}

#undef LOCTEXT_NAMESPACE