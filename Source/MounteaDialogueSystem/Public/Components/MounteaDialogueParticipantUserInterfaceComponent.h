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
#include "Interfaces/Core/MounteaDialogueParticipantUIInterface.h"
#include "MounteaDialogueParticipantUserInterfaceComponent.generated.h"

struct FMounteaDialogueContextPayload;
/**
 * Mountea Dialogue Participant User Interface Component.
 *
 * Attach to any Actor (PlayerController, Pawn, NPC) to give it ownership of a
 * single dialogue UI target. Call BindToManager to connect it to a
 * UMounteaDialogueManager and start receiving UI signals.
 *
 * - Purely local: SetIsReplicatedByDefault(false).
 * - No auto-discovery: inert until BindToManager is called.
 * - Single target: owns exactly one UserInterface (UObject implementing
 *   IMounteaDialogueWBPInterface). No widget-typed fields.
 *
 * @see IMounteaDialogueParticipantUIInterface
 * @see UMounteaDialogueManager
 */
UCLASS(ClassGroup=(Mountea), Blueprintable,
	hideCategories=(Collision, AssetUserData, Cooking, Activation, Rendering, Sockets),
	AutoExpandCategories=("Mountea", "Dialogue"),
	meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue User Interface Component"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipantUserInterfaceComponent : public UActorComponent, public IMounteaDialogueParticipantUIInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueParticipantUserInterfaceComponent();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- IMounteaDialogueParticipantUIInterface — Manager Binding ------------------

public:

	virtual void SetParentManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager) override;
	virtual TScriptInterface<IMounteaDialogueManagerInterface> GetParentManager_Implementation() const override;
	virtual void BindToManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;
	virtual void UnbindFromManager_Implementation() override;

	// --- IMounteaDialogueParticipantUIInterface — UI Target -----------------------

	virtual void SetUserInterface_Implementation(UObject* NewUserInterface) override;
	virtual UObject* GetUserInterface_Implementation() const override;

	// --- IMounteaDialogueParticipantUIInterface — Screen UI Lifecycle -------------

	virtual bool CreateDialogueUI_Implementation(FString& Message) override;
	virtual bool UpdateDialogueUI_Implementation(FString& Message, const FString& Command) override;
	virtual bool CloseDialogueUI_Implementation() override;
	virtual void ExecuteWidgetCommand_Implementation(const FString& Command) override;

	// --- IMounteaDialogueParticipantUIInterface — Input Forwarding ----------------

	virtual void RequestSelectNode_Implementation(const FGuid& NodeGuid) override;
	virtual void RequestSkipDialogueRow_Implementation() override;
	virtual void RequestCloseDialogue_Implementation() override;
	virtual void RequestProcessDialogueRow_Implementation() override;

	// --- IMounteaDialogueParticipantUIInterface — Signal Dispatch -----------------

	virtual void DispatchUISignal_Implementation(const FMounteaDialogueUISignal& Signal) override;
	
	// --- IMounteaDialogueParticipantUIInterface — Widget Created -----------------
	
	virtual FOnDialogueWidgetCreated& GetOnDialogueWidgetCreatedEventHandle() override
	{
		return OnDialogueWidgetCreated;
	};

protected:

	// --- Lifecycle delegate handlers bound in BindLifecycleDelegates --------------

	UFUNCTION()
	void OnDialogueStarted(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueClosed(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueFailed(const FString& ErrorMessage);
	UFUNCTION()
	void OnDialogueNodeStarted(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueNodeFinished(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueNodeSelected(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueRowStarted(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueRowFinished(UMounteaDialogueContext* Context);

private:

	/** Bound to Manager->GetDialogueUISignalEventHandle(). Routes to DispatchUISignal. */
	UFUNCTION()
	void OnUISignalReceived(const FMounteaDialogueUISignal& Signal);

	/** Bound to Manager->GetDialogueContextUpdatedEventHande(). Drains the pending signal queue. */
	UFUNCTION()
	void OnContextVersionUpdated(UMounteaDialogueContext* Context);

	void BindLifecycleDelegates(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);
	void UnbindLifecycleDelegates(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);

	/**
	 * Immediately executes a signal — routes by command or reconcile flag.
	 * Called from DispatchUISignal_Implementation for ready signals and from DrainPendingSignals.
	 */
	void ExecuteUISignal(const FMounteaDialogueUISignal& Signal);

	/**
	 * Drains queued signals whose RequiredContextVersion <= CurrentVersion and match SessionGUID.
	 * Signals with INT32_MAX RequiredContextVersion are treated as clear sentinels — flushes queue.
	 */
	void DrainPendingSignals(int32 CurrentVersion, const FGuid& SessionGUID);

	// --- Signal queue state (non-replicated) ------------------------------------

	TArray<FMounteaDialogueUISignal> PendingUISignals;
	int32 LastDispatchedContextVersion = 0;
	FGuid LastDispatchedSessionGUID;

	// --- Reconcile state (non-replicated, mirrors Manager caches) ---------------

	/**
	 * Rebuilds the local UI from the current session payload.
	 * Called when a signal arrives with bForceReconcile = true.
	 */
	void ReconcileFromPayload(const FMounteaDialogueContextPayload& Payload);

	/**
	 * Applies a widget command to UserInterface via IMounteaDialogueWBPInterface::RefreshDialogueWidget.
	 */
	void ApplyWidgetCommand(const FString& Command);

	/** Resets all reconcile caches for a new session. */
	void ResetClientSyncCaches(const FGuid& SessionGUID);

	FString LastAppliedCommand;
	uint8 LastReconciledViewMode = 0;
	FGuid LastReconciledRowGUID;
	int32 LastReconciledRowIndex = INDEX_NONE;
	uint32 LastReconciledOptionsHash = 0;
	int32 LastAppliedPayloadCommandVersion = 0;
	FGuid LastClientSyncSessionGUID;

	// --- Client prediction state (non-replicated) --------------------------------

	bool IsPredictionEnabled() const;
	void BeginSelectPrediction(const FGuid& NodeGuid);
	void BeginClosePrediction(const FGuid& SessionGuid);
	void ResolvePredictionFromPayload(const FMounteaDialogueContextPayload& Payload);
	void RollbackPrediction(const FString& Reason);
	void ClearPredictionState();
	void ApplyPredictedUICommand(const FString& Command);

	UFUNCTION()
	void OnPredictionTimeout();

	bool bPredictionEnabled = true;
	EDialogueClientPredictionType PendingPredictionType = EDialogueClientPredictionType::None;
	FGuid PendingPredictionSessionGUID;
	FGuid PendingPredictionNodeGUID;
	int32 PendingPredictionStartContextVersion = 0;
	bool bPredictedCloseEventFired = false;
	FGuid PredictedCloseSessionGUID;
	FTimerHandle PendingPredictionHandle;

protected:
	
	/** Event called when Widget is created. */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Participant|UI")
	FOnDialogueWidgetCreated OnDialogueWidgetCreated;

	/**
	 * The single UI target object. Must implement IMounteaDialogueWBPInterface.
	 * May be a UUserWidget or a UWidgetComponent. Accessed exclusively via interface calls.
	 */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadWrite,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(NoResetToDefault))
	TObjectPtr<UObject> UserInterface = nullptr;

	/** Cached manager reference set via BindToManager. Not replicated. */
	UPROPERTY(VisibleAnywhere,
		BlueprintReadWrite,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(NoResetToDefault))
	TScriptInterface<IMounteaDialogueManagerInterface> ParentManager;
};
