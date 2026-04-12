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
#include "Data/MounteaDialogueContextPayload.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "GameFramework/GameStateBase.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"

void UMounteaDialogueWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMounteaDialogueWorldSubsystem::Deinitialize()
{
	RegisteredManagers.Empty();
	ActiveSessions.Empty();
	ActiveManager.Reset();
	ActiveSessionGUID.Invalidate();
	bDialogueLockActive = false;

	Super::Deinitialize();
}

void UMounteaDialogueWorldSubsystem::RegisterManager(UMounteaDialogueManager* Manager)
{
	if (!Manager) return;
	RegisteredManagers.AddUnique(Manager);

	UMounteaDialogueSession* session = GetGameStateSession();
	if (IsValid(session))
		session->TryDispatchPendingClientPayload();
}

void UMounteaDialogueWorldSubsystem::UnregisterManager(UMounteaDialogueManager* Manager)
{
	if (!Manager) return;

	if (ActiveManager.Get() == Manager)
		ReleaseDialogueLock(Manager, ActiveSessionGUID);

	RegisteredManagers.Remove(Manager);
}

UMounteaDialogueSession* UMounteaDialogueWorldSubsystem::GetGameStateSession() const
{
	AGameStateBase* gameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return gameState ? gameState->FindComponentByClass<UMounteaDialogueSession>() : nullptr;
}

void UMounteaDialogueWorldSubsystem::HandleStartRequest(UMounteaDialogueManager* Manager, const FDialogueStartRequest& Request)
{
	if (!Manager) 
		return;

	if (!TryAcquireDialogueLock(Manager))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[HandleStartRequest] Another dialogue is already active. Only one active dialogue is supported."));
		return;
	}

	bool bLockAcquired = true;
	const auto ReleaseLockIfNeeded = [this, &bLockAcquired, Manager]()
	{
		if (!bLockAcquired)
			return;

		ReleaseDialogueLock(Manager);
		bLockAcquired = false;
	};

	if (!Request.IsValid())
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[HandleStartRequest] Invalid start request. MainParticipantActor must be provided."));
		ReleaseLockIfNeeded();
		return;
	}

	TScriptInterface<IMounteaDialogueParticipantInterface> playerParticipant;
	TScriptInterface<IMounteaDialogueParticipantInterface> mainParticipant;
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> allParticipants;
	TArray<FText> errors;
	errors.Add(FText::FromString(TEXT("[HandleStartRequest]")));

	if (!ResolveParticipants(Manager, Request, playerParticipant, mainParticipant, allParticipants, errors))
	{
		const FText combined = FText::Join(FText::FromString(TEXT("\n")), errors);
		Manager->GetDialogueFailedEventHandle().Broadcast(combined.ToString());
		ReleaseLockIfNeeded();
		return;
	}

	UMounteaDialogueSession* session = GetGameStateSession();
	if (!session)
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[HandleStartRequest] No UMounteaDialogueSession found on GameState. Add the component to your GameState Blueprint."));
		ReleaseLockIfNeeded();
		return;
	}

	session->SetAuthoritativeManager(Manager);

	UMounteaDialogueGraph* resolvedGraph = nullptr;
	const bool bUseGraphOverride = !Request.DialogueGraph.IsNull();
	if (bUseGraphOverride)
	{
		resolvedGraph = Request.DialogueGraph.LoadSynchronous();
		if (!IsValid(resolvedGraph))
		{
			Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[HandleStartRequest] DialogueGraph override is set but failed to load."));
			ReleaseLockIfNeeded();
			return;
		}
	}
	else
		resolvedGraph = mainParticipant->Execute_GetDialogueGraph(mainParticipant.GetObject());

	if (!IsValid(resolvedGraph))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[HandleStartRequest] Unable to resolve active dialogue graph."));
		ReleaseLockIfNeeded();
		return;
	}

	UMounteaDialogueGraphNode* graphStartNode = resolvedGraph->GetStartNode();
	if (!IsValid(graphStartNode))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(
			FString::Printf(TEXT("[HandleStartRequest] Dialogue graph '%s' has no start node."), *resolvedGraph->GetName()));
		ReleaseLockIfNeeded();
		return;
	}

	UMounteaDialogueGraphNode* firstRuntimeNode = UMounteaDialogueTraversalStatics::GetFirstChildNode(graphStartNode);
	if (!IsValid(firstRuntimeNode))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(
			FString::Printf(TEXT("[HandleStartRequest] Dialogue graph '%s' start node has no valid child to begin from."), *resolvedGraph->GetName()));
		ReleaseLockIfNeeded();
		return;
	}

	// Lift resolved state into the replicated payload.
	FMounteaDialogueContextPayload payload;
	payload.SessionGUID = FGuid::NewGuid();
	ActiveSessionGUID = payload.SessionGUID;

	UMounteaDialogueContext* tempContext = NewObject<UMounteaDialogueContext>(Manager);
	if (!IsValid(tempContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[HandleStartRequest] Failed to create temporary context."));
		ReleaseLockIfNeeded();
		return;
	}

	tempContext->SessionGUID = payload.SessionGUID;
	tempContext->DialogueParticipants = allParticipants;
	tempContext->SetDialogueContext(firstRuntimeNode, TArray<UMounteaDialogueGraphNode*>());
	tempContext->UpdateActiveDialogueRow(UMounteaDialogueTraversalStatics::GetSpeechData(firstRuntimeNode));
	tempContext->UpdateActiveDialogueRowDataIndex(0);

	TScriptInterface<IMounteaDialogueParticipantInterface> initialActiveParticipant = mainParticipant;
	const TScriptInterface<IMounteaDialogueParticipantInterface> rolePreferredParticipant =
		UMounteaDialogueParticipantStatics::GetParticipantByType(
			allParticipants,
			EDialogueParticipantType::NPC,
			Manager);
	if (rolePreferredParticipant.GetObject() && rolePreferredParticipant.GetInterface())
		initialActiveParticipant = rolePreferredParticipant;

	tempContext->SetActiveDialogueParticipant(initialActiveParticipant);
	const TScriptInterface<IMounteaDialogueParticipantInterface> resolvedActiveParticipant =
		UMounteaDialogueTraversalStatics::ResolveActiveParticipant(tempContext);
	UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(tempContext, resolvedActiveParticipant);

	TArray<UMounteaDialogueGraphNode*> filteredChildren =
		UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(firstRuntimeNode, tempContext);
	UMounteaDialogueTraversalStatics::SortNodes(filteredChildren);
	tempContext->UpdateAllowedChildrenNodes(filteredChildren);

	payload.DialogueParticipants = tempContext->DialogueParticipants;
	payload.ActiveDialogueParticipant = tempContext->ActiveDialogueParticipant;
	payload.ActiveNodeGUID = firstRuntimeNode->GetNodeGUID();
	payload.ActiveGraphGUID = firstRuntimeNode->GetGraphGUID();
	for (const auto& child : filteredChildren)
	{
		if (IsValid(child))
			payload.AllowedChildNodeGUIDs.Add(child->GetNodeGUID());
	}
	payload.ActiveDialogueRow = tempContext->ActiveDialogueRow;
	payload.ActiveDialogueRowDataIndex = tempContext->ActiveDialogueRowDataIndex;

	// WriteContextPayload replicates to clients and notifies the server-side manager via NotifyLocalManagers.
	// This guarantees context is live on the server before the state transition below fires PrepareNode.
	session->WriteContextPayload(MoveTemp(payload));

	ActiveSessions.AddUnique(session);

	// State transition happens after payload is written — server context is guaranteed valid by the time
	// StartDialogue → PrepareNode runs via DialogueStartRequestReceived.
	Manager->GetDialogueStartRequestedResultEventHandle().Broadcast(true, TEXT("OK"));
	bLockAcquired = false;
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

	const UWorld* managerWorld = Manager->GetWorld();
	const auto ResolveRequestActor = [managerWorld](AActor* ActorRef, const TSoftObjectPtr<AActor>& SoftRef) -> AActor*
	{
		if (IsValid(ActorRef))
		{
			if (!managerWorld || ActorRef->GetWorld() == managerWorld)
				return ActorRef;

			LOG_WARNING(TEXT("[ResolveParticipants] Ignoring actor-ref '%s' from different world."), *GetNameSafe(ActorRef))
		}

		if (!SoftRef.IsNull())
		{
			AActor* softResolvedActor = SoftRef.Get();
			if (IsValid(softResolvedActor) && (!managerWorld || softResolvedActor->GetWorld() == managerWorld))
				return softResolvedActor;

			if (IsValid(softResolvedActor))
				LOG_WARNING(TEXT("[ResolveParticipants] Ignoring soft-ref actor '%s' from different world."), *GetNameSafe(softResolvedActor))
		}

		return nullptr;
	};
	
	// Resolve main (NPC) participant
	AActor* mainActor = ResolveRequestActor(Request.MainParticipantActorRef, Request.MainParticipantActor);
	if (!mainActor)
	{
		OutErrors.Add(NSLOCTEXT("ResolveParticipants", "NoMainActor", "MainParticipantActor could not be resolved!"));
		return false;
	}

	bool bFound = false;
	OutMainParticipant = UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(mainActor, bFound);
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
		if (APawn* playerPawn = UMounteaDialogueParticipantStatics::FindPlayerPawn(ownerActor, searchDepth))
		{
			bool bPlayerFound = false;
			OutPlayerParticipant = UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(playerPawn, bPlayerFound);
			if (bPlayerFound && OutPlayerParticipant.GetObject() && OutPlayerParticipant->Execute_CanParticipateInDialogue(OutPlayerParticipant.GetObject()))
				OutAllParticipants.AddUnique(OutPlayerParticipant);
			else
				OutPlayerParticipant = nullptr;
		}
	}

	// Resolve other participants
	for (AActor* otherActorRef : Request.OtherParticipantActorRefs)
	{
		AActor* otherActor = ResolveRequestActor(otherActorRef, TSoftObjectPtr<AActor>());
		if (!IsValid(otherActor))
			continue;

		bool bOtherFound = false;
		auto otherParticipant = UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(otherActor, bOtherFound);
		if (bOtherFound && otherParticipant.GetObject() && otherParticipant->Execute_CanParticipateInDialogue(otherParticipant.GetObject()))
			OutAllParticipants.AddUnique(otherParticipant);
	}

	for (const auto& softRef : Request.OtherParticipantActors)
	{
		AActor* otherActor = ResolveRequestActor(nullptr, softRef);
		if (!IsValid(otherActor))
			continue;

		bool bOtherFound = false;
		auto otherParticipant = UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(otherActor, bOtherFound);
		if (bOtherFound && otherParticipant.GetObject() && otherParticipant->Execute_CanParticipateInDialogue(otherParticipant.GetObject()))
			OutAllParticipants.AddUnique(otherParticipant);
	}

	return true;
}

bool UMounteaDialogueWorldSubsystem::TryAcquireDialogueLock(UMounteaDialogueManager* Manager)
{
	if (!IsValid(Manager))
		return false;

	if (!bDialogueLockActive)
	{
		bDialogueLockActive = true;
		ActiveManager = Manager;
		ActiveSessionGUID.Invalidate();
		return true;
	}

	if (!ActiveManager.IsValid())
	{
		bDialogueLockActive = false;
		return TryAcquireDialogueLock(Manager);
	}

	return false;
}

void UMounteaDialogueWorldSubsystem::ReleaseDialogueLock(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!bDialogueLockActive)
		return;

	if (IsValid(Manager) && ActiveManager.IsValid() && ActiveManager.Get() != Manager)
		return;

	if (SessionGUID.IsValid() && ActiveSessionGUID.IsValid() && SessionGUID != ActiveSessionGUID)
		return;

	bDialogueLockActive = false;
	ActiveManager.Reset();
	ActiveSessionGUID.Invalidate();

	if (UMounteaDialogueSession* session = GetGameStateSession())
		ActiveSessions.Remove(session);
}
