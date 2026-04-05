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

#include "Data/MounteaDialogueContextPayload.h"
#include "GameFramework/Actor.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

static TScriptInterface<IMounteaDialogueParticipantInterface> ResolveParticipantFromActorInternal(AActor* ParticipantActor)
{
	bool bFoundParticipant = false;
	TScriptInterface<IMounteaDialogueParticipantInterface> participantInterface =
		UMounteaDialogueParticipantStatics::ResolveParticipantFromActor(ParticipantActor, bFoundParticipant);

	if (!bFoundParticipant || !participantInterface.GetObject() || !participantInterface.GetInterface())
		return nullptr;

	return participantInterface;
}

static void SerializeParticipantInterface(
	FArchive& Ar,
	UPackageMap* Map,
	TScriptInterface<IMounteaDialogueParticipantInterface>& Participant)
{
	UObject* participantActorObject = nullptr;
	if (Ar.IsSaving() && Participant.GetObject() && Participant.GetInterface())
	{
		AActor* owningActor = IMounteaDialogueParticipantInterface::Execute_GetOwningActor(Participant.GetObject());
		participantActorObject = owningActor;
	}

	Map->SerializeObject(Ar, AActor::StaticClass(), participantActorObject);

	if (Ar.IsLoading())
	{
		Participant = ResolveParticipantFromActorInternal(Cast<AActor>(participantActorObject));
	}
}

static void SerializeParticipantInterfaces(
	FArchive& Ar,
	UPackageMap* Map,
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& Participants)
{
	int32 participantCount = Participants.Num();
	Ar << participantCount;

	if (Ar.IsLoading())
		Participants.SetNum(participantCount);

	for (int32 index = 0; index < participantCount; index++)
	{
		SerializeParticipantInterface(Ar, Map, Participants[index]);
	}

	if (Ar.IsLoading())
	{
		Participants.RemoveAll(
			[](const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant)
			{
				return !Participant.GetObject() || !Participant.GetInterface();
			});
	}
}

bool FMounteaDialogueContextPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!Map)
	{
		bOutSuccess = false;
		return false;
	}

	Ar << SessionGUID;
	Ar << ActiveNodeGUID;
	Ar << PreviousNodeGUID;
	Ar << AllowedChildNodeGUIDs;
	Ar << ActiveGraphGUID;
	SerializeParticipantInterface(Ar, Map, ActiveDialogueParticipant);
	SerializeParticipantInterfaces(Ar, Map, DialogueParticipants);

	// FDialogueRow — field-by-field
	ActiveDialogueRow.CompatibleTags.NetSerialize(Ar, Map, bOutSuccess);
	Ar << ActiveDialogueRow.UIRowID;
	Ar << ActiveDialogueRow.DialogueParticipantName;
	Ar << ActiveDialogueRow.RowTitle;

	// TArray<FDialogueRowData> — each item serialized field-by-field due to UObject refs
	{
	int32 dataCount = ActiveDialogueRow.RowData.Num();
		Ar << dataCount;
		if (Ar.IsLoading())
	ActiveDialogueRow.RowData.SetNum(dataCount);

		for (int32 i = 0; i < dataCount; i++)
		{
		FDialogueRowData& dialogueRowData = ActiveDialogueRow.RowData[i];
			Ar << dialogueRowData.RowText;

			UObject* soundAsset = dialogueRowData.RowSound;
			Map->SerializeObject(Ar, USoundBase::StaticClass(), soundAsset);
			if (Ar.IsLoading())
				dialogueRowData.RowSound = Cast<USoundBase>(soundAsset);

			uint8 durationMode = static_cast<uint8>(dialogueRowData.RowDurationMode);
			Ar << durationMode;
			if (Ar.IsLoading())
				dialogueRowData.RowDurationMode = static_cast<ERowDurationMode>(durationMode);

			Ar << dialogueRowData.RowDuration;
			Ar << dialogueRowData.RowDurationOverride;

			uint8 execMode = static_cast<uint8>(dialogueRowData.RowExecutionBehaviour);
			Ar << execMode;
			if (Ar.IsLoading())
				dialogueRowData.RowExecutionBehaviour = static_cast<ERowExecutionMode>(execMode);

			Ar << dialogueRowData.RowGUID;
		}
	}

	// DialogueRowAdditionalData — static data asset, loaded on both client and server
	{
		UObject* additionalData = ActiveDialogueRow.DialogueRowAdditionalData;
		Map->SerializeObject(Ar, UObject::StaticClass(), additionalData);
		if (Ar.IsLoading())
			ActiveDialogueRow.DialogueRowAdditionalData = Cast<UDialogueAdditionalData>(additionalData);
	}

	Ar << ActiveDialogueRow.RowGUID;

	Ar << ActiveDialogueRowDataIndex;
	Ar << ContextVersion;

	bOutSuccess = true;
	return true;
}
