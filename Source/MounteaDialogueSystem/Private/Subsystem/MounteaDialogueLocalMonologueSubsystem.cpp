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

#include "Subsystem/MounteaDialogueLocalMonologueSubsystem.h"

#include "Components/MounteaDialogueLocalMonologueComponent.h"
#include "Components/MounteaDialogueManager.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"

void UMounteaDialogueLocalMonologueSubsystem::Deinitialize()
{
	RegisteredMonologueComponents.Empty();
	ActiveMonologueComponent.Reset();
	ActiveMonologueSessionGUID.Invalidate();

	Super::Deinitialize();
}

bool UMounteaDialogueLocalMonologueSubsystem::TryStartMonologue(
	UMounteaDialogueLocalMonologueComponent* Component,
	const FDialogueStartRequest& Request,
	FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (!IsValid(Component))
	{
		OutErrorMessage = TEXT("[TryStartMonologue] Invalid local monologue component.");
		return false;
	}

	RegisterMonologueComponent(Component);

	if (ActiveMonologueComponent.IsValid() && ActiveMonologueComponent.Get() != Component)
	{
		OutErrorMessage = TEXT("[TryStartMonologue] Another local monologue is already active.");
		return false;
	}

	FString startReason;
	if (!CanStartLocalMonologue(startReason))
	{
		OutErrorMessage = startReason;
		return false;
	}

	if (!Component->StartLocalMonologueInternal(Request, OutErrorMessage))
		return false;

	const FGuid startedSessionGuid = Component->GetActiveLocalMonologueSessionGUID();
	if (!startedSessionGuid.IsValid())
	{
		OutErrorMessage = TEXT("[TryStartMonologue] Started monologue produced an invalid session GUID.");
		return false;
	}

	ActiveMonologueComponent = Component;
	ActiveMonologueSessionGUID = startedSessionGuid;
	return true;
}

void UMounteaDialogueLocalMonologueSubsystem::StopMonologue(UMounteaDialogueLocalMonologueComponent* Component)
{
	if (!ActiveMonologueComponent.IsValid())
		return;

	if (IsValid(Component) && ActiveMonologueComponent.Get() != Component)
		return;

	UMounteaDialogueLocalMonologueComponent* activeComponent = ActiveMonologueComponent.Get();
	if (!IsValid(activeComponent))
	{
		ActiveMonologueComponent.Reset();
		ActiveMonologueSessionGUID.Invalidate();
		return;
	}

	if (activeComponent->IsLocalMonologueActive())
	{
		activeComponent->RequestCloseLocalMonologue();
		return;
	}

	ReleaseMonologueLock(activeComponent, ActiveMonologueSessionGUID);
}

bool UMounteaDialogueLocalMonologueSubsystem::HasActiveLocalMonologue() const
{
	return ActiveMonologueComponent.IsValid();
}

bool UMounteaDialogueLocalMonologueSubsystem::CanStartLocalMonologue(FString& OutReason) const
{
	OutReason.Empty();

	if (ActiveMonologueComponent.IsValid())
	{
		OutReason = TEXT("[CanStartLocalMonologue] A local monologue is already active.");
		return false;
	}

	if (HasAnyActiveRegularManager(OutReason))
		return false;

	return true;
}

void UMounteaDialogueLocalMonologueSubsystem::RegisterMonologueComponent(UMounteaDialogueLocalMonologueComponent* Component)
{
	if (!IsValid(Component))
		return;

	RegisteredMonologueComponents.AddUnique(Component);
}

void UMounteaDialogueLocalMonologueSubsystem::UnregisterMonologueComponent(UMounteaDialogueLocalMonologueComponent* Component)
{
	if (!IsValid(Component))
		return;

	RegisteredMonologueComponents.Remove(Component);

	if (ActiveMonologueComponent.Get() == Component)
		ReleaseMonologueLock(Component, ActiveMonologueSessionGUID);
}

void UMounteaDialogueLocalMonologueSubsystem::ReleaseMonologueLock(
	UMounteaDialogueLocalMonologueComponent* Component,
	const FGuid& SessionGUID)
{
	if (!ActiveMonologueComponent.IsValid())
		return;

	if (IsValid(Component) && ActiveMonologueComponent.Get() != Component)
		return;

	if (SessionGUID.IsValid() && ActiveMonologueSessionGUID.IsValid() && SessionGUID != ActiveMonologueSessionGUID)
		return;

	ActiveMonologueComponent.Reset();
	ActiveMonologueSessionGUID.Invalidate();
}

bool UMounteaDialogueLocalMonologueSubsystem::HasAnyActiveRegularManager(FString& OutReason) const
{
	OutReason.Empty();

	const ULocalPlayer* localPlayer = GetLocalPlayer();
	APlayerController* playerController = localPlayer ? localPlayer->GetPlayerController(GetWorld()) : nullptr;
	if (!IsValid(playerController))
		return false;

	TArray<UMounteaDialogueManager*> managersToCheck;
	const auto AddManagersFromActor = [&managersToCheck](AActor* Actor)
	{
		if (!IsValid(Actor))
			return;

		TArray<UMounteaDialogueManager*> actorManagers;
		Actor->GetComponents(actorManagers);
		for (UMounteaDialogueManager* manager : actorManagers)
			managersToCheck.AddUnique(manager);
	};

	AddManagersFromActor(playerController);
	AddManagersFromActor(playerController->GetPawn());
	AddManagersFromActor(playerController->PlayerState);

	for (UMounteaDialogueManager* manager : managersToCheck)
	{
		if (!IsValid(manager))
			continue;

		if (manager->IsA<UMounteaDialogueLocalMonologueComponent>())
			continue;

		if (IMounteaDialogueManagerInterface::Execute_GetManagerState(manager) == EDialogueManagerState::EDMS_Active)
		{
			OutReason = FString::Printf(
				TEXT("[CanStartLocalMonologue] Regular dialogue manager '%s' is currently active."),
				*GetNameSafe(manager));
			return true;
		}
	}

	return false;
}
