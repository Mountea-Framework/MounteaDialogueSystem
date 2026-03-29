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
#include "Subsystems/WorldSubsystem.h"
#include "MounteaDialogueWorldSubsystem.generated.h"

class UMounteaDialogueSession;
class UMounteaDialogueManager;
struct FDialogueStartRequest;
class IMounteaDialogueParticipantInterface;

/**
 * UMounteaDialogueWorldSubsystem manages active dialogue sessions and registered managers
 * for the current world. It serves as the central registry for all dialogue activity,
 * bridging start requests to session allocation and providing participant lookup.
 *
 * @see UMounteaDialogueSession
 * @see UMounteaDialogueManager
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:

	/**
	 * Registers a Dialogue Manager with the subsystem.
	 * Called from UMounteaDialogueManager::BeginPlay.
	 *
	 * @param Manager  Manager component to register.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|World Subsystem",
		meta=(CustomTag="MounteaK2Setter"))
	void RegisterManager(UMounteaDialogueManager* Manager);

	/**
	 * Unregisters a Dialogue Manager from the subsystem.
	 * Called from UMounteaDialogueManager::EndPlay.
	 *
	 * @param Manager  Manager component to unregister.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|World Subsystem",
		meta=(CustomTag="MounteaK2Setter"))
	void UnregisterManager(UMounteaDialogueManager* Manager);

	/**
	 * Returns a read-only view of all currently registered managers.
	 */
	const TArray<TObjectPtr<UMounteaDialogueManager>>& GetRegisteredManagers() const
	{
		return RegisteredManagers;
	}

	/**
	 * Returns a read-only view of all currently active dialogue sessions.
	 */
	const TArray<TObjectPtr<UMounteaDialogueSession>>& GetActiveSessions() const
	{
		return ActiveSessions;
	}

	/**
	 * Finds the UMounteaDialogueSession component on the current AGameState.
	 * Returns null if the component is not present — project must add it to their GameState BP.
	 */
	UMounteaDialogueSession* GetGameStateSession() const;

	/**
	 * Handles an incoming start request forwarded from a manager's server RPC.
	 * Resolves participants, validates them, creates context, and triggers dialogue start.
	 * Must be called on the server.
	 *
	 * @param Manager  The manager that initiated the request.
	 * @param Request  The packed start request with soft actor references.
	 */
	void HandleStartRequest(UMounteaDialogueManager* Manager, const FDialogueStartRequest& Request);

	/**
	 * Starts an environment (NPC-NPC or monologue) dialogue directly on the server.
	 * No RPC — environment managers are server-side only.
	 *
	 * @param Manager  The environment manager that owns this dialogue.
	 * @param Request  The packed start request.
	 */
	void RequestStartEnvironmentDialogue(UMounteaDialogueManager* Manager, const FDialogueStartRequest& Request);

	/**
	 * Releases the single-dialogue lock if the caller matches the active lock owner.
	 * Server-only.
	 */
	void ReleaseDialogueLock(UMounteaDialogueManager* Manager, const FGuid& SessionGUID = FGuid());

private:

	/**
	 * Resolves soft actor references in Request into typed participant interfaces.
	 * Determines player participant from manager's owner chain.
	 *
	 * @param Manager                The calling manager (used to identify player participant).
	 * @param Request                Start request with soft actor refs.
	 * @param OutPlayerParticipant   Resolved player participant (null for environment dialogue).
	 * @param OutMainParticipant     Resolved primary NPC participant.
	 * @param OutAllParticipants     All resolved participants combined.
	 * @param OutErrors              Collected validation error messages.
	 * @return true if all required participants resolved and validated successfully.
	 */
	bool ResolveParticipants(
		UMounteaDialogueManager* Manager,
		const FDialogueStartRequest& Request,
		TScriptInterface<IMounteaDialogueParticipantInterface>& OutPlayerParticipant,
		TScriptInterface<IMounteaDialogueParticipantInterface>& OutMainParticipant,
		TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& OutAllParticipants,
		TArray<FText>& OutErrors) const;

	bool TryAcquireDialogueLock(UMounteaDialogueManager* Manager);

private:

	/**
	 * All dialogue managers registered in the current world.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMounteaDialogueManager>> RegisteredManagers;

	/**
	 * All active dialogue sessions in the current world.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMounteaDialogueSession>> ActiveSessions;

	UPROPERTY(Transient)
	TWeakObjectPtr<UMounteaDialogueManager> ActiveManager;

	UPROPERTY(Transient)
	FGuid ActiveSessionGUID;

	UPROPERTY(Transient)
	bool bDialogueLockActive = false;
};
