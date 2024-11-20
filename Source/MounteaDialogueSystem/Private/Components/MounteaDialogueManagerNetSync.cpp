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

void UMounteaDialogueManagerNetSync::RouteRPC_Server_Implementation(UFunction* RPCFunction, APlayerController* Instigator, const FGenericRPCPayload& Payload)
{
	if (!RPCFunction || !Instigator)
	{
		LOG_ERROR(TEXT("Server received invalid RPCFunction or Instigator"));
		return;
	}

	AActor* DialogueInitiator = nullptr;
	FDialogueParticipants InitialParticipants;
	
	Payload.Unpack(DialogueInitiator, InitialParticipants);
	
	if (!DialogueInitiator)
	{
		LOG_ERROR(TEXT("Failed to unpack DialogueInitiator"));
		return;
	}

	LOG_INFO(TEXT("Successfully unpacked DialogueInitiator: %s"), *DialogueInitiator->GetName());
	ExecuteRPC(RPCFunction, Instigator, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueManagerNetSync::BeginPlay()
{
	Super::BeginPlay();

	OnComponentActivated.AddUniqueDynamic(this, &UMounteaDialogueManagerNetSync::OnManagerSyncActivated);
	OnComponentDeactivated.AddDynamic(this, &UMounteaDialogueManagerNetSync::OnManagerSyncDeactivated);
	
	if (!GetOwner())
	{
		SetActive(false, true);
		return;
	}

	if (!GetOwner()->IsA(APlayerController::StaticClass()))
	{
		SetActive(false, true);
		return;
	}
}

void UMounteaDialogueManagerNetSync::AddManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager)
{
	if (!IsActive())
		return;
	
	if (Managers.Contains(NewManager))
		return;
	
	Managers.Add(NewManager);
}

void UMounteaDialogueManagerNetSync::RemoveManager(const TScriptInterface<IMounteaDialogueManagerInterface>& OldManager)
{
	if (!IsActive())
		return;
	
	if (!Managers.Contains(OldManager))
		return;

	if (!OldManager.GetObject())
		return;

	Managers.Remove(OldManager);
}

void UMounteaDialogueManagerNetSync::OnManagerSyncActivated(UActorComponent* Component, bool bReset)
{
	// TODO: Setup bindings?
}

void UMounteaDialogueManagerNetSync::OnManagerSyncDeactivated(UActorComponent* Component)
{
	// TODO: Setup bindings?
}