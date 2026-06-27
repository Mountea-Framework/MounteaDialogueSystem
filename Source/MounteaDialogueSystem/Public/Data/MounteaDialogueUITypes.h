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
#include "MounteaDialogueUITypes.generated.h"

/**
 * Tracks the type of optimistic prediction the client has applied locally.
 * Used by UMounteaDialogueParticipantUserInterfaceComponent to determine
 * whether incoming payload changes should reconcile or confirm a prediction.
 */
UENUM(BlueprintType)
enum class EDialogueClientPredictionType : uint8
{
	None,
	Select,
	Close
};

/**
 * Version-stamped UI command delivered from the server Manager to the owning client
 * via Client_DispatchUISignal RPC.
 *
 * The receiving UMounteaDialogueParticipantUserInterfaceComponent queues signals whose
 * RequiredContextVersion has not yet been satisfied and drains them in ascending version
 * order once OnContextPayloadUpdated brings the local context up to date.
 *
 * @see UMounteaDialogueParticipantUserInterfaceComponent
 * @see UMounteaDialogueManager::Client_DispatchUISignal
 */
USTRUCT(BlueprintType)
struct MOUNTEADIALOGUESYSTEM_API FMounteaDialogueUISignal
{
	GENERATED_BODY()

	/**
	 * Widget command to execute (see MounteaDialogueWidgetCommands namespace).
	 * Empty string is treated as a no-op.
	 */
	UPROPERTY(BlueprintReadOnly,
		Category="Mountea|Dialogue|UI")
	FString Command;

	/**
	 * Session this signal belongs to.
	 * Signals whose SessionGUID does not match the active session are silently dropped.
	 */
	UPROPERTY(BlueprintReadOnly,
		Category="Mountea|Dialogue|UI")
	FGuid SessionGUID;

	/**
	 * Minimum ContextVersion the local payload must reach before this signal may be dispatched.
	 * Set to 0 for optimistic / immediate dispatch (client prediction path).
	 */
	UPROPERTY(BlueprintReadOnly,
		Category="Mountea|Dialogue|UI")
	int32 RequiredContextVersion = 0;

	/**
	 * When true the component performs a full ReconcileFromPayload instead of
	 * executing Command directly.
	 * Used for create and close hard-sync edges where state must be fully rebuilt.
	 */
	UPROPERTY(BlueprintReadOnly,
		Category="Mountea|Dialogue|UI")
	bool bForceReconcile = false;
};
