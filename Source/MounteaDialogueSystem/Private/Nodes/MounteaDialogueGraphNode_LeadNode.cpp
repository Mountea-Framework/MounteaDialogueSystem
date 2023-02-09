// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_LeadNode"

UMounteaDialogueGraphNode_LeadNode::UMounteaDialogueGraphNode_LeadNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_LeadNodeBaseTitle", "Lead Node");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_LeadNodeContextMenuName", "Lead Node");
	BackgroundColor = FLinearColor(FColor::Orange);

	AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());
}

FText UMounteaDialogueGraphNode_LeadNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_LeadNodeDescription", "Lead Node is a Node which usually a contains NPC parts of the Dialogue.");
}

FText UMounteaDialogueGraphNode_LeadNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_LeadNodeCategory", "Mountea Dialogue Branche Nodes");
}

#undef LOCTEXT_NAMESPACE
