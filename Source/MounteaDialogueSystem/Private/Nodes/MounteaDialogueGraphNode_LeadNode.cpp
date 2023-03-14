// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_LeadNode"

UMounteaDialogueGraphNode_LeadNode::UMounteaDialogueGraphNode_LeadNode()
{
#if WITH_EDITORONLY_DATA
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_LeadNodeBaseTitle", "Lead Node");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_LeadNodeBaseInternalTitle", "Lead Node");
	
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_LeadNodeContextMenuName", "Lead Node");
	
	BackgroundColor = FLinearColor(FColor::Orange);
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_LeadTooltip", "* This Node represents NPC lines.\n* This Node starts automatically upon reaching in the Dialogue Tree.\n* Requires Dialogue Data Table to work properly.");
#endif
	
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());

	bAutoStarts = true;
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
