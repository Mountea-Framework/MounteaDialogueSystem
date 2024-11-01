// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Helpers/MounteaDialogueParticipantStatics.h"

bool UMounteaDialogueParticipantStatics::CanStartDialogue(UObject* Target)
{
	return ExecuteIfImplements<bool>(Target, TEXT("CanStartDialogue"), &IMounteaDialogueParticipantInterface::Execute_CanStartDialogue);
}

bool UMounteaDialogueParticipantStatics::CanStartDialogueV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_CanStartDialogue(Target.GetObject()) : false;
}

AActor* UMounteaDialogueParticipantStatics::GetOwningActor(UObject* Target)
{
	return ExecuteIfImplements<AActor*>(Target, TEXT("GetOwningActor"), &IMounteaDialogueParticipantInterface::Execute_GetOwningActor);
}

AActor* UMounteaDialogueParticipantStatics::GetOwningActorV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetOwningActor(Target.GetObject()) : nullptr;
}

FGameplayTag UMounteaDialogueParticipantStatics::GetParticipantTag(UObject* Target)
{
	return ExecuteIfImplements<FGameplayTag>(Target, TEXT("GetParticipantTag"), &IMounteaDialogueParticipantInterface::Execute_GetParticipantTag);
}

FGameplayTag UMounteaDialogueParticipantStatics::GetParticipantTagV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetParticipantTag(Target.GetObject()) : FGameplayTag::EmptyTag;
}

void UMounteaDialogueParticipantStatics::InitializeParticipant(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("InitializeParticipant"), &IMounteaDialogueParticipantInterface::Execute_InitializeParticipant);
}

void UMounteaDialogueParticipantStatics::InitializeParticipantV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_InitializeParticipant(Target.GetObject());
}

// Participant State functions
EDialogueParticipantState UMounteaDialogueParticipantStatics::GetParticipantState(UObject* Target)
{
	return ExecuteIfImplements<EDialogueParticipantState>(Target, TEXT("GetParticipantState"), &IMounteaDialogueParticipantInterface::Execute_GetParticipantState);
}

EDialogueParticipantState UMounteaDialogueParticipantStatics::GetParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetParticipantState(Target.GetObject()) : EDialogueParticipantState{};
}

void UMounteaDialogueParticipantStatics::SetParticipantState(UObject* Target, const EDialogueParticipantState NewState)
{
	ExecuteIfImplements<void>(Target, TEXT("SetParticipantState"), &IMounteaDialogueParticipantInterface::Execute_SetParticipantState, NewState);
}

void UMounteaDialogueParticipantStatics::SetParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const EDialogueParticipantState NewState)
{
	if (Target.GetObject()) Target->Execute_SetParticipantState(Target.GetObject(), NewState);
}

EDialogueParticipantState UMounteaDialogueParticipantStatics::GetDefaultParticipantState(UObject* Target)
{
	return ExecuteIfImplements<EDialogueParticipantState>(Target, TEXT("GetDefaultParticipantState"), &IMounteaDialogueParticipantInterface::Execute_GetDefaultParticipantState);
}

EDialogueParticipantState UMounteaDialogueParticipantStatics::GetDefaultParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDefaultParticipantState(Target.GetObject()) : EDialogueParticipantState{};
}

void UMounteaDialogueParticipantStatics::SetDefaultParticipantState(UObject* Target, const EDialogueParticipantState NewState)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDefaultParticipantState"), &IMounteaDialogueParticipantInterface::Execute_SetDefaultParticipantState, NewState);
}

void UMounteaDialogueParticipantStatics::SetDefaultParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const EDialogueParticipantState NewState)
{
	if (Target.GetObject()) Target->Execute_SetDefaultParticipantState(Target.GetObject(), NewState);
}

// Node functions
void UMounteaDialogueParticipantStatics::SaveStartingNode(UObject* Target, UMounteaDialogueGraphNode* NewStartingNode)
{
	ExecuteIfImplements<void>(Target, TEXT("SaveStartingNode"), &IMounteaDialogueParticipantInterface::Execute_SaveStartingNode, NewStartingNode);
}

void UMounteaDialogueParticipantStatics::SaveStartingNodeV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UMounteaDialogueGraphNode* NewStartingNode)
{
	if (Target.GetObject()) Target->Execute_SaveStartingNode(Target.GetObject(), NewStartingNode);
}

void UMounteaDialogueParticipantStatics::SaveTraversedPath(UObject* Target, TArray<FDialogueTraversePath>& InPath)
{
	ExecuteIfImplements<void>(Target, TEXT("SaveTraversedPath"), &IMounteaDialogueParticipantInterface::Execute_SaveTraversedPath, InPath);
}

void UMounteaDialogueParticipantStatics::SaveTraversedPathV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, TArray<FDialogueTraversePath>& InPath)
{
	if (Target.GetObject()) Target->Execute_SaveTraversedPath(Target.GetObject(), InPath);
}

UMounteaDialogueGraphNode* UMounteaDialogueParticipantStatics::GetSavedStartingNode(UObject* Target)
{
	return ExecuteIfImplements<UMounteaDialogueGraphNode*>(Target, TEXT("GetSavedStartingNode"), &IMounteaDialogueParticipantInterface::Execute_GetSavedStartingNode);
}

UMounteaDialogueGraphNode* UMounteaDialogueParticipantStatics::GetSavedStartingNodeV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetSavedStartingNode(Target.GetObject()) : nullptr;
}

// Audio functions
void UMounteaDialogueParticipantStatics::PlayParticipantVoice(UObject* Target, USoundBase* ParticipantVoice)
{
	ExecuteIfImplements<void>(Target, TEXT("PlayParticipantVoice"), &IMounteaDialogueParticipantInterface::Execute_PlayParticipantVoice, ParticipantVoice);
}

void UMounteaDialogueParticipantStatics::PlayParticipantVoiceV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, USoundBase* ParticipantVoice)
{
	if (Target.GetObject()) Target->Execute_PlayParticipantVoice(Target.GetObject(), ParticipantVoice);
}

void UMounteaDialogueParticipantStatics::SkipParticipantVoice(UObject* Target, USoundBase* ParticipantVoice)
{
	ExecuteIfImplements<void>(Target, TEXT("SkipParticipantVoice"), &IMounteaDialogueParticipantInterface::Execute_SkipParticipantVoice, ParticipantVoice);
}

void UMounteaDialogueParticipantStatics::SkipParticipantVoiceV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, USoundBase* ParticipantVoice)
{
	if (Target.GetObject()) Target->Execute_SkipParticipantVoice(Target.GetObject(), ParticipantVoice);
}

UAudioComponent* UMounteaDialogueParticipantStatics::GetAudioComponent(UObject* Target)
{
	return ExecuteIfImplements<UAudioComponent*>(Target, TEXT("GetAudioComponent"), &IMounteaDialogueParticipantInterface::Execute_GetAudioComponent);
}

UAudioComponent* UMounteaDialogueParticipantStatics::GetAudioComponentV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetAudioComponent(Target.GetObject()) : nullptr;
}

void UMounteaDialogueParticipantStatics::SetAudioComponent(UObject* Target, UAudioComponent* NewAudioComponent)
{
	ExecuteIfImplements<void>(Target, TEXT("SetAudioComponent"), &IMounteaDialogueParticipantInterface::Execute_SetAudioComponent, NewAudioComponent);
}

void UMounteaDialogueParticipantStatics::SetAudioComponentV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UAudioComponent* NewAudioComponent)
{
	if (Target.GetObject()) Target->Execute_SetAudioComponent(Target.GetObject(), NewAudioComponent);
}

// Graph functions
UMounteaDialogueGraph* UMounteaDialogueParticipantStatics::GetDialogueGraph(UObject* Target)
{
	return ExecuteIfImplements<UMounteaDialogueGraph*>(Target, TEXT("GetDialogueGraph"), &IMounteaDialogueParticipantInterface::Execute_GetDialogueGraph);
}

UMounteaDialogueGraph* UMounteaDialogueParticipantStatics::GetDialogueGraphV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueGraph(Target.GetObject()) : nullptr;
}

void UMounteaDialogueParticipantStatics::SetDialogueGraph(UObject* Target, UMounteaDialogueGraph* NewDialogueGraph)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueGraph"), &IMounteaDialogueParticipantInterface::Execute_SetDialogueGraph, NewDialogueGraph);
}

void UMounteaDialogueParticipantStatics::SetDialogueGraphV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UMounteaDialogueGraph* NewDialogueGraph)
{
	if (Target.GetObject()) Target->Execute_SetDialogueGraph(Target.GetObject(), NewDialogueGraph);
}

TArray<FDialogueTraversePath> UMounteaDialogueParticipantStatics::GetTraversedPath(UObject* Target)
{
	return ExecuteIfImplements<TArray<FDialogueTraversePath>>(Target, TEXT("GetTraversedPath"), &IMounteaDialogueParticipantInterface::Execute_GetTraversedPath);
}

TArray<FDialogueTraversePath> UMounteaDialogueParticipantStatics::GetTraversedPathV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetTraversedPath(Target.GetObject()) : TArray<FDialogueTraversePath>{};
}

// Command functions
void UMounteaDialogueParticipantStatics::ProcessDialogueCommand(UObject* Target, const FString& Command, UObject* Payload)
{
	ExecuteIfImplements<void>(Target, TEXT("ProcessDialogueCommand"), &IMounteaDialogueParticipantInterface::Execute_ProcessDialogueCommand, Command, Payload);
}

void UMounteaDialogueParticipantStatics::ProcessDialogueCommandV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const FString& Command, UObject* Payload)
{
	if (Target.GetObject()) Target->Execute_ProcessDialogueCommand(Target.GetObject(), Command, Payload);
}