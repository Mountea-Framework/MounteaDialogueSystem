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

FString FDialogueRow::ToString() const
{
	FString Result;

	Result += FString::Printf(TEXT("RowTitle: %s"), *RowTitle.ToString());
	Result += FString::Printf(TEXT(", DialogueParticipant: %s"), *DialogueParticipant.ToString());
	Result += FString::Printf(TEXT(", UIRowID: %d"), UIRowID);
	Result += FString::Printf(TEXT(", RowGUID: %s"), *RowGUID.ToString(EGuidFormats::DigitsWithHyphensLower));

	if (RowOptionalIcon)
	{
		Result += TEXT(", RowOptionalIcon: Yes");
	}
	else
	{
		Result += TEXT(", RowOptionalIcon: No");
	}

	Result += FString::Printf(TEXT(", DialogueRowData Count: %d"), DialogueRowData.Num());

	return Result;
}


FMounteaDialogueContextReplicatedStruct::FMounteaDialogueContextReplicatedStruct()
	: ActiveDialogueParticipant(nullptr)
	, PlayerDialogueParticipant(nullptr)
	, DialogueParticipant(nullptr)
	, ActiveNodeGuid(FGuid::NewGuid())
	, PreviousActiveNodeGuid(FGuid::NewGuid())
	, AllowedChildNodes(TArray<FGuid>())
	, ActiveDialogueTableHandle(FDataTableRowHandle())
	, ActiveDialogueRowDataIndex(0)
{}

FMounteaDialogueContextReplicatedStruct::FMounteaDialogueContextReplicatedStruct(UMounteaDialogueContext* Source)
	: ActiveDialogueParticipant(Source ? Source->ActiveDialogueParticipant.GetObject() : nullptr)
	, PlayerDialogueParticipant(Source ? Source->PlayerDialogueParticipant.GetObject() : nullptr)
	, DialogueParticipant(Source ? Source->DialogueParticipant.GetObject() : nullptr)
	, ActiveNodeGuid(Source ? ( Source->ActiveNode ? Source->ActiveNode->GetNodeGUID() : FGuid() ) : FGuid())
	, PreviousActiveNodeGuid( Source ? Source->PreviousActiveNode : FGuid() )
	, AllowedChildNodes(Source ? UMounteaDialogueSystemBFC::NodesToGuids(Source->AllowedChildNodes) : TArray<FGuid>())
	, ActiveDialogueTableHandle(Source ? Source->ActiveDialogueTableHandle : FDataTableRowHandle())
	, ActiveDialogueRowDataIndex(Source ? Source->ActiveDialogueRowDataIndex : 0)
{
	DialogueParticipants.Empty();
	if (Source)
	{
		for (const auto& Participant : Source->DialogueParticipants)
		{
			if (Participant.GetObject())
			{
				DialogueParticipants.Add(Participant.GetObject());
			}
		}
	}
}

FMounteaDialogueContextReplicatedStruct FMounteaDialogueContextReplicatedStruct::operator+=(UMounteaDialogueContext* Source)
{
	*this = FMounteaDialogueContextReplicatedStruct(Source);
	return *this;
}

bool FMounteaDialogueContextReplicatedStruct::IsValid() const
{
	return ActiveDialogueParticipant != nullptr && ActiveNodeGuid.IsValid() && DialogueParticipants.Num() != 0;
}

void FMounteaDialogueContextReplicatedStruct::Reset()
{
	*this = FMounteaDialogueContextReplicatedStruct();
}
