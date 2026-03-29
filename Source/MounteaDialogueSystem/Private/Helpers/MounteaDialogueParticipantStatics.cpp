// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Helpers/MounteaDialogueParticipantStatics.h"

#include "Settings/MounteaDialogueConfiguration.h"
#include "Settings/MounteaDialogueSystemSettings.h"

bool UMounteaDialogueParticipantStatics::CanStartDialogue(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_CanStartDialogue(Target.GetObject()) : false;
}

bool UMounteaDialogueParticipantStatics::CanParticipateInDialogue(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_CanParticipateInDialogue(Target.GetObject()) : false;
}

AActor* UMounteaDialogueParticipantStatics::GetOwningActor(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetOwningActor(Target.GetObject()) : nullptr;
}

FGameplayTag UMounteaDialogueParticipantStatics::GetParticipantTag(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetParticipantTag(Target.GetObject()) : FGameplayTag::EmptyTag;
}

void UMounteaDialogueParticipantStatics::InitializeParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (Target.GetObject()) Target->Execute_InitializeParticipant(Target.GetObject(), Manager);
}

// Participant State functions
EDialogueParticipantState UMounteaDialogueParticipantStatics::GetParticipantState(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetParticipantState(Target.GetObject()) : EDialogueParticipantState{};
}

void UMounteaDialogueParticipantStatics::SetParticipantState(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const EDialogueParticipantState NewState)
{
	if (Target.GetObject()) Target->Execute_SetParticipantState(Target.GetObject(), NewState);
}

EDialogueParticipantState UMounteaDialogueParticipantStatics::GetDefaultParticipantState(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDefaultParticipantState(Target.GetObject()) : EDialogueParticipantState{};
}

void UMounteaDialogueParticipantStatics::SetDefaultParticipantState(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const EDialogueParticipantState NewState)
{
	if (Target.GetObject()) Target->Execute_SetDefaultParticipantState(Target.GetObject(), NewState);
}

TScriptInterface<IMounteaDialogueManagerInterface> UMounteaDialogueParticipantStatics::GetDialogueManager(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->GetDialogueManager() : nullptr;
}

// Node functions
void UMounteaDialogueParticipantStatics::SaveStartingNode(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UMounteaDialogueGraphNode* NewStartingNode)
{
	if (Target.GetObject()) Target->Execute_SaveStartingNode(Target.GetObject(), NewStartingNode);
}

void UMounteaDialogueParticipantStatics::SaveTraversedPath(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, TArray<FDialogueTraversePath>& InPath)
{
	if (Target.GetObject()) Target->Execute_SaveTraversedPath(Target.GetObject(), InPath);
}

UMounteaDialogueGraphNode* UMounteaDialogueParticipantStatics::GetSavedStartingNode(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetSavedStartingNode(Target.GetObject()) : nullptr;
}

// Audio functions
void UMounteaDialogueParticipantStatics::PlayParticipantVoice(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, USoundBase* ParticipantVoice)
{
	if (Target.GetObject()) Target->Execute_PlayParticipantVoice(Target.GetObject(), ParticipantVoice);
}

void UMounteaDialogueParticipantStatics::SkipParticipantVoice(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, USoundBase* ParticipantVoice)
{
	if (Target.GetObject()) Target->Execute_SkipParticipantVoice(Target.GetObject(), ParticipantVoice);
}

UAudioComponent* UMounteaDialogueParticipantStatics::GetAudioComponent(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetAudioComponent(Target.GetObject()) : nullptr;
}

void UMounteaDialogueParticipantStatics::SetAudioComponent(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UAudioComponent* NewAudioComponent)
{
	if (Target.GetObject()) Target->Execute_SetAudioComponent(Target.GetObject(), NewAudioComponent);
}

// Graph functions
UMounteaDialogueGraph* UMounteaDialogueParticipantStatics::GetDialogueGraph(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueGraph(Target.GetObject()) : nullptr;
}

void UMounteaDialogueParticipantStatics::SetDialogueGraph(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UMounteaDialogueGraph* NewDialogueGraph)
{
	if (Target.GetObject()) Target->Execute_SetDialogueGraph(Target.GetObject(), NewDialogueGraph);
}

TArray<FDialogueTraversePath> UMounteaDialogueParticipantStatics::GetTraversedPath(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetTraversedPath(Target.GetObject()) : TArray<FDialogueTraversePath>{};
}

// Command functions
void UMounteaDialogueParticipantStatics::ProcessDialogueCommand(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const FString& Command, UObject* Payload)
{
	if (Target.GetObject()) Target->Execute_ProcessDialogueCommand(Target.GetObject(), Command, Payload);
}

const FDialogueParticipant* UMounteaDialogueParticipantStatics::FindParticipantDataRow(const FName& ParticipantRow, FGameplayTag* OutParticipantTag)
{
	if (ParticipantRow.IsNone())
		return nullptr;

	const UMounteaDialogueSystemSettings* dialogueSettings = GetDefault<UMounteaDialogueSystemSettings>();
	if (!IsValid(dialogueSettings))
		return nullptr;

	const UMounteaDialogueConfiguration* dialogueConfig = dialogueSettings->GetDialogueConfiguration().LoadSynchronous();
	if (!IsValid(dialogueConfig))
		return nullptr;

	for (const TSoftObjectPtr<UDataTable>& dialogueParticipantsTable : dialogueConfig->DialogueParticipantsTables)
	{
		const UDataTable* participantsTable = dialogueParticipantsTable.LoadSynchronous();
		if (!IsValid(participantsTable))
			continue;

		const FDialogueParticipant* dialogueParticipantRow = participantsTable->FindRow<FDialogueParticipant>(ParticipantRow, TEXT(""));
		if (!dialogueParticipantRow)
			continue;

		if (OutParticipantTag != nullptr)
			*OutParticipantTag = dialogueParticipantRow->ParticipantCategoryTag;

		return dialogueParticipantRow;
	}

	return nullptr;
}

TArray<FName> UMounteaDialogueParticipantStatics::GetDialogueParticipantRowNames()
{
	const UMounteaDialogueSystemSettings* dialogueSettings = GetDefault<UMounteaDialogueSystemSettings>();
	if (!IsValid(dialogueSettings))
		return {};

	const UMounteaDialogueConfiguration* dialogueConfig = dialogueSettings->GetDialogueConfiguration().LoadSynchronous();
	if (!IsValid(dialogueConfig))
		return {};

	TSet<FName> uniqueParticipants;
	uniqueParticipants.Reserve(dialogueConfig->DialogueParticipantsTables.Num() * 4);

	for (const TSoftObjectPtr<UDataTable>& participantsTablePtr : dialogueConfig->DialogueParticipantsTables)
	{
		const UDataTable* participantsTable = participantsTablePtr.LoadSynchronous();
		if (!IsValid(participantsTable))
			continue;

		uniqueParticipants.Append(participantsTable->GetRowNames());
	}

	TArray<FName> sortedParticipants = uniqueParticipants.Array();
	sortedParticipants.Sort([](const FName& left, const FName& right)
	{
		return left.LexicalLess(right);
	});
	return sortedParticipants;
}
