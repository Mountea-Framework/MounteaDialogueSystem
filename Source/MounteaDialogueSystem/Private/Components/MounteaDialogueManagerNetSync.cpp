// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Components/MounteaDialogueManagerNetSync.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"

UMounteaDialogueManagerNetSync::UMounteaDialogueManagerNetSync()
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

void UMounteaDialogueManagerNetSync::BeginPlay()
{
	Super::BeginPlay();
	
	if (!GetOwner() || !GetOwner()->IsA(APlayerController::StaticClass()))
		SetActive(false, true);
}

void UMounteaDialogueManagerNetSync::ReceiveCreateContext(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& NewContext)
{
	
}

void UMounteaDialogueManagerNetSync::ReceiveStartRequest(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
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

void UMounteaDialogueManagerNetSync::ReceiveCreateContext_Server_Implementation(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& NewContext)
{
	LOG_WARNING(TEXT("[Receive Start Request] Manager Sync Component Called Start!"))
	ReceiveCreateContext(CallingManager, NewContext);
}

void UMounteaDialogueManagerNetSync::ReceiveStartRequest_Server_Implementation(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	LOG_WARNING(TEXT("[Receive Start Request] Manager Sync Component Called Start!"))
	ReceiveStartRequest(CallingManager, DialogueInitiator, InitialParticipants);
}
