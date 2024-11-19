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

void UMounteaDialogueManagerNetSync::RouteRPC_Server_Implementation(UFunction* RPCFunction, APlayerController* Instigator, const FGenericRPCPayload& Payload)
{
	// Validate authority
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	// Unpack and execute
	FMounteaDialogueContextReplicatedStruct Context;
	Payload.Unpack(Context);
	
	ExecuteRPC(RPCFunction, Instigator, Context);
}

void UMounteaDialogueManagerNetSync::BeginPlay()
{
	Super::BeginPlay();

	// Register activation/deactivation handlers
	OnComponentActivated.AddUniqueDynamic(this, &UMounteaDialogueManagerNetSync::OnManagerSyncActivated);
	OnComponentDeactivated.AddDynamic(this, &UMounteaDialogueManagerNetSync::OnManagerSyncDeactivated);
	
	// Validate owner
	if (!GetOwner())
	{
		SetActive(false, true);
		return;
	}

	// Ensure owner is a PlayerController
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
