// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_ReturnToNode"

UMounteaDialogueGraphNode_ReturnToNode::UMounteaDialogueGraphNode_ReturnToNode() : bAutoCompleteSelectedNode(false), SelectedNode(nullptr)
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

void UMounteaDialogueGraphNode_ReturnToNode::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (SelectedNode && Manager)
	{
		if (const auto Context = Manager->GetDialogueContext())
		{
			LOG_WARNING(TEXT("[ProcessNode - Return to Node] Updating Context"))
			
			Context->SetDialogueContext(Context->DialogueParticipant, SelectedNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(SelectedNode));

			Context->ActiveDialogueRowDataIndex = 	UMounteaDialogueSystemBFC::GetDialogueRow(SelectedNode).DialogueRowData.Num() - 1; // Force-set the last row
			
			Manager->GetDialogueNodeSelectedEventHandle().Broadcast(Context);	

			if (bAutoCompleteSelectedNode)
			{
				//Manager->GetDialogueNodeFinishedEventHandle().Broadcast(Context);
				//Manager->GetDialogueVoiceSkipRequestEventHandle().Broadcast(nullptr);
			}
		}
	}
	
	Super::ProcessNode_Implementation(Manager);
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_ReturnToNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeCategory", "Utility Nodes");
}

bool UMounteaDialogueGraphNode_ReturnToNode::ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat)
{
	bool bSatisfied = Super::ValidateNode(ValidationsMessages, RichFormat);

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
	
				ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
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
	
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
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
