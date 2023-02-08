// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_StartNode"

UMounteaDialogueGraphNode_StartNode::UMounteaDialogueGraphNode_StartNode()
{
	bAllowInputNodes = false;
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_StartNodeTitle", "Start Node");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_StartNodeContextMenuName", "Start Node");
	BackgroundColor = FLinearColor(0, 1, 0, 1);

	bAllowCopy = false;
	bAllowCut = false;
	bAllowPaste = false;
	bAllowDelete = false;
	bAllowManualCreate = false;
}

FText UMounteaDialogueGraphNode_StartNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_StartNodeDescription", "Start Node is automatically placed Node which cannot be deleted. Indicates starting point of the Dialogue.");
}

FText UMounteaDialogueGraphNode_StartNode::GetNodeCategory_Implementation() const
{
	return Super::GetNodeCategory_Implementation();
}

#undef LOCTEXT_NAMESPACE
