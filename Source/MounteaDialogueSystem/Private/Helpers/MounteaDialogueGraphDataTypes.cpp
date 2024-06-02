// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueGraphDataTypes.h"

#include "Data/MounteaDialogueContext.h"

/* Serialization is not needed, data can be found locally for Clients -> saving bandwith as well
void FDialogueRow::SerializeDialogueRowData()
{
	ReplicatedDialogueRowData.Reset();
	for (const FDialogueRowData& RowData : DialogueRowData)
	{
		ReplicatedDialogueRowData.Add(RowData);
	}
}

void FDialogueRow::DeserializeDialogueRowData()
{
	DialogueRowData.Reset();
	for (const FDialogueRowData& RowData : ReplicatedDialogueRowData)
	{
		DialogueRowData.Add(RowData);
	}
}
*/

FMounteaDialogueContextReplicatedStruct::FMounteaDialogueContextReplicatedStruct()
	: ActiveDialogueParticipant(nullptr)
	, PlayerDialogueParticipant(nullptr)
	, DialogueParticipant(nullptr)
	, ActiveNode(nullptr)
	, AllowedChildNodes(TArray<TObjectPtr<UMounteaDialogueGraphNode>>())
	, ActiveDialogueRowDataIndex(0)
{}

FMounteaDialogueContextReplicatedStruct::FMounteaDialogueContextReplicatedStruct(UMounteaDialogueContext* Source)
	: ActiveDialogueParticipant(Source ? Source->ActiveDialogueParticipant : nullptr)
	, PlayerDialogueParticipant(Source ? Source->PlayerDialogueParticipant : nullptr)
	, DialogueParticipant(Source ? Source->DialogueParticipant : nullptr)
	, DialogueParticipants(Source ? Source->DialogueParticipants : TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>())
	, ActiveNode(Source ? Source->ActiveNode : nullptr)
	, AllowedChildNodes(Source ? Source->AllowedChildNodes : TArray<TObjectPtr<UMounteaDialogueGraphNode>>())
	, ActiveDialogueRowDataIndex(Source ? Source->ActiveDialogueRowDataIndex : 0)
{
}


void FMounteaDialogueContextReplicatedStruct::SetData(UMounteaDialogueContext* Source)
{
	if (!Source) return;

	ActiveDialogueParticipant = Source->ActiveDialogueParticipant;
	PlayerDialogueParticipant = Source->PlayerDialogueParticipant;
	DialogueParticipant = Source->DialogueParticipant;
	DialogueParticipants = Source->DialogueParticipants;
	ActiveDialogueRowDataIndex = Source->ActiveDialogueRowDataIndex;
	ActiveNode = Source->ActiveNode;
	AllowedChildNodes = Source->AllowedChildNodes;
	ActiveDialogueRowDataIndex = Source->ActiveDialogueRowDataIndex;
}

bool FMounteaDialogueContextReplicatedStruct::IsValid() const
{
	return PlayerDialogueParticipant != nullptr && DialogueParticipant != nullptr && ActiveNode != nullptr && DialogueParticipants.Num() != 0;
}
