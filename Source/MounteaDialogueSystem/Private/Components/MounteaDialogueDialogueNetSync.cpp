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

void UMounteaDialogueDialogueNetSync::ReceiveCloseRequest(UObject* CallingManager)
{
	if (!IsActive())
	{
		LOG_WARNING(TEXT("[Receive Close Request] Manager Sync Component is not Active!"))
		return;;
	}
	
	if (!GetOwner()->HasAuthority())
		ReceiveCloseRequest_Server(CallingManager);
	else
	{
		IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(CallingManager);
	}
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

void UMounteaDialogueDialogueNetSync::ReceiveBroadcastContextRequest(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& Context)
{
	if (!IsActive())
	{
		LOG_WARNING(TEXT("[Receive Broadcast Context] Manager Sync Component is not Active!"))
		return;
	}

	if (!GetOwner()->HasAuthority())
		ReceiveBroadcastContextRequest_Server(CallingManager, Context);
	else
	{
		TScriptInterface<IMounteaDialogueManagerInterface> dialogueManager = CallingManager;
		dialogueManager->SyncContext(Context);
	}
}

void UMounteaDialogueDialogueNetSync::ReceiveCloseRequest_Server_Implementation(UObject* CallingManager)
{
	ReceiveCloseRequest(CallingManager);
}

void UMounteaDialogueDialogueNetSync::ReceiveBroadcastContextRequest_Server_Implementation(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& Context)
{
	ReceiveBroadcastContextRequest(CallingManager, Context);
}

void UMounteaDialogueDialogueNetSync::ReceiveSetState_Server_Implementation(UObject* CallingManager, const EDialogueManagerState NewState)
{
	ReceiveSetState(CallingManager, NewState);
}

void UMounteaDialogueDialogueNetSync::ReceiveStartRequest_Server_Implementation(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	ReceiveStartRequest(CallingManager, DialogueInitiator, InitialParticipants);
}
