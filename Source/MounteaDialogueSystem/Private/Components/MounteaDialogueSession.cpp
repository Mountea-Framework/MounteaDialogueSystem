// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "Components/MounteaDialogueSession.h"
#include "Components/MounteaDialogueManager.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Subsystem/MounteaDialogueWorldSubsystem.h"

UMounteaDialogueSession::UMounteaDialogueSession()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UMounteaDialogueSession::BeginPlay()
{
	Super::BeginPlay();
}

void UMounteaDialogueSession::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UMounteaDialogueSession, ContextPayload, Params);
}

void UMounteaDialogueSession::OnRep_ContextPayload()
{
	NotifyLocalManagers();
}

void UMounteaDialogueSession::WriteContextPayload(FMounteaDialogueContextPayload NewPayload)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	NewPayload.ContextVersion = ContextPayload.ContextVersion + 1;
	ContextPayload = MoveTemp(NewPayload);
	MARK_PROPERTY_DIRTY_FROM_NAME(UMounteaDialogueSession, ContextPayload, this);

	NotifyLocalManagers();
}

void UMounteaDialogueSession::NotifyLocalManagers() const
{
	UWorld* world = GetWorld();
	if (!world)
		return;

	UMounteaDialogueWorldSubsystem* subsystem = world->GetSubsystem<UMounteaDialogueWorldSubsystem>();
	if (!subsystem)
		return;

	for (UMounteaDialogueManager* manager : subsystem->GetRegisteredManagers())
	{
		if (!IsValid(manager))
			continue;

		const AActor* ownerActor = manager->GetOwner();
		if (!IsValid(ownerActor))
			continue;

		if (!UMounteaDialogueSystemBFC::IsLocalPlayer(ownerActor))
			continue;

		manager->OnContextPayloadUpdated(ContextPayload);
	}
}
