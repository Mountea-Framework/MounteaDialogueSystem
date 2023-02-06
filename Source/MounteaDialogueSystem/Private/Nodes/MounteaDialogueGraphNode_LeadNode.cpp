// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_LeadNode"

UMounteaDialogueGraphNode_LeadNode::UMounteaDialogueGraphNode_LeadNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBase", "DialogueLeadNode");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBase", "Lead Node");
	BackgroundColor = FLinearColor(FColor::Orange);

	AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());
}

#undef LOCTEXT_NAMESPACE