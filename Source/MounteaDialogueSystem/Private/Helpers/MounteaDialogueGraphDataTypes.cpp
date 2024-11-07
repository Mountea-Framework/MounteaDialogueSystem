// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueGraphDataTypes.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"

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
	: ActiveDialogueParticipant(Source ? Source->ActiveDialogueParticipant : nullptr)
	, PlayerDialogueParticipant(Source ? Source->PlayerDialogueParticipant : nullptr)
	, DialogueParticipant(Source ? Source->DialogueParticipant : nullptr)
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
				DialogueParticipants.Add(Participant);
		}
	}
}

FMounteaDialogueContextReplicatedStruct FMounteaDialogueContextReplicatedStruct::operator+=(UMounteaDialogueContext* Source)
{
	*this = FMounteaDialogueContextReplicatedStruct(Source);
	return *this;
}

bool FMounteaDialogueContextReplicatedStruct::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << ActiveNodeGuid;
	Ar << PreviousActiveNodeGuid;
	
	auto DataTable = ActiveDialogueTableHandle.DataTable;
	FName RowName = ActiveDialogueTableHandle.RowName;
	
	Ar << DataTable;
	Ar << RowName;
	
	if (Ar.IsLoading())
	{
		ActiveDialogueTableHandle.DataTable = DataTable;
		ActiveDialogueTableHandle.RowName = RowName;
	}

	Ar << ActiveDialogueRowDataIndex;

	// For TScriptInterface properties, we need to serialize the raw UObject* pointers
	UObject* ActiveParticipantObj = nullptr;
	UObject* PlayerParticipantObj = nullptr;
	UObject* DialogueParticipantObj = nullptr;

	if (Ar.IsSaving())
	{
		ActiveParticipantObj = ActiveDialogueParticipant.GetObject();
		PlayerParticipantObj = PlayerDialogueParticipant.GetObject();
		DialogueParticipantObj = DialogueParticipant.GetObject();
	}
	
	Ar << ActiveParticipantObj;
	Ar << PlayerParticipantObj;
	Ar << DialogueParticipantObj;

	if (Ar.IsLoading())
	{
		// Reconstruct the TScriptInterfaces from the loaded objects
		ActiveDialogueParticipant = TScriptInterface<IMounteaDialogueParticipantInterface>(ActiveParticipantObj);
		PlayerDialogueParticipant = TScriptInterface<IMounteaDialogueParticipantInterface>(PlayerParticipantObj);
		DialogueParticipant = TScriptInterface<IMounteaDialogueParticipantInterface>(DialogueParticipantObj);
	}

	if (Ar.IsSaving())
	{
		int32 NumParticipants = DialogueParticipants.Num();
		Ar << NumParticipants;

		for (const auto& Participant : DialogueParticipants)
		{
			UObject* ParticipantObj = Participant.GetObject();
			Ar << ParticipantObj;
		}
	}
	else if (Ar.IsLoading())
	{
		int32 NumParticipants;
		Ar << NumParticipants;
		
		DialogueParticipants.Empty(NumParticipants);
		
		for (int32 i = 0; i < NumParticipants; ++i)
		{
			UObject* ParticipantObj = nullptr;
			Ar << ParticipantObj;
			
			if (ParticipantObj)
			{
				DialogueParticipants.Add(TScriptInterface<IMounteaDialogueParticipantInterface>(ParticipantObj));
			}
		}
	}

	Ar << AllowedChildNodes;
	
	bOutSuccess = true;
	return true;
}

FString FMounteaDialogueContextReplicatedStruct::ToString() const
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
	activeNode.Append(ActiveNodeGuid.IsValid() ? ActiveNodeGuid.ToString() : TEXT("invalid"));

	returnValue
		.Append(activeDialoguePart).Append(TEXT("\n"))
		.Append(playerDialoguePart).Append(TEXT("\n"))
		.Append(otherDialoguePart).Append(TEXT("\n"))
		.Append(allDialogueParts).Append(TEXT("\n"))
		.Append(activeNode).Append(TEXT("\n"));

	return returnValue;
}

bool FMounteaDialogueContextReplicatedStruct::IsValid() const
{
	return ActiveDialogueParticipant.GetObject()
			&& ActiveNodeGuid.IsValid()
			&& DialogueParticipants.Num() > 0;
}

void FMounteaDialogueContextReplicatedStruct::Reset()
{
	*this = FMounteaDialogueContextReplicatedStruct();
}
