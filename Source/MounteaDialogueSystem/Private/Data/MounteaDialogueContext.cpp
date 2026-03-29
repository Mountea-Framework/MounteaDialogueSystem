// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools


#include "Data/MounteaDialogueContext.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"


FString UMounteaDialogueContext::ToString() const
{
	FString returnValue;

	FString activeDialoguePart = FString("Active Dialogue Participant: ");
	activeDialoguePart.Append(ActiveDialogueParticipant.GetObject() ? ActiveDialogueParticipant->Execute_GetParticipantTag(ActiveDialogueParticipant.GetObject()).ToString() : TEXT("invalid"));

	FString allDialogueParts = FString("Dialogue Participants: ");
	allDialogueParts.Append(FString::Printf(TEXT("%d"), DialogueParticipants.Num()));

	FString activeNode = FString("Active Node ID: ");
	activeNode.Append(ActiveNode ? ActiveNode->GetNodeGUID().ToString() : TEXT("invalid"));

	FString activeRow = FString("Active Row: ");
	activeRow.Append(ActiveDialogueRow.RowTitle.ToString());

	FString activeRowData = FString("Active Row Data: ");
	activeRowData.Append(FString::Printf(TEXT("%d"), ActiveDialogueRow.RowData.Num()));

	FString lastWidgetCommand = FString("Last Widget Context: ");
	lastWidgetCommand.Append(FString::Printf(TEXT("%s"), *LastWidgetCommand));

	returnValue
		.Append(activeDialoguePart).Append(TEXT("\n"))
		.Append(allDialogueParts).Append(TEXT("\n"))
		.Append(activeNode).Append(TEXT("\n"))
		.Append(activeRow).Append(TEXT("\n"))
		.Append(activeRowData)
		.Append(lastWidgetCommand);

	return returnValue;
}

bool UMounteaDialogueContext::IsValid() const
{
	return ActiveNode != nullptr && DialogueParticipants.Num() > 0;
}

void UMounteaDialogueContext::SetDialogueContext(UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	if (ActiveNode && ActiveNode->GetNodeGUID() != PreviousActiveNode)
		PreviousActiveNode = ActiveNode->GetNodeGUID();

	ActiveNode = NewActiveNode;
	AllowedChildNodes = NewAllowedChildNodes;
}

void UMounteaDialogueContext::UpdateActiveDialogueNode(UMounteaDialogueGraphNode* NewActiveNode)
{
	if (ActiveNode && ActiveNode->GetNodeGUID() != PreviousActiveNode)
	{
		PreviousActiveNode = ActiveNode->GetNodeGUID();
	}
	
	ActiveNode = NewActiveNode;

	OnDialogueContextUpdated.Broadcast();
}

void UMounteaDialogueContext::UpdateAllowedChildrenNodes(const TArray<UMounteaDialogueGraphNode*>& NewNodes)
{
	AllowedChildNodes = NewNodes;
}

void UMounteaDialogueContext::UpdateActiveDialogueTable(const FDataTableRowHandle& NewHandle)
{
	ActiveDialogueTableHandle = NewHandle;
}

void UMounteaDialogueContext::UpdateActiveDialogueRow(const FDialogueRow& NewActiveRow)
{
	ActiveDialogueRow = NewActiveRow;
}

void UMounteaDialogueContext::UpdateActiveDialogueRowDataIndex(const int32 NewIndex)
{
	ActiveDialogueRowDataIndex = NewIndex;
}

void UMounteaDialogueContext::SetActiveDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant)
{
	if (NewParticipant != ActiveDialogueParticipant)
		return;

	ActiveDialogueParticipant = NewParticipant;

	OnDialogueContextUpdated.Broadcast();
}

void UMounteaDialogueContext::AddTraversedNode(const UMounteaDialogueGraphNode* TraversedNode)
{
	if (!TraversedNode || !TraversedNode->Graph)
	{
		return;
	}
	
	FDialogueTraversePath* ExistingRow = TraversedPath.FindByPredicate([&](FDialogueTraversePath& Path)
	{
		return Path.NodeGuid == TraversedNode->GetNodeGUID() && Path.GraphGuid == TraversedNode->GetGraphGUID();
	});

	if (ExistingRow)
	{
		ExistingRow->TraverseCount++;
	}
	else
	{
		FDialogueTraversePath NewRow;
		{
			NewRow.NodeGuid = TraversedNode->GetNodeGUID();
			NewRow.GraphGuid = TraversedNode->GetGraphGUID();
			NewRow.TraverseCount = 1;
		}
		TraversedPath.Add(NewRow);
	}
}

bool UMounteaDialogueContext::AddDialogueParticipants(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	bool bSatisfied = true;
	for (const auto& Itr : NewParticipants)
	{
		const bool bTempSatisfied = AddDialogueParticipant(Itr);
		bSatisfied = bTempSatisfied ? bSatisfied : bTempSatisfied;
	}

	return bSatisfied;
}

bool UMounteaDialogueContext::AddDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant)
{
	if (DialogueParticipants.Contains(NewParticipant))
	{
		return false;
	}

	DialogueParticipants.Add(NewParticipant);
	return true;
}

bool UMounteaDialogueContext::RemoveDialogueParticipants(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	bool bSatisfied = true;
	for (const auto& Itr : NewParticipants)
	{
		const bool bTempSatisfied = RemoveDialogueParticipant(Itr);
		bSatisfied = bTempSatisfied ? bSatisfied : bTempSatisfied;
	}

	return bSatisfied;
}

bool UMounteaDialogueContext::RemoveDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant)
{
	if (DialogueParticipants.Contains(NewParticipant))
	{
		DialogueParticipants.Remove(NewParticipant);
		return true;
	}

	return false;
}

void UMounteaDialogueContext::ClearDialogueParticipants()
{
	DialogueParticipants.Empty();
}

void UMounteaDialogueContext::SetDialogueContextBP(UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	SetDialogueContext(NewActiveNode, NewAllowedChildNodes);

	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

void UMounteaDialogueContext::UpdateActiveDialogueNodeBP(UMounteaDialogueGraphNode* NewActiveNode)
{
	UpdateActiveDialogueNode(NewActiveNode);

	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

void UMounteaDialogueContext::UpdateActiveDialogueRowBP(const FDialogueRow& NewActiveRow)
{
	UpdateActiveDialogueRow(NewActiveRow);
	
	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

void UMounteaDialogueContext::UpdateActiveDialogueTableBP(const FDataTableRowHandle& NewTable)
{
	UpdateActiveDialogueTable(NewTable);

	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

void UMounteaDialogueContext::UpdateActiveDialogueRowDataIndexBP(const int32 NewIndex)
{
	UpdateActiveDialogueRowDataIndex(NewIndex);

	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

void UMounteaDialogueContext::UpdateActiveDialogueParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	SetActiveDialogueParticipant(NewParticipant);

	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

bool UMounteaDialogueContext::AddDialogueParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant)
{
	if (AddDialogueParticipant(NewParticipant))
	{
		DialogueContextUpdatedFromBlueprint.Broadcast(this);
		return true;
	}

	return false;
}

bool UMounteaDialogueContext::RemoveDialogueParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant)
{
	if (RemoveDialogueParticipant(NewParticipant))
	{
		DialogueContextUpdatedFromBlueprint.Broadcast(this);
		return true;
	}

	return false;
}

bool UMounteaDialogueContext::AddDialogueParticipantsBP(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	if (AddDialogueParticipants(NewParticipants))
	{
		DialogueContextUpdatedFromBlueprint.Broadcast(this);
		return true;
	}

	return false;
}

bool UMounteaDialogueContext::RemoveDialogueParticipantsBP(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	if (RemoveDialogueParticipants(NewParticipants))
	{
		DialogueContextUpdatedFromBlueprint.Broadcast(this);
		return true;
	}

	return false;
}


