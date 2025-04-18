// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_StartNode"

UMounteaDialogueGraphNode_StartNode::UMounteaDialogueGraphNode_StartNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_StartNodeTitle", "Start Dialogue");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_StartNodeInternalTitle", "Start Dialogue");
#if WITH_EDITORONLY_DATA
	bAllowInputNodes = false;
	
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_StartNodeContextMenuName", "Start Dialogue");
	BackgroundColor = FLinearColor(0, 1, 0, 1);
	
	bAllowCopy = false;
	bAllowCut = false;
	bAllowPaste = false;
	bAllowDelete = false;
	bAllowManualCreate = false;
	bCanRenameNode = false;

	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_CompleteTooltip", "* This Node will be added to the Dialogue Graph automatically when Graph is created.\n* This Node cannot be created manually.\n* This Node cannot be deleted from Graph.\n* Does not implement any logic, works as an Anchor starting point.");
#endif

	// TODO: Once there are Conditional Decorators, this will be replaced
	MaxChildrenNodes = 1;
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_StartNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_StartNodeDescription", "Start Node is automatically placed Node which cannot be deleted. Indicates starting point of the Dialogue.");
}

FText UMounteaDialogueGraphNode_StartNode::GetNodeCategory_Implementation() const
{
	return Super::GetNodeCategory_Implementation();
}

bool UMounteaDialogueGraphNode_StartNode::ValidateNode(FDataValidationContext& Context, const bool RichFormat) const
{
	bool bResult = Super::ValidateNode(Context, RichFormat);

	if (ChildrenNodes.Num() == 0)
	{
		bResult = false;

		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": Does not have any Children Nodes!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": Does not have any Children Nodes!");
		
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (ChildrenNodes.Num() > 1)
	{
		bResult = false;

		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": Does have more than 1 Child Node. This version can utilize only first Child Node from Start Node!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": Does have more than 1 Child Node. This version can utilize only first Child Node from Start Node!");
		
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	return bResult;
}

#endif

#undef LOCTEXT_NAMESPACE
