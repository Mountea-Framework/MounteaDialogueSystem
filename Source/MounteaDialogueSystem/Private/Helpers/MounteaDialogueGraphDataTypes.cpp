// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueGraphDataTypes.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

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

bool FDialogueRow::IsValid() const
{
	return
	RowGUID.IsValid() &&
	DialogueRowData.Num() > 0;
}
/*
void FDialogueRow::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
	FTableRowBase::OnDataTableChanged(InDataTable, InRowName);

	if (!InDataTable)
	{
		return;
	}

	const FName* FoundRowName = nullptr;
	
	for (const auto& Pair : InDataTable->GetRowMap())
	{
		const FDialogueRow* RowPtr = reinterpret_cast<const FDialogueRow*>(Pair.Value);
		if (RowPtr == this)
		{
			FoundRowName = &Pair.Key;
			break;
		}
	}

	if (FoundRowName)
	{
		if (*FoundRowName != InRowName)
		{
			RowGUID = FGuid::NewGuid();
			UE_LOG(LogTemp, Warning, TEXT("Updated GUID"))
		}
	}
}
*/
FMounteaDialogueContextReplicatedStruct::FMounteaDialogueContextReplicatedStruct()
	: ActiveDialogueParticipant(nullptr)
	, PlayerDialogueParticipant(nullptr)
	, DialogueParticipant(nullptr)
	, ActiveNodeGuid(FGuid::NewGuid())
	, PreviousActiveNodeGuid(FGuid::NewGuid())
	, AllowedChildNodes(TArray<FGuid>())
	, ActiveDialogueRowDataIndex(0)
{}

FMounteaDialogueContextReplicatedStruct::FMounteaDialogueContextReplicatedStruct(UMounteaDialogueContext* Source)
	: ActiveDialogueParticipant(Source ? Source->ActiveDialogueParticipant : nullptr)
	, PlayerDialogueParticipant(Source ? Source->PlayerDialogueParticipant : nullptr)
	, DialogueParticipant(Source ? Source->DialogueParticipant : nullptr)
	, DialogueParticipants(Source ? Source->DialogueParticipants : TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>())
	, ActiveNodeGuid(Source ? ( Source->ActiveNode ? Source->ActiveNode->GetNodeGUID() : FGuid::NewGuid() ) : FGuid::NewGuid())
	, PreviousActiveNodeGuid( Source ? Source->PreviousActiveNode : FGuid::NewGuid() )
	, AllowedChildNodes(Source ? UMounteaDialogueSystemBFC::NodesToGuids(Source->AllowedChildNodes) : TArray<FGuid>())
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
	ActiveNodeGuid = Source->ActiveNode ? Source->ActiveNode->GetNodeGUID() : FGuid::NewGuid();
	PreviousActiveNodeGuid = Source ? Source->PreviousActiveNode : FGuid::NewGuid();
	AllowedChildNodes = UMounteaDialogueSystemBFC::NodesToGuids(Source->AllowedChildNodes);
	ActiveDialogueRowDataIndex = Source->ActiveDialogueRowDataIndex;
}

bool FMounteaDialogueContextReplicatedStruct::IsValid() const
{
	return PlayerDialogueParticipant != nullptr && DialogueParticipant != nullptr && ActiveNodeGuid.IsValid() && DialogueParticipants.Num() != 0;
}
