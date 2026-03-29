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

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/MounteaDialogueContextPayload.h"
#include "Interfaces/Core/MounteaDialogueConditionContextInterface.h"
#include "MounteaDialogueSession.generated.h"

class UMounteaDialogueWorldSubsystem;
class UMounteaDialogueManager;
class UMounteaDialogueGraphNode;

/**
 * UMounteaDialogueSession is the server-authoritative state machine for a single active dialogue.
 * It owns FMounteaDialogueContextPayload, executes graph traversal, evaluates edge conditions,
 * and replicates state to all connected clients through GameState.
 *
 * This component is intended to be attached to AGameState.
 *
 * @see FMounteaDialogueContextPayload
 * @see UMounteaDialogueWorldSubsystem
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, 
	hideCategories=(Collision, AssetUserData, Cooking, Activation, Rendering, Sockets), 
	AutoExpandCategories=("Mountea", "Dialogue"), 
	meta=(BlueprintSpawnableComponent, DisplayName = "Mountea Dialogue Session"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSession : public UActorComponent, public IMounteaDialogueConditionContextInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueSession();

protected:

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	/**
	 * The replicated dialogue context payload.
	 * Owned exclusively by the server. Clients receive updates via OnRep_ContextPayload.
	 * Never mutated directly — always written through WriteContextPayload.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_ContextPayload)
	FMounteaDialogueContextPayload ContextPayload;

	UFUNCTION()
	void OnRep_ContextPayload();

public:

	/**
	 * Returns the current context payload.
	 * Read-only access. Mutation must go through WriteContextPayload on the server.
	 */
	const FMounteaDialogueContextPayload& GetContextPayload() const
	{
		return ContextPayload;
	}

	/**
	 * Sets the server-authoritative manager that owns the active session flow.
	 * The server notify path targets this manager directly to avoid local-player filtering issues.
	 */
	void SetAuthoritativeManager(UMounteaDialogueManager* Manager);

	/**
	 * Server-only session finalize hook.
	 * Persists traversed-path data for all participants in the current payload and
	 * clears session-side manager ownership.
	 */
	void FinalizeSession();

	/**
	 * Server-only. Writes a new payload, increments ContextVersion, marks the property dirty
	 * for push-model replication, and dispatches to the authoritative server manager.
	 *
	 * @param NewPayload  The fully constructed payload to write.
	 */
	void WriteContextPayload(FMounteaDialogueContextPayload NewPayload);

	void SetRoleOverride(EDialogueParticipantType Role, const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant);
	TScriptInterface<IMounteaDialogueParticipantInterface> GetRoleOverride(EDialogueParticipantType Role) const;

	bool HandleSelectNode(UMounteaDialogueManager* Manager, const FGuid& SessionGUID, const FGuid& NodeGUID);
	bool HandleSkipDialogueRow(UMounteaDialogueManager* Manager, const FGuid& SessionGUID);
	bool HandleNodeProcessed(UMounteaDialogueManager* Manager, const FGuid& SessionGUID);
	bool HandleDialogueRowProcessed(UMounteaDialogueManager* Manager, const FGuid& SessionGUID, bool bForceFinish);
	bool HandleProcessDialogueRow(UMounteaDialogueManager* Manager, const FGuid& SessionGUID);
	bool HandlePrepareNode(UMounteaDialogueManager* Manager, const FGuid& SessionGUID);
	bool HandleNodePrepared(UMounteaDialogueManager* Manager, const FGuid& SessionGUID);
	bool HandleProcessNode(UMounteaDialogueManager* Manager, const FGuid& SessionGUID);

	// ~IMounteaDialogueConditionContextInterface
	/**
	 * Returns traversed path from all registered participants.
	 * Aggregated at session level for edge condition evaluation.
	 * The per-session accumulator is deferred to TraversedPath migration (Stage 5).
	 */
	virtual TArray<FDialogueTraversePath> GetConditionTraversedPath_Implementation() const override;

	/**
	 * Returns the participant currently active in this session.
	 */
	virtual TScriptInterface<IMounteaDialogueParticipantInterface> GetConditionActiveParticipant_Implementation() const override
	{ return ContextPayload.ActiveDialogueParticipant; };

	/**
	 * Returns the real session GUID from the authoritative payload.
	 */
	virtual FGuid GetConditionSessionGUID_Implementation() const override
	{ return ContextPayload.SessionGUID; };
	// ~IMounteaDialogueConditionContextInterface

private:

	/**
	 * Dispatches the latest payload to managers.
	 * Server path targets the authoritative manager directly.
	 * Client path notifies only managers whose owners are locally controlled.
	 */
	void NotifyLocalManagers() const;
	void AddTraversedNode(const UMounteaDialogueGraphNode* TraversedNode);
	bool IsSessionRequestValid(UMounteaDialogueManager* Manager, const FGuid& SessionGUID, const TCHAR* ActionName) const;
	bool SyncPayloadFromManagerContext(UMounteaDialogueManager* Manager);

	TWeakObjectPtr<UMounteaDialogueManager> AuthoritativeManager;
	TMap<int32, TScriptInterface<IMounteaDialogueParticipantInterface>> RoleOverrides;
	TArray<FDialogueTraversePath> SessionTraversedPath;
	int32 LastDeliveredContextVersion = 0;
};
