// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Misc/DataValidation.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_ReturnToNode"

UMounteaDialogueGraphNode_ReturnToNode::UMounteaDialogueGraphNode_ReturnToNode() : DelayDuration(0.1f),
																				   bAutoCompleteSelectedNode(false),
																				   SelectedNode(nullptr)
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeTitle", "Return To Node");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeInternalTitle", "Return To Node");
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeContextMenu", "Return To Node");
	BackgroundColor = FLinearColor(FColor::White);

	bAllowOutputNodes = false;

	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_ReturnToNodeTooltip",
							  "* Provides ability to return from Dialogue Node to different one.\n* Useful when dialogue branching disallows pin connections.");
#endif

	bAutoStarts = true;

	bInheritGraphDecorators = false;

	AllowedInputClasses.Add(UMounteaDialogueGraphNode::StaticClass());

	// Disable those Node Classes
	AllowedNodesFilter.Add(UMounteaDialogueGraphNode_ReturnToNode::StaticClass());
	AllowedNodesFilter.Add(UMounteaDialogueGraphNode_CompleteNode::StaticClass());
	AllowedNodesFilter.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
}

void UMounteaDialogueGraphNode_ReturnToNode::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (GetWorld())
	{
		FTimerDelegate TimerDelegate_TypeWriterUpdateInterval;
		TimerDelegate_TypeWriterUpdateInterval.BindUFunction(this, "OnDelayDurationExpired", Manager);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_Delay, TimerDelegate_TypeWriterUpdateInterval, DelayDuration, false);
	}
	else
	{
		OnDelayDurationExpired(Manager);
	}
	
	Super::ProcessNode_Implementation(Manager);
}

void UMounteaDialogueGraphNode_ReturnToNode::OnDelayDurationExpired(const TScriptInterface<IMounteaDialogueManagerInterface>& MounteaDialogueManagerInterface)
{
	if (SelectedNode && MounteaDialogueManagerInterface)
	{
		if (const auto Context = MounteaDialogueManagerInterface->Execute_GetDialogueContext(MounteaDialogueManagerInterface.GetObject()))
		{
			auto dialogueNodeToStart = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(SelectedNode);
			
			Context->SetDialogueContext(Context->DialogueParticipant, SelectedNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(SelectedNode));
			
			FDataTableRowHandle newDialogueTableHandle = FDataTableRowHandle();
			newDialogueTableHandle.DataTable = dialogueNodeToStart->GetDataTable();
			newDialogueTableHandle.RowName = dialogueNodeToStart->GetRowName();
			Context->UpdateActiveDialogueTable(dialogueNodeToStart ? newDialogueTableHandle : FDataTableRowHandle());
			Context->UpdateActiveDialogueRow( UMounteaDialogueSystemBFC::FindDialogueRow(newDialogueTableHandle.DataTable, newDialogueTableHandle.RowName) );
			Context->ActiveDialogueRowDataIndex = 0;

			MounteaDialogueManagerInterface->Execute_NodeProcessed(MounteaDialogueManagerInterface.GetObject());

			// TODO: Force to the new system
			/*
			if (bAutoCompleteSelectedNode)
			{
				MounteaDialogueManagerInterface->GetDialogueNodeFinishedEventHandle().Broadcast(Context);
				MounteaDialogueManagerInterface->GetDialogueVoiceSkipRequestEventHandle().Broadcast(nullptr);
			}
			*/
		}
	}
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
