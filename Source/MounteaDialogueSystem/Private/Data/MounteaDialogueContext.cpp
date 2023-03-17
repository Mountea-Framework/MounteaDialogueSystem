// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueContext.h"


bool UMounteaDialogueContext::IsValid() const
{
	return ActiveNode != nullptr && DialogueParticipant.GetInterface() != nullptr && PlayerDialogueParticipant.GetInterface() != nullptr;
}

void UMounteaDialogueContext::SetDialogueContext(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, const TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	DialogueParticipant = NewParticipant;
	ActiveNode = NewActiveNode;
	AllowedChildNodes = NewAllowedChildNodes;
}

void UMounteaDialogueContext::UpdateDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	DialogueParticipant = NewParticipant;
}

void UMounteaDialogueContext::UpdateActiveDialogueNode(UMounteaDialogueGraphNode* NewActiveNode)
{
	ActiveNode = NewActiveNode;
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
}

void UMounteaDialogueContext::UpdateActiveDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	if (NewParticipant.GetInterface() == nullptr)
	{
		ActiveDialogueParticipant = NewParticipant;
		return;
	}

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
		TraversedPath[TraversedNode->GetNodeGUID()]++;
	}
	else
	{
		TraversedPath.Add(TraversedNode->GetNodeGUID(), 1);
	}
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
