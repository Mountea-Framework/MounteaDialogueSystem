// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "Nodes/MounteaDialogueGraphNode_DialogueNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_DelayNode"

UMounteaDialogueGraphNode_DialogueNode::UMounteaDialogueGraphNode_DialogueNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseTitle", "Dialogue Node");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseInternalTitle", "Dialogue Node");
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeContextMenuName", "Editable Dialogue Node");
	
	EditorNodeColour = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("297373")));
	EditorHeaderForegroundColour = FLinearColor::White;
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeTooltip", "* Editable Dialogue Node which allows direct input instead of using Data Table.");
#endif
	
	bAutoStarts = false;
	
	AllowedInputClasses.Add(UMounteaDialogueGraphNode::StaticClass());

	MaxChildrenNodes = -1;
}

bool UMounteaDialogueGraphNode_DialogueNode::SetSpeechData_Implementation(const FDialogueRow& NewSpeechData)
{
	if (NewSpeechData.IsNearlyEqual(Dialogue))
		return false;
	Dialogue = NewSpeechData;
	return true;
}

FText UMounteaDialogueGraphNode_DialogueNode::GetNodeTitle_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DialogueNodeTitle", "Dialogue Speech Node");
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_DialogueNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DialogueNodeDescription", "Editable Dialogue Node which allows direct input instead of using Data Table.");
}

FText UMounteaDialogueGraphNode_DialogueNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DialogueNodeCategory", "Dialogue");
}

bool UMounteaDialogueGraphNode_DialogueNode::ValidateNode(FDataValidationContext& Context, const bool RichFormat) const
{
	return Super::ValidateNode(Context, RichFormat);
}

#endif


#undef LOCTEXT_NAMESPACE