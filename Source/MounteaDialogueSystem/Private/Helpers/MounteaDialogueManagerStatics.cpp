// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Helpers/MounteaDialogueManagerStatics.h"

AActor* UMounteaDialogueManagerStatics::GetOwningActor(UObject* Target)
{
	return ExecuteIfImplements<AActor*>(Target, TEXT("GetOwningActor"), &IMounteaDialogueManagerInterface::Execute_GetOwningActor);
}

AActor* UMounteaDialogueManagerStatics::GetOwningActorV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetOwningActor(Target.GetObject()) : nullptr;
}

UObject* UMounteaDialogueManagerStatics::GetManagerObject(UObject* Target)
{
	return ExecuteIfImplements<UObject*>(Target, TEXT("GetManagerObject"), &IMounteaDialogueManagerInterface::Execute_GetManagerObject);
}

UObject* UMounteaDialogueManagerStatics::GetManagerObjectV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetManagerObject(Target.GetObject()) : nullptr;
}

UMounteaDialogueContext* UMounteaDialogueManagerStatics::GetDialogueContext(UObject* Target)
{
	return ExecuteIfImplements<UMounteaDialogueContext*>(Target, TEXT("GetDialogueContext"), &IMounteaDialogueManagerInterface::Execute_GetDialogueContext);
}

UMounteaDialogueContext* UMounteaDialogueManagerStatics::GetDialogueContextV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueContext(Target.GetObject()) : nullptr;
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetState(UObject* Target)
{
	return ExecuteIfImplements<EDialogueManagerState>(Target, TEXT("GetState"), &IMounteaDialogueManagerInterface::Execute_GetState);
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetStateV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetState(Target.GetObject()) : EDialogueManagerState::Default;
}

void UMounteaDialogueManagerStatics::SetDialogueManagerState(UObject* Target, const EDialogueManagerState NewState)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueManagerState"), &IMounteaDialogueManagerInterface::Execute_SetDialogueManagerState, NewState);
}

void UMounteaDialogueManagerStatics::SetDialogueManagerStateV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const EDialogueManagerState NewState)
{
	if (Target.GetObject()) Target->Execute_SetDialogueManagerState(Target.GetObject(), NewState);
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetDefaultDialogueManagerState(UObject* Target)
{
	return ExecuteIfImplements<EDialogueManagerState>(Target, TEXT("GetDefaultDialogueManagerState"), &IMounteaDialogueManagerInterface::Execute_GetDefaultDialogueManagerState);
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetDefaultDialogueManagerStateV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDefaultDialogueManagerState(Target.GetObject()) : EDialogueManagerState::Default;
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObject(UObject* Target, UObject* NewDialogueObject)
{
	return ExecuteIfImplements<bool>(Target, TEXT("AddDialogueUIObject"), &IMounteaDialogueManagerInterface::Execute_AddDialogueUIObject, NewDialogueObject);
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObjectV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* NewDialogueObject)
{
	return Target.GetObject() ? Target->Execute_AddDialogueUIObject(Target.GetObject(), NewDialogueObject) : false;
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObjects(UObject* Target,const TArray<UObject*>& NewDialogueObjects)
{
	return ExecuteIfImplements<bool>(Target, TEXT("AddDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_AddDialogueUIObjects, NewDialogueObjects);
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects)
{
	return Target.GetObject() ? Target->Execute_AddDialogueUIObjects(Target.GetObject(), NewDialogueObjects) : false;
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObject(UObject* Target, UObject* DialogueObjectToRemove)
{
	return ExecuteIfImplements<bool>(Target, TEXT("RemoveDialogueUIObject"), &IMounteaDialogueManagerInterface::Execute_RemoveDialogueUIObject, DialogueObjectToRemove);
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObjectV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* DialogueObjectToRemove)
{
	return Target.GetObject() ? Target->Execute_RemoveDialogueUIObject(Target.GetObject(), DialogueObjectToRemove) : false;
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObjects(UObject* Target, const TArray<UObject*>& DialogueObjectsToRemove)
{
	return ExecuteIfImplements<bool>(Target, TEXT("RemoveDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_RemoveDialogueUIObjects, DialogueObjectsToRemove);
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& DialogueObjectsToRemove)
{
	return Target.GetObject() ? Target->Execute_RemoveDialogueUIObjects(Target.GetObject(), DialogueObjectsToRemove) : false;
}

void UMounteaDialogueManagerStatics::SetDialogueUIObjects(UObject* Target, const TArray<UObject*>& NewDialogueObjects)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_SetDialogueUIObjects, NewDialogueObjects);
}

void UMounteaDialogueManagerStatics::SetDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects)
{
	if (Target.GetObject()) Target->Execute_SetDialogueUIObjects(Target.GetObject(), NewDialogueObjects);
}

void UMounteaDialogueManagerStatics::ResetDialogueUIObjects(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("ResetDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_ResetDialogueUIObjects);
}

void UMounteaDialogueManagerStatics::ResetDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_ResetDialogueUIObjects(Target.GetObject());
}

void UMounteaDialogueManagerStatics::SkipDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("SkipDialogueRow"), &IMounteaDialogueManagerInterface::Execute_SkipDialogueRow);
}

void UMounteaDialogueManagerStatics::SkipDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_SkipDialogueRow(Target.GetObject());
}

void UMounteaDialogueManagerStatics::CallDialogueNodeSelected(UObject* Target, const FGuid& NodeGUID)
{
	ExecuteIfImplements<void>(Target, TEXT("CallDialogueNodeSelected"), &IMounteaDialogueManagerInterface::Execute_CallDialogueNodeSelected, NodeGUID);
}

void UMounteaDialogueManagerStatics::CallDialogueNodeSelectedV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FGuid& NodeGUID)
{
	if (Target.GetObject()) Target->Execute_CallDialogueNodeSelected(Target.GetObject(), NodeGUID);
}

void UMounteaDialogueManagerStatics::PrepareNode(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("PrepareNode"), &IMounteaDialogueManagerInterface::Execute_PrepareNode);
}

void UMounteaDialogueManagerStatics::PrepareNodeV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_PrepareNode(Target.GetObject());
}

void UMounteaDialogueManagerStatics::ProcessNode(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("ProcessNode"), &IMounteaDialogueManagerInterface::Execute_ProcessNode);
}

void UMounteaDialogueManagerStatics::ProcessNodeV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_ProcessNode(Target.GetObject());
}

void UMounteaDialogueManagerStatics::InitializeDialogue(
	UObject* Target, APlayerState* OwningPlayerState, const FDialogueParticipants& Participants)
{
	ExecuteIfImplements<void>(Target, TEXT("InitializeDialogue"), &IMounteaDialogueManagerInterface::Execute_InitializeDialogue, OwningPlayerState, Participants);
}

void UMounteaDialogueManagerStatics::InitializeDialogueV2(
	const TScriptInterface<IMounteaDialogueManagerInterface>& Target, APlayerState* OwningPlayerState,
	const FDialogueParticipants& Participants)
{
	if (Target.GetObject()) Target->Execute_InitializeDialogue(Target.GetObject(), OwningPlayerState, Participants);
}

void UMounteaDialogueManagerStatics::StartDialogue(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("StartDialogue"), &IMounteaDialogueManagerInterface::Execute_StartDialogue);
}

void UMounteaDialogueManagerStatics::StartDialogueV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_StartDialogue(Target.GetObject());
}

void UMounteaDialogueManagerStatics::CloseDialogue(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("CloseDialogue"), &IMounteaDialogueManagerInterface::Execute_CloseDialogue);
}

void UMounteaDialogueManagerStatics::CloseDialogueV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_CloseDialogue(Target.GetObject());
}

bool UMounteaDialogueManagerStatics::InvokeDialogueUI(UObject* Target, FString& Message)
{
	return ExecuteIfImplements<bool>(Target, TEXT("InvokeDialogueUI"), &IMounteaDialogueManagerInterface::Execute_InvokeDialogueUI, Message);
}

bool UMounteaDialogueManagerStatics::InvokeDialogueUIV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message)
{
	return Target.GetObject() ? Target->Execute_InvokeDialogueUI(Target.GetObject(), Message) : false;
}

bool UMounteaDialogueManagerStatics::UpdateDialogueUI(UObject* Target, FString& Message, const FString& Command)
{
	return ExecuteIfImplements<bool>(Target, TEXT("UpdateDialogueUI"), &IMounteaDialogueManagerInterface::Execute_UpdateDialogueUI, Message, Command);
}

bool UMounteaDialogueManagerStatics::UpdateDialogueUIV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message, const FString& Command)
{
	return Target.GetObject() ? Target->Execute_UpdateDialogueUI(Target.GetObject(), Message, Command) : false;
}

bool UMounteaDialogueManagerStatics::CloseDialogueUI(UObject* Target)
{
	return ExecuteIfImplements<bool>(Target, TEXT("CloseDialogueUI"), &IMounteaDialogueManagerInterface::Execute_CloseDialogueUI);
}

bool UMounteaDialogueManagerStatics::CloseDialogueUIV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_CloseDialogueUI(Target.GetObject()) : false;
}

void UMounteaDialogueManagerStatics::ExecuteWidgetCommand(UObject* Target, const FString& Command)
{
	ExecuteIfImplements<void>(Target, TEXT("ExecuteWidgetCommand"), &IMounteaDialogueManagerInterface::Execute_ExecuteWidgetCommand, Command);
}

void UMounteaDialogueManagerStatics::ExecuteWidgetCommandV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FString& Command)
{
	if (Target.GetObject()) Target->Execute_ExecuteWidgetCommand(Target.GetObject(), Command);
}

void UMounteaDialogueManagerStatics::SetDialogueWidget(UObject* Target, UUserWidget* DialogueUIPtr)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueWidget"), &IMounteaDialogueManagerInterface::Execute_SetDialogueWidget, DialogueUIPtr);
}

void UMounteaDialogueManagerStatics::SetDialogueWidgetV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UUserWidget* DialogueUIPtr)
{
	if (Target.GetObject()) Target->Execute_SetDialogueWidget(Target.GetObject(), DialogueUIPtr);
}

UUserWidget* UMounteaDialogueManagerStatics::GetDialogueWidget(UObject* Target)
{
	return ExecuteIfImplements<UUserWidget*>(Target, TEXT("GetDialogueWidget"), &IMounteaDialogueManagerInterface::Execute_GetDialogueWidget);
}

UUserWidget* UMounteaDialogueManagerStatics::GetDialogueWidgetV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueWidget(Target.GetObject()) : nullptr;
}

TSubclassOf<UUserWidget> UMounteaDialogueManagerStatics::GetDialogueWidgetClass(UObject* Target)
{
	return ExecuteIfImplements<TSubclassOf<UUserWidget>>(Target, TEXT("GetDialogueWidgetClass"), &IMounteaDialogueManagerInterface::Execute_GetDialogueWidgetClass);
}

TSubclassOf<UUserWidget> UMounteaDialogueManagerStatics::GetDialogueWidgetClassV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueWidgetClass(Target.GetObject()) : nullptr;
}

int32 UMounteaDialogueManagerStatics::GetDialogueWidgetZOrder(UObject* Target)
{
	return ExecuteIfImplements<int32>(Target, TEXT("GetDialogueWidgetZOrder"), &IMounteaDialogueManagerInterface::Execute_GetDialogueWidgetZOrder);
}

int32 UMounteaDialogueManagerStatics::GetDialogueWidgetZOrderV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	return Target.GetObject() ? Target->Execute_GetDialogueWidgetZOrder(Target.GetObject()) : -1;
}

void UMounteaDialogueManagerStatics::SetDialogueWidgetZOrder(UObject* Target, const int32 NewZOrder)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueWidgetZOrder"), &IMounteaDialogueManagerInterface::Execute_SetDialogueWidgetZOrder, NewZOrder);
}

void UMounteaDialogueManagerStatics::SetDialogueWidgetZOrderV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const int32 NewZOrder)
{
	if (Target.GetObject()) Target->Execute_SetDialogueWidgetZOrder(Target.GetObject(), NewZOrder);
}

void UMounteaDialogueManagerStatics::StartExecuteDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("StartExecuteDialogueRow"), &IMounteaDialogueManagerInterface::Execute_StartExecuteDialogueRow);
}

void UMounteaDialogueManagerStatics::StartExecuteDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_StartExecuteDialogueRow(Target.GetObject());
}

void UMounteaDialogueManagerStatics::FinishedExecuteDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("FinishedExecuteDialogueRow"), &IMounteaDialogueManagerInterface::Execute_FinishedExecuteDialogueRow);
}

void UMounteaDialogueManagerStatics::FinishedExecuteDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_FinishedExecuteDialogueRow(Target.GetObject());
}

void UMounteaDialogueManagerStatics::TriggerNextDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("TriggerNextDialogueRow"), &IMounteaDialogueManagerInterface::Execute_TriggerNextDialogueRow);
}

void UMounteaDialogueManagerStatics::TriggerNextDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
{
	if (Target.GetObject()) Target->Execute_TriggerNextDialogueRow(Target.GetObject());
}
