// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Components/MounteaDialogueManagerNetSync.h"

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

void UMounteaDialogueManagerNetSync::OnManagerSyncActivated(UActorComponent* Component, bool bReset)
{
	
}

void UMounteaDialogueManagerNetSync::OnManagerSyncDeactivated(UActorComponent* Component)
{
	
}

void UMounteaDialogueManagerNetSync::BeginPlay()
{
	Super::BeginPlay();

	OnComponentActivated.AddUniqueDynamic(this, &UMounteaDialogueManagerNetSync::OnManagerSyncActivated);
	OnComponentDeactivated.AddDynamic(this, &UMounteaDialogueManagerNetSync::OnManagerSyncDeactivated);
	
	if (!GetOwner())
		SetActive(false, true);

	if (!GetOwner()->IsA(APlayerController::StaticClass()))
		SetActive(false, true);
}

void UMounteaDialogueManagerNetSync::AddManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager)
{
	if (!IsActive())
		return;
	
	if (Managers.Contains(NewManager))
		return;
	
	Managers.Add(NewManager);
	NewManager->GetDialogueStartRequestedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueManagerNetSync::SyncStartRequested);
}

void UMounteaDialogueManagerNetSync::RemoveManager(const TScriptInterface<IMounteaDialogueManagerInterface>& OldManager)
{
	if (!IsActive())
		return;
	
	if (!Managers.Contains(OldManager))
		return;

	if (!OldManager.GetObject())
		return;

	OldManager->GetDialogueStartRequestedEventHandle().RemoveDynamic(this, &UMounteaDialogueManagerNetSync::SyncStartRequested);
	Managers.Remove(OldManager);
}

void UMounteaDialogueManagerNetSync::SyncStartRequested(const TScriptInterface<IMounteaDialogueManagerInterface>& CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	if (!IsValid(CallingManager.GetObject()))
		return;
	
	if (GetOwner() && GetOwner()->HasAuthority())
		CallingManager->Execute_RequestStartDialogue(CallingManager.GetObject(), DialogueInitiator, InitialParticipants);
	else
		SyncStartRequested_Server(CallingManager, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueManagerNetSync::SyncStartRequested_Server_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	SyncStartRequested(CallingManager, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueManagerNetSync::SyncBroadcastContext(const FMounteaDialogueContextReplicatedStruct& Context)
{
	
}

void UMounteaDialogueManagerNetSync::SyncBroadcastContext_Server_Implementation(const FMounteaDialogueContextReplicatedStruct& Context)
{
	
}

