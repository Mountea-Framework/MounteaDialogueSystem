// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"

#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Misc/DataValidation.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_ReturnToNode"

UMounteaDialogueGraphNode_ReturnToNode::UMounteaDialogueGraphNode_ReturnToNode()
{
#if WITH_EDITORONLY_DATA
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeTitle", "Return To Node");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeInternalTitle", "Return To Node");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeContextMenu", "Return To Node");
	BackgroundColor = FLinearColor(FColor::White);

	bAllowOutputNodes = false;
	
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeTooltip", "* Provides ability to return from Dialogue Node to different one.\n* Useful when dialogue branching disallows pin connections.");
#endif

	bAutoStarts = true;

	bInheritGraphDecorators = false;
	
	AllowedInputClasses.Add(UMounteaDialogueGraphNode::StaticClass());

	// Disable those Node Classes
	AllowedNodesFilter.Add(UMounteaDialogueGraphNode_ReturnToNode::StaticClass());
	AllowedNodesFilter.Add(UMounteaDialogueGraphNode_CompleteNode::StaticClass());
}

void UMounteaDialogueGraphNode_ReturnToNode::ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (SelectedNode && Manager)
	{
		if (const auto Context = Manager->GetDialogueContext())
		{
			Context->SetDialogueContext(Context->DialogueParticipant, SelectedNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(SelectedNode));
			Manager->GetDialogueNodeSelectedEventHandle().Broadcast(Context);
		}
	}
	
	Super::ProcessNode(Manager);
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_ReturnToNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeCategory", "Utility Nodes");
}

bool UMounteaDialogueGraphNode_ReturnToNode::ValidateNode(FDataValidationContext& Context, const bool RichFormat) const
{
	bool bSatisfied = Super::ValidateNode(Context, RichFormat);

	if (ParentNodes.Num() > 0)
	{
		for (const auto& Itr : ParentNodes)
		{
			if (Itr && Itr->ChildrenNodes.Num() > 1)
			{
				bSatisfied = false;

				const FString RichTextReturn =
				FString("* ").
				Append("<RichTextBlock.Bold>").
				Append(NodeTitle.ToString()).
				Append("</>").
				Append(": This node expects to be only output from its Parent Node(s)!");

				const FString TextReturn =
				FString(NodeTitle.ToString()).
				Append(": This node expects to be only output from its Parent Node(s)!");
	
				Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
			}
		}
	}

	if (SelectedNodeIndex.IsEmpty())
	{
		bSatisfied = false;

		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": Selected Node is not Valid!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": Selected Node is not Valid!");
	
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	return bSatisfied;
}

void UMounteaDialogueGraphNode_ReturnToNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_ReturnToNode, SelectedNodeIndex))
	{
		if (SelectedNodeIndex.IsEmpty())
		{
			SelectedNode = nullptr;
		}
		else
		{
			const int32 Index = FCString::Atoi((TEXT("%s"), *SelectedNodeIndex));
			SelectedNode = Graph->AllNodes[Index];
		}
	}

	ReturnNodeUpdated.ExecuteIfBound();
}

FText UMounteaDialogueGraphNode_ReturnToNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeDescription", "Provides ability to return from Dialogue Node to different one.");
}

#endif

#undef LOCTEXT_NAMESPACE
