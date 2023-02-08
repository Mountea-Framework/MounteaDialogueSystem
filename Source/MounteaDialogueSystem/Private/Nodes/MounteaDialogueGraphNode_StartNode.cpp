// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_StartNode"

UMounteaDialogueGraphNode_StartNode::UMounteaDialogueGraphNode_StartNode()
{
	bAllowInputNodes = false;
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_StartNode", "DialogueStartNode");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_StartNode", "Start Node");
	BackgroundColor = FLinearColor(0, 1, 0, 1);

	bAllowCopy = false;
	bAllowCut = false;
	bAllowPaste = false;
	bAllowDelete = false;
	bAllowManualCreate = false;
}

#undef LOCTEXT_NAMESPACE