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

#include "Subsystem/MounteaDialogueWorldSubsystem.h"
#include "Components/MounteaDialogueManager.h"
#include "Components/MounteaDialogueSession.h"
#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"

void UMounteaDialogueWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMounteaDialogueWorldSubsystem::Deinitialize()
{
	RegisteredManagers.Empty();
	ActiveSessions.Empty();

	Super::Deinitialize();
}

void UMounteaDialogueWorldSubsystem::RegisterManager(UMounteaDialogueManager* Manager)
{
	if (!Manager) return;
	RegisteredManagers.AddUnique(Manager);
}

void UMounteaDialogueWorldSubsystem::UnregisterManager(UMounteaDialogueManager* Manager)
{
	if (!Manager) return;
	RegisteredManagers.Remove(Manager);
}

void UMounteaDialogueWorldSubsystem::HandleStartRequest(UMounteaDialogueManager* Manager, const FDialogueStartRequest& Request)
{
	if (!Manager) return;

	TScriptInterface<IMounteaDialogueParticipantInterface> playerParticipant;
	TScriptInterface<IMounteaDialogueParticipantInterface> mainParticipant;
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> allParticipants;
	TArray<FText> errors;
	errors.Add(FText::FromString(TEXT("[HandleStartRequest]")));

	if (!ResolveParticipants(Manager, Request, playerParticipant, mainParticipant, allParticipants, errors))
	{
		const FText combined = FText::Join(FText::FromString(TEXT("\n")), errors);
		Manager->GetDialogueFailedEventHandle().Broadcast(combined.ToString());
		return;
	}

	UMounteaDialogueContext* newContext = UMounteaDialogueSystemBFC::CreateDialogueContext(Manager, mainParticipant, allParticipants);
	if (!newContext)
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[HandleStartRequest] Failed to create Dialogue Context!"));
		return;
	}

	if (playerParticipant.GetObject())
		newContext->UpdateDialoguePlayerParticipant(playerParticipant);

	Manager->SetDialogueContext(newContext);

	Manager->GetDialogueStartRequestedResultEventHandle().Broadcast(true, TEXT("OK"));
}

void UMounteaDialogueWorldSubsystem::RequestStartEnvironmentDialogue(UMounteaDialogueManager* Manager, const FDialogueStartRequest& Request)
{
	HandleStartRequest(Manager, Request);
}

bool UMounteaDialogueWorldSubsystem::ResolveParticipants(
	UMounteaDialogueManager* Manager,
	const FDialogueStartRequest& Request,
	TScriptInterface<IMounteaDialogueParticipantInterface>& OutPlayerParticipant,
	TScriptInterface<IMounteaDialogueParticipantInterface>& OutMainParticipant,
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& OutAllParticipants,
	TArray<FText>& OutErrors) const
{
	if (!IsValid(Manager))
	{
		OutErrors.Add(NSLOCTEXT("ResolveParticipants", "NoManager", "Manager could not be resolved!"));
		return false;
	}
	
	// Resolve main (NPC) participant
	AActor* mainActor = Request.MainParticipantActor.Get();
	if (!mainActor)
	{
		OutErrors.Add(NSLOCTEXT("ResolveParticipants", "NoMainActor", "MainParticipantActor could not be resolved!"));
		return false;
	}

	bool bFound = false;
	OutMainParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(mainActor, bFound);
	if (!bFound || !OutMainParticipant.GetObject())
	{
		OutErrors.Add(NSLOCTEXT("ResolveParticipants", "NoMainParticipant", "MainParticipantActor does not implement IMounteaDialogueParticipantInterface!"));
		return false;
	}

	if (!OutMainParticipant->Execute_CanStartDialogue(OutMainParticipant.GetObject()))
	{
		OutErrors.Add(NSLOCTEXT("ResolveParticipants", "CannotStart", "Main participant cannot start dialogue!"));
		return false;
	}

	OutAllParticipants.Add(OutMainParticipant);

	// Resolve player participant from manager owner chain (null for environment managers)
	AActor* ownerActor = Manager->Execute_GetOwningActor(Manager);
	if (IsValid(ownerActor))
	{
		int searchDepth = 0;
		if (APawn* playerPawn = UMounteaDialogueSystemBFC::FindPlayerPawn(ownerActor, searchDepth))
		{
			bool bPlayerFound = false;
			OutPlayerParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(playerPawn, bPlayerFound);
			if (bPlayerFound && OutPlayerParticipant.GetObject() && OutPlayerParticipant->Execute_CanParticipateInDialogue(OutPlayerParticipant.GetObject()))
				OutAllParticipants.AddUnique(OutPlayerParticipant);
			else
				OutPlayerParticipant = nullptr;
		}
	}

	// Resolve other participants
	for (const auto& softRef : Request.OtherParticipantActors)
	{
		AActor* otherActor = softRef.Get();
		if (!IsValid(otherActor)) continue;

		bool bOtherFound = false;
		auto otherParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(otherActor, bOtherFound);
		if (bOtherFound && otherParticipant.GetObject() && otherParticipant->Execute_CanParticipateInDialogue(otherParticipant.GetObject()))
			OutAllParticipants.AddUnique(otherParticipant);
	}

	return true;
}
