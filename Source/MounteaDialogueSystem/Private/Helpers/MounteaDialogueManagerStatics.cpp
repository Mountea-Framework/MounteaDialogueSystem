// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Helpers/MounteaDialogueManagerStatics.h"

AActor* UMounteaDialogueManagerStatics::GetOwningActor(UObject* Target)
{
	return ExecuteIfImplements<AActor*>(Target, TEXT("GetOwningActor"), &IMounteaDialogueManagerInterface::Execute_GetOwningActor);
}

UMounteaDialogueContext* UMounteaDialogueManagerStatics::GetDialogueContext(UObject* Target)
{
	return ExecuteIfImplements<UMounteaDialogueContext*>(Target, TEXT("GetDialogueContext"), &IMounteaDialogueManagerInterface::Execute_GetDialogueContext);
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetState(UObject* Target)
{
	return ExecuteIfImplements<EDialogueManagerState>(Target, TEXT("GetState"), &IMounteaDialogueManagerInterface::Execute_GetState);
}

void UMounteaDialogueManagerStatics::SetDialogueManagerState(UObject* Target, const EDialogueManagerState NewState)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueManagerState"), &IMounteaDialogueManagerInterface::Execute_SetDialogueManagerState, NewState);
}

EDialogueManagerState UMounteaDialogueManagerStatics::GetDefaultDialogueManagerState(UObject* Target)
{
	return ExecuteIfImplements<EDialogueManagerState>(Target, TEXT("GetDefaultDialogueManagerState"), &IMounteaDialogueManagerInterface::Execute_GetDefaultDialogueManagerState);
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObject(UObject* Target, UObject* NewDialogueObject)
{
	return ExecuteIfImplements<bool>(Target, TEXT("AddDialogueUIObject"), &IMounteaDialogueManagerInterface::Execute_AddDialogueUIObject, NewDialogueObject);
}

bool UMounteaDialogueManagerStatics::AddDialogueUIObjects(UObject* Target,const TArray<UObject*>& NewDialogueObjects)
{
	return ExecuteIfImplements<bool>(Target, TEXT("AddDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_AddDialogueUIObjects, NewDialogueObjects);
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObject(UObject* Target, UObject* DialogueObjectToRemove)
{
	return ExecuteIfImplements<bool>(Target, TEXT("RemoveDialogueUIObject"), &IMounteaDialogueManagerInterface::Execute_RemoveDialogueUIObject, DialogueObjectToRemove);
}

bool UMounteaDialogueManagerStatics::RemoveDialogueUIObjects(UObject* Target, const TArray<UObject*>& DialogueObjectsToRemove)
{
	return ExecuteIfImplements<bool>(Target, TEXT("RemoveDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_RemoveDialogueUIObjects, DialogueObjectsToRemove);
}

void UMounteaDialogueManagerStatics::SetDialogueUIObjects(UObject* Target, const TArray<UObject*>& NewDialogueObjects)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_SetDialogueUIObjects, NewDialogueObjects);
}

void UMounteaDialogueManagerStatics::ResetDialogueUIObjects(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("ResetDialogueUIObjects"), &IMounteaDialogueManagerInterface::Execute_ResetDialogueUIObjects);
}

void UMounteaDialogueManagerStatics::SkipDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("SkipDialogueRow"), &IMounteaDialogueManagerInterface::Execute_SkipDialogueRow);
}

void UMounteaDialogueManagerStatics::CallDialogueNodeSelected(UObject* Target, const FGuid& NodeGUID)
{
	ExecuteIfImplements<void>(Target, TEXT("CallDialogueNodeSelected"), &IMounteaDialogueManagerInterface::Execute_CallDialogueNodeSelected, NodeGUID);
}

void UMounteaDialogueManagerStatics::PrepareNode(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("PrepareNode"), &IMounteaDialogueManagerInterface::Execute_PrepareNode);
}

void UMounteaDialogueManagerStatics::ProcessNode(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("ProcessNode"), &IMounteaDialogueManagerInterface::Execute_ProcessNode);
}

void UMounteaDialogueManagerStatics::InitializeDialogue(UObject* Target, APlayerState* OwningPlayerState, const FDialogueParticipants& Participants)
{
	ExecuteIfImplements<void>(Target, TEXT("InitializeDialogue"), &IMounteaDialogueManagerInterface::Execute_InitializeDialogue, OwningPlayerState, Participants);
}

void UMounteaDialogueManagerStatics::StartDialogue(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("StartDialogue"), &IMounteaDialogueManagerInterface::Execute_StartDialogue);
}

void UMounteaDialogueManagerStatics::CloseDialogue(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("CloseDialogue"), &IMounteaDialogueManagerInterface::Execute_CloseDialogue);
}

bool UMounteaDialogueManagerStatics::InvokeDialogueUI(UObject* Target, FString& Message)
{
	return ExecuteIfImplements<bool>(Target, TEXT("InvokeDialogueUI"), &IMounteaDialogueManagerInterface::Execute_InvokeDialogueUI, Message);
}

bool UMounteaDialogueManagerStatics::UpdateDialogueUI(UObject* Target, FString& Message, const FString& Command)
{
	return ExecuteIfImplements<bool>(Target, TEXT("UpdateDialogueUI"), &IMounteaDialogueManagerInterface::Execute_UpdateDialogueUI, Message, Command);
}

bool UMounteaDialogueManagerStatics::CloseDialogueUI(UObject* Target)
{
	return ExecuteIfImplements<bool>(Target, TEXT("CloseDialogueUI"), &IMounteaDialogueManagerInterface::Execute_CloseDialogueUI);
}

void UMounteaDialogueManagerStatics::ExecuteWidgetCommand(UObject* Target, const FString& Command)
{
	ExecuteIfImplements<void>(Target, TEXT("ExecuteWidgetCommand"), &IMounteaDialogueManagerInterface::Execute_ExecuteWidgetCommand, Command);
}

void UMounteaDialogueManagerStatics::SetDialogueWidget(UObject* Target, UUserWidget* DialogueUIPtr)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueWidget"), &IMounteaDialogueManagerInterface::Execute_SetDialogueWidget, DialogueUIPtr);
}

UUserWidget* UMounteaDialogueManagerStatics::GetDialogueWidget(UObject* Target)
{
	return ExecuteIfImplements<UUserWidget*>(Target, TEXT("GetDialogueWidget"), &IMounteaDialogueManagerInterface::Execute_GetDialogueWidget);
}

TSubclassOf<UUserWidget> UMounteaDialogueManagerStatics::GetDialogueWidgetClass(UObject* Target)
{
	return ExecuteIfImplements<TSubclassOf<UUserWidget>>(Target, TEXT("GetDialogueWidgetClass"), &IMounteaDialogueManagerInterface::Execute_GetDialogueWidgetClass);
}

int32 UMounteaDialogueManagerStatics::GetDialogueWidgetZOrder(UObject* Target)
{
	return ExecuteIfImplements<int32>(Target, TEXT("GetDialogueWidgetZOrder"), &IMounteaDialogueManagerInterface::Execute_GetDialogueWidgetZOrder);
}

void UMounteaDialogueManagerStatics::SetDialogueWidgetZOrder(UObject* Target, const int32 NewZOrder)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueWidgetZOrder"), &IMounteaDialogueManagerInterface::Execute_SetDialogueWidgetZOrder, NewZOrder);
}

void UMounteaDialogueManagerStatics::StartExecuteDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("StartExecuteDialogueRow"), &IMounteaDialogueManagerInterface::Execute_StartExecuteDialogueRow);
}

void UMounteaDialogueManagerStatics::FinishedExecuteDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("FinishedExecuteDialogueRow"), &IMounteaDialogueManagerInterface::Execute_FinishedExecuteDialogueRow);
}

void UMounteaDialogueManagerStatics::TriggerNextDialogueRow(UObject* Target)
{
	ExecuteIfImplements<void>(Target, TEXT("TriggerNextDialogueRow"), &IMounteaDialogueManagerInterface::Execute_TriggerNextDialogueRow);
}
