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
#include "MounteaDialogueSession.generated.h"

class UMounteaDialogueWorldSubsystem;
class UMounteaDialogueManager;

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
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSession : public UActorComponent
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
	 * Server-only. Writes a new payload, increments ContextVersion, marks the property dirty
	 * for push-model replication, and calls NotifyLocalManagers for the listen-server host path.
	 *
	 * @param NewPayload  The fully constructed payload to write.
	 */
	void WriteContextPayload(FMounteaDialogueContextPayload NewPayload);

private:

	/**
	 * Iterates all managers registered in UMounteaDialogueWorldSubsystem and calls
	 * OnContextPayloadUpdated on those whose owning actor is locally controlled.
	 * Used on the listen-server host because OnRep does not fire on the server side.
	 */
	void NotifyLocalManagers() const;
};
