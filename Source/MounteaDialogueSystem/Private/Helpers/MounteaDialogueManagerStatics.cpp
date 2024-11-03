﻿// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Helpers/MounteaDialogueManagerStatics.h"

AActor* UMounteaDialogueManagerStatics::GetOwningActor(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetOwningActor(Target.GetObject()) : nullptr;
}

UMounteaDialogueContext* UMounteaDialogueManagerStatics::GetDialogueContext(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueContext(Target.GetObject()) : nullptr;
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetManagerState(Target.GetObject()) : EDialogueManagerState::Default;
}

void UMounteaDialogueManagerStatics::SetDialogueManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const EDialogueManagerState NewState)
{
	if (Target.GetObject()) Target->SetManagerState(NewState);
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetDefaultDialogueManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDefaultManagerState(Target.GetObject()) : EDialogueManagerState::Default;
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObject(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* NewDialogueObject)
{
	return Target.GetObject() ? Target->Execute_AddDialogueUIObject(Target.GetObject(), NewDialogueObject) : false;
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects)
{
	return Target.GetObject() ? Target->Execute_AddDialogueUIObjects(Target.GetObject(), NewDialogueObjects) : false;
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObject(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* DialogueObjectToRemove)
{
	return Target.GetObject() ? Target->Execute_RemoveDialogueUIObject(Target.GetObject(), DialogueObjectToRemove) : false;
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& DialogueObjectsToRemove)
{
	return Target.GetObject() ? Target->Execute_RemoveDialogueUIObjects(Target.GetObject(), DialogueObjectsToRemove) : false;
}

void UMounteaDialogueManagerStatics::SetDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects)
{
	if (Target.GetObject()) Target->Execute_SetDialogueUIObjects(Target.GetObject(), NewDialogueObjects);
}

void UMounteaDialogueManagerStatics::ResetDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_ResetDialogueUIObjects(Target.GetObject());
}

void UMounteaDialogueManagerStatics::NodeSelected(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FGuid& NodeGUID)
{
	if (Target.GetObject()) Target->Execute_NodeSelected(Target.GetObject(), NodeGUID);
}

void UMounteaDialogueManagerStatics::PrepareNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_PrepareNode(Target.GetObject());
}

void UMounteaDialogueManagerStatics::ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_ProcessNode(Target.GetObject());
}

void UMounteaDialogueManagerStatics::CloseDialogue(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_CloseDialogue(Target.GetObject());
}

bool UMounteaDialogueManagerStatics::CreateDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message)
{
	return Target.GetObject() ? Target->Execute_CreateDialogueUI(Target.GetObject(), Message) : false;
}

bool UMounteaDialogueManagerStatics::UpdateDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message, const FString& Command)
{
	return Target.GetObject() ? Target->Execute_UpdateDialogueUI(Target.GetObject(), Message, Command) : false;
}

bool UMounteaDialogueManagerStatics::CloseDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_CloseDialogueUI(Target.GetObject()) : false;
}

void UMounteaDialogueManagerStatics::ExecuteWidgetCommand(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FString& Command)
{
	if (Target.GetObject()) Target->Execute_ExecuteWidgetCommand(Target.GetObject(), Command);
}

void UMounteaDialogueManagerStatics::SetDialogueWidget(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UUserWidget* DialogueUIPtr)
{
	if (Target.GetObject()) Target->Execute_SetDialogueWidget(Target.GetObject(), DialogueUIPtr);
}

UUserWidget* UMounteaDialogueManagerStatics::GetDialogueWidget(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueWidget(Target.GetObject()) : nullptr;
}

TSubclassOf<UUserWidget> UMounteaDialogueManagerStatics::GetDialogueWidgetClass(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueWidgetClass(Target.GetObject()) : nullptr;
}

int32 UMounteaDialogueManagerStatics::GetDialogueWidgetZOrder(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueWidgetZOrder(Target.GetObject()) : -1;
}

void UMounteaDialogueManagerStatics::SetDialogueWidgetZOrder(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const int32 NewZOrder)
{
	if (Target.GetObject()) Target->Execute_SetDialogueWidgetZOrder(Target.GetObject(), NewZOrder);
}

void UMounteaDialogueManagerStatics::ProcessDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_ProcessDialogueRow(Target.GetObject());
}

void UMounteaDialogueManagerStatics::DialogueRowProcessed(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_DialogueRowProcessed(Target.GetObject());
}

void UMounteaDialogueManagerStatics:: SkipDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_SkipDialogueRow(Target.GetObject());
}
