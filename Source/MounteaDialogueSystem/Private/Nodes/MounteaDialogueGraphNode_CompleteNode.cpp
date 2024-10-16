// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_CompleteNode"

UMounteaDialogueGraphNode_CompleteNode::UMounteaDialogueGraphNode_CompleteNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_CompleteNodeTitle", "Complete Dialogue");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_CompleteNodeInternalTitle", "Complete Dialogue");
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_CompleteNodeContextMenuName", "Complete Dialogue");
	BackgroundColor = FLinearColor(1, 0, 0, 1);

	bAllowOutputNodes = false;

	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_CompleteTooltip", "* This Node will complete Dialogue after Player's input.\n* Indicates that Dialogue can be manually closed.\n* Requires Dialogue Data Table to work properly.");
#endif

	AllowedInputClasses.Add(UMounteaDialogueGraphNode_DialogueNodeBase::StaticClass());
	bAutoStarts = false;
	bUseGameplayTags = false;
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_CompleteNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_CompleteNodeDescription", "Complete Node is an indication that Dialogue can be manually closed here by Player.");
}

FText UMounteaDialogueGraphNode_CompleteNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_CompleteNodeCategory", "Mountea Dialogue Nodes");
}

#endif
#undef LOCTEXT_NAMESPACE
