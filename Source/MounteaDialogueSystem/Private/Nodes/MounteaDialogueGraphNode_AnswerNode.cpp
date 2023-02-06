// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"

#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_AnswerNode"

UMounteaDialogueGraphNode_AnswerNode::UMounteaDialogueGraphNode_AnswerNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBase", "DialogueAnswearNode");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBase", "Answear Node");
	BackgroundColor = FLinearColor(FColor::Turquoise);

	AllowedInputClasses.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass());
}

#undef LOCTEXT_NAMESPACE