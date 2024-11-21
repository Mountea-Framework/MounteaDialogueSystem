// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Components/MounteaDialogueDialogueNetSync.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"

UMounteaDialogueDialogueNetSync::UMounteaDialogueDialogueNetSync()
{
	bAutoActivate = true;
	
	SetIsReplicatedByDefault(true);
	SetActiveFlag(true);
	
	PrimaryComponentTick.bCanEverTick = false;

	ComponentTags.Add(FName("Mountea"));
	ComponentTags.Add(FName("Dialogue"));
	ComponentTags.Add(FName("Manager"));
	ComponentTags.Add(FName("Sync"));
}

void UMounteaDialogueDialogueNetSync::BeginPlay()
{
	Super::BeginPlay();
	
	if (!GetOwner() || !GetOwner()->IsA(APlayerController::StaticClass()))
		SetActive(false, true);
}

void UMounteaDialogueDialogueNetSync::ReceiveStartRequest(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	if (!IsActive())
	{
		LOG_WARNING(TEXT("[Receive Start Request] Manager Sync Component is not Active!"))
		return;;
	}
	
	if (!GetOwner()->HasAuthority())
		ReceiveStartRequest_Server(CallingManager, DialogueInitiator, InitialParticipants);
	else
		IMounteaDialogueManagerInterface::Execute_RequestStartDialogue(CallingManager, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueDialogueNetSync::ReceiveSetState(UObject* CallingManager, const EDialogueManagerState NewState)
{
	if (!IsActive())
	{
		LOG_WARNING(TEXT("[Receive Set State] Manager Sync Component is not Active!"))
		return;;
	}
	
	if (!GetOwner()->HasAuthority())
		ReceiveSetState_Server(CallingManager, NewState);
	else
	{
		TScriptInterface<IMounteaDialogueManagerInterface> dialogueManager = CallingManager;
		dialogueManager->SetManagerState(NewState);
	}
}

void UMounteaDialogueDialogueNetSync::ReceiveSetState_Server_Implementation(UObject* CallingManager, const EDialogueManagerState NewState)
{
	ReceiveSetState(CallingManager, NewState);
}

void UMounteaDialogueDialogueNetSync::ReceiveStartRequest_Server_Implementation(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	LOG_WARNING(TEXT("[Receive Start Request] Manager Sync Component Called Start!"))
	ReceiveStartRequest(CallingManager, DialogueInitiator, InitialParticipants);
}
