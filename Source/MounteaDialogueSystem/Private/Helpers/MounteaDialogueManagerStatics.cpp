// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Helpers/MounteaDialogueManagerStatics.h"

void UMounteaDialogueManagerStatics::CallDialogueNodeSelected(UObject* Target, const FGuid& NodeGUID)
{
	ExecuteIfImplements<void>(Target, TEXT("CallDialogueNodeSelected"), &IMounteaDialogueManagerInterface::Execute_CallDialogueNodeSelected, NodeGUID);
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

void UMounteaDialogueManagerStatics::SetDialogueManagerState(UObject* Target, const EDialogueManagerState NewState)
{
	ExecuteIfImplements<void>(Target, TEXT("SetDialogueManagerState"), &IMounteaDialogueManagerInterface::Execute_SetDialogueManagerState, NewState);
}

