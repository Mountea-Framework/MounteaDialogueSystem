// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueContext.h"

#include "Interfaces/MounteaDialogueParticipantInterface.h"
#include "Net/UnrealNetwork.h"


bool UMounteaDialogueContext::IsValid() const
{
	return ActiveNode != nullptr && DialogueParticipant.GetInterface() != nullptr && PlayerDialogueParticipant.GetInterface() != nullptr;
}

void UMounteaDialogueContext::SetDialogueContext(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, const TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	DialogueParticipant = NewParticipant;
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
	ActiveNode = NewActiveNode;
}

void UMounteaDialogueContext::UpdateAllowedChildrenNodes(const TArray<UMounteaDialogueGraphNode*>& NewNodes)
{
	AllowedChildNodes = NewNodes;
}

void UMounteaDialogueContext::UpdateActiveDialogueRow(const FDialogueRow& NewActiveRow)
{
	ActiveDialogueRow = NewActiveRow;

	ActiveDialogueRow.SerializeDialogueRowData();
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
	if (!TraversedNode) return;
	
	// If we have already passed over this Node, then just increase the counter
	if (TraversedPath.Contains(TraversedNode->GetNodeGUID()))
	{
		if (FDialogueTraversePath* ExistingRow = TraversedPath.FindByKey(TraversedNode->GetNodeGUID()))
		{
			ExistingRow->TraverseCount++;
		}
	}
	else
	{
		FDialogueTraversePath NewRow;
		{
			NewRow.NodeGuid = TraversedNode->GetNodeGUID();
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

void UMounteaDialogueContext::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMounteaDialogueContext, ActiveDialogueParticipant);
	DOREPLIFETIME(UMounteaDialogueContext, PlayerDialogueParticipant);
	DOREPLIFETIME(UMounteaDialogueContext, DialogueParticipant);
	DOREPLIFETIME(UMounteaDialogueContext, DialogueParticipants);
	DOREPLIFETIME(UMounteaDialogueContext, ActiveNode);
	DOREPLIFETIME(UMounteaDialogueContext, AllowedChildNodes);
	DOREPLIFETIME(UMounteaDialogueContext, ActiveDialogueRow);
	DOREPLIFETIME(UMounteaDialogueContext, ActiveDialogueRowDataIndex);
	DOREPLIFETIME(UMounteaDialogueContext, TraversedPath);
}