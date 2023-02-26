// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"

#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_AnswerNode"

UMounteaDialogueGraphNode_AnswerNode::UMounteaDialogueGraphNode_AnswerNode()
{
#if WITH_EDITORONLY_DATA
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeTitle", "Answer Node");
	InternalName = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeInternalTitle", "Answer Node");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeContextMenuName", "Answer Node");
	
	BackgroundColor = FLinearColor(FColor::Turquoise);
#endif
	
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());

	bAutoStarts = false;

	// TODO: Once there are Conditional Decorators, this will be replaced
	MaxChildrenNodes = 1;
}

FText UMounteaDialogueGraphNode_AnswerNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AnswerNodeDescription", "Answear Node is a Node which usually contains Player parts of the Dialogue.");
}

FText UMounteaDialogueGraphNode_AnswerNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AnswearNodeCategory", "Mountea Dialogue Branche Nodes");
}

#if WITH_EDITOR
bool UMounteaDialogueGraphNode_AnswerNode::ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat)
{
	bool Result = Super::ValidateNode(ValidationsMessages, RichFormat);
	
	return Result;
}

#endif

#undef LOCTEXT_NAMESPACE
