// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"

#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_AnswerNode"

UMounteaDialogueGraphNode_AnswerNode::UMounteaDialogueGraphNode_AnswerNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeTitle", "Answear Node");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeContextMenuName", "Answear Node");
	BackgroundColor = FLinearColor(FColor::Turquoise);

	AllowedInputClasses.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass());
}

FText UMounteaDialogueGraphNode_AnswerNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AnswerNodeDescription", "Answear Node is a Node which usually contains Player parts of the Dialogue.");
}

FText UMounteaDialogueGraphNode_AnswerNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AnswearNodeCategory", "Mountea Dialogue Branche Nodes");
}

#undef LOCTEXT_NAMESPACE
