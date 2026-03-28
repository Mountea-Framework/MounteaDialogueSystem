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
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

bool FMounteaDialogueContextPayload::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << SessionGUID;
	Ar << ActiveNodeGUID;
	Ar << PreviousNodeGUID;
	Ar << AllowedChildNodeGUIDs;
	Ar << ActiveGraphGUID;
	Ar << ActiveDialogueParticipant;
	Ar << PlayerDialogueParticipant;
	Ar << DialogueParticipant;
	Ar << DialogueParticipants;

	// FDialogueRow — field-by-field
	ActiveDialogueRow.CompatibleTags.NetSerialize(Ar, Map, bOutSuccess);
	Ar << ActiveDialogueRow.UIRowID;
	Ar << ActiveDialogueRow.DialogueParticipant;
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
	Ar << LastWidgetCommand;
	Ar << ContextVersion;

	bOutSuccess = true;
	return true;
}
