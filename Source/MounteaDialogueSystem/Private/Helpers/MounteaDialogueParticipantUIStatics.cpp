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

#include "Helpers/MounteaDialogueParticipantUIStatics.h"

#include "GameFramework/Actor.h"

TScriptInterface<IMounteaDialogueParticipantUIInterface>
UMounteaDialogueParticipantUIStatics::FindParticipantUIComponent(AActor* ForActor)
{
	if (!IsValid(ForActor))
		return nullptr;

	for (UActorComponent* component : ForActor->GetComponents())
	{
		if (component && component->Implements<UMounteaDialogueParticipantUIInterface>())
			return TScriptInterface<IMounteaDialogueParticipantUIInterface>(component);
	}

	return nullptr;
}

void UMounteaDialogueParticipantUIStatics::SetParentManager(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager)
{
	if (Target.GetObject())
		Execute_SetParentManager(Target.GetObject(), NewManager);
}

TScriptInterface<IMounteaDialogueManagerInterface> UMounteaDialogueParticipantUIStatics::GetParentManager(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target)
{
	if (Target.GetObject())
		return Execute_GetParentManager(Target.GetObject());
	return nullptr;
}

void UMounteaDialogueParticipantUIStatics::BindToManager(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (Target.GetObject())
		Execute_BindToManager(Target.GetObject(), Manager);
}

void UMounteaDialogueParticipantUIStatics::UnbindFromManager(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target)
{
	if (Target.GetObject())
		Execute_UnbindFromManager(Target.GetObject());
}

void UMounteaDialogueParticipantUIStatics::SetUserInterface(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	UObject* NewUserInterface)
{
	if (Target.GetObject())
		Execute_SetUserInterface(Target.GetObject(), NewUserInterface);
}

UObject* UMounteaDialogueParticipantUIStatics::GetUserInterface(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target)
{
	if (Target.GetObject())
		return Execute_GetUserInterface(Target.GetObject());
	return nullptr;
}

bool UMounteaDialogueParticipantUIStatics::CreateDialogueUI(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	FString& Message)
{
	if (Target.GetObject())
		return Execute_CreateDialogueUI(Target.GetObject(), Message);
	Message = TEXT("Invalid UI component target.");
	return false;
}

bool UMounteaDialogueParticipantUIStatics::UpdateDialogueUI(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	FString& Message,
	const FString& Command)
{
	if (Target.GetObject())
		return Execute_UpdateDialogueUI(Target.GetObject(), Message, Command);
	Message = TEXT("Invalid UI component target.");
	return false;
}

bool UMounteaDialogueParticipantUIStatics::CloseDialogueUI(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target)
{
	if (Target.GetObject())
		return Execute_CloseDialogueUI(Target.GetObject());
	return false;
}

void UMounteaDialogueParticipantUIStatics::ExecuteWidgetCommand(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	const FString& Command)
{
	if (Target.GetObject())
		Execute_ExecuteWidgetCommand(Target.GetObject(), Command);
}

void UMounteaDialogueParticipantUIStatics::RequestSelectNode(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	const FGuid& NodeGuid)
{
	if (Target.GetObject())
		Execute_RequestSelectNode(Target.GetObject(), NodeGuid);
}

void UMounteaDialogueParticipantUIStatics::RequestSkipDialogueRow(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target)
{
	if (Target.GetObject())
		Execute_RequestSkipDialogueRow(Target.GetObject());
}

void UMounteaDialogueParticipantUIStatics::RequestCloseDialogue(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target)
{
	if (Target.GetObject())
		Execute_RequestCloseDialogue(Target.GetObject());
}

void UMounteaDialogueParticipantUIStatics::RequestProcessDialogueRow(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target)
{
	if (Target.GetObject())
		Execute_RequestProcessDialogueRow(Target.GetObject());
}

void UMounteaDialogueParticipantUIStatics::DispatchUISignal(
	const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
	const FMounteaDialogueUISignal& Signal)
{
	if (Target.GetObject())
		Execute_DispatchUISignal(Target.GetObject(), Signal);
}
