// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueContext.h"


bool UMounteaDialogueContext::IsValid() const
{
	return ActiveNode != nullptr && DialogueParticipant.GetInterface() != nullptr && PlayerDialogueParticipant.GetInterface() != nullptr;
}

void UMounteaDialogueContext::SetDialogueContext(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	DialogueParticipant = NewParticipant;
	ActiveNode = NewActiveNode;
	AllowedChildNodes = NewAllowedChildNodes;
}

void UMounteaDialogueContext::UpdateDialogueParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
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

void UMounteaDialogueContext::UpdateDialoguePlayerParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
{
	PlayerDialogueParticipant = NewParticipant;
}

void UMounteaDialogueContext::UpdateActiveDialogueParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant)
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
