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
#include "GameplayTagContainer.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "UObject/Interface.h"
#include "MounteaDialogueConditionContextInterface.generated.h"

class IMounteaDialogueParticipantInterface;

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueConditionContextInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Condition context interface passed to UMounteaDialogueConditionBase::EvaluateCondition.
 *
 * Provides the condition with read-only access to traversal history, the active participant,
 * and the owning session GUID. Implemented by UMounteaDialogueSession (server) and
 * UMounteaDialogueContext (local read-only view).
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueConditionContextInterface
{
	GENERATED_BODY()

public:

	/**
	 * Returns the list of nodes traversed during this dialogue session.
	 * Used by conditions that gate on visit counts or prior choices.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Condition")
	TArray<FDialogueTraversePath> GetConditionTraversedPath() const;
	virtual TArray<FDialogueTraversePath> GetConditionTraversedPath_Implementation() const = 0;

	/**
	 * Returns the participant that is currently active (speaking or acting).
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Condition")
	TScriptInterface<IMounteaDialogueParticipantInterface> GetConditionActiveParticipant() const;
	virtual TScriptInterface<IMounteaDialogueParticipantInterface> GetConditionActiveParticipant_Implementation() const = 0;

	/**
	 * Returns the GUID of the owning dialogue session.
	 * Can be used to correlate conditions with specific sessions.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Condition")
	FGuid GetConditionSessionGUID() const;
	virtual FGuid GetConditionSessionGUID_Implementation() const = 0;
};
