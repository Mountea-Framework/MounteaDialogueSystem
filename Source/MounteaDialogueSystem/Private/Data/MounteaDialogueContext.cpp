// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueContext.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"
#include "Net/UnrealNetwork.h"


FString UMounteaDialogueContext::ToString() const
{
	FString returnValue;

	FString activeDialoguePart = FString("Active Dialogue Participant: ");
	activeDialoguePart.Append(ActiveDialogueParticipant.GetObject() ? ActiveDialogueParticipant->Execute_GetParticipantTag(ActiveDialogueParticipant.GetObject()).ToString() : TEXT("invalid"));

	FString playerDialoguePart = FString("Player Dialogue Participant: ");
	playerDialoguePart.Append(PlayerDialogueParticipant.GetObject() ? PlayerDialogueParticipant->Execute_GetParticipantTag(PlayerDialogueParticipant.GetObject()).ToString() : TEXT("invalid"));

	FString otherDialoguePart = FString("Other Dialogue Participant: ");
	otherDialoguePart.Append(DialogueParticipant.GetObject() ? DialogueParticipant->Execute_GetParticipantTag(DialogueParticipant.GetObject()).ToString() : TEXT("invalid"));

	FString allDialogueParts = FString("Dialogue Participants: ");
	allDialogueParts.Append(FString::Printf(TEXT("%d"), DialogueParticipants.Num()));

	FString activeNode = FString("Active Node ID: ");
	activeNode.Append(ActiveNode ? ActiveNode->GetNodeGUID().ToString() : TEXT("invalid"));

	FString activeRow = FString("Active Row: ");
	activeRow.Append(ActiveDialogueRow.RowTitle.ToString());

	FString activeRowData = FString("Active Row Data: ");
	activeRowData.Append(FString::Printf(TEXT("%d"), ActiveDialogueRow.DialogueRowData.Num()));

	returnValue
		.Append(activeDialoguePart).Append(TEXT("\n"))
		.Append(playerDialoguePart).Append(TEXT("\n"))
		.Append(otherDialoguePart).Append(TEXT("\n"))
		.Append(allDialogueParts).Append(TEXT("\n"))
		.Append(activeNode).Append(TEXT("\n"))
		.Append(activeRow).Append(TEXT("\n"))
		.Append(activeRowData);

	return returnValue;
}

bool UMounteaDialogueContext::IsValid() const
{
	return ActiveNode != nullptr && DialogueParticipant.GetInterface() != nullptr && PlayerDialogueParticipant.GetInterface() != nullptr;
}

void UMounteaDialogueContext::SetDialogueContext(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, const TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	DialogueParticipant = NewParticipant;
	
	if (ActiveNode && ActiveNode->GetNodeGUID() != PreviousActiveNode)
	{
		PreviousActiveNode = ActiveNode->GetNodeGUID();
	}
	
	ActiveNode = NewActiveNode;
	AllowedChildNodes = NewAllowedChildNodes;

	if (!DialogueParticipants.Contains(NewParticipant))
	{
		DialogueParticipants.Add(NewParticipant);
	}
}

void UMounteaDialogueContext::UpdateDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	DialogueParticipant = NewParticipant;

	AddDialogueParticipant(NewParticipant);
}

void UMounteaDialogueContext::UpdateActiveDialogueNode(UMounteaDialogueGraphNode* NewActiveNode)
{
	if (ActiveNode && ActiveNode->GetNodeGUID() != PreviousActiveNode)
	{
		PreviousActiveNode = ActiveNode->GetNodeGUID();
	}
	
	ActiveNode = NewActiveNode;
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

void UMounteaDialogueContext::UpdateDialoguePlayerParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	PlayerDialogueParticipant = NewParticipant;
	
	AddDialogueParticipant(NewParticipant);
}

void UMounteaDialogueContext::UpdateActiveDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	if (NewParticipant != PlayerDialogueParticipant && NewParticipant != DialogueParticipant)
	{
		//TODO: Properly log this
		return;
	}

	ActiveDialogueParticipant = NewParticipant;
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

void UMounteaDialogueContext::SetDialogueContextBP(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode,TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	SetDialogueContext(NewParticipant, NewActiveNode, NewAllowedChildNodes);

	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

void UMounteaDialogueContext::UpdateDialogueParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	UpdateDialogueParticipant(NewParticipant);
	
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

void UMounteaDialogueContext::UpdateDialoguePlayerParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	UpdateDialoguePlayerParticipant(NewParticipant);

	DialogueContextUpdatedFromBlueprint.Broadcast(this);
}

void UMounteaDialogueContext::UpdateActiveDialogueParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	UpdateActiveDialogueParticipant(NewParticipant);

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

void UMounteaDialogueContext::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
