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
#include "Data/MounteaDialogueUITypes.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManager.generated.h"

/**
 *  Mountea Dialogue Manager Component
 * 
 * Should be attached to Player State in order to be replication ready.
 * ❔ Allows any Actor to be Dialogue Manager
 * ❔ Implements 'IMounteaDialogueManagerInterface'.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable,  AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"), 
	meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManager : public UActorComponent, public IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueManager();

protected:
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	virtual FDialogueStartRequested& GetDialogueStartRequestedEventHandle() override
	{ return OnDialogueStartRequested; };
	virtual FDialogueStartRequestedResult& GetDialogueStartRequestedResultEventHandle() override
	{ return OnDialogueStartRequestedResult; };
	virtual FDialogueEvent& GetDialogueStartedEventHandle() override
	{ return OnDialogueStarted; };
	virtual FDialogueEvent& GetDialogueClosedEventHandle() override
	{ return OnDialogueClosed; };
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() override
	{ return OnDialogueContextUpdated; };
	virtual FDialogueUserInterfaceChanged& GetDialogueUserInterfaceChangedEventHandle() override
	{ return OnDialogueUserInterfaceChanged; };
	virtual FDialogueNodeEvent& GetDialogueNodeSelectedEventHandle() override
	{ return  OnDialogueNodeSelected; };
	virtual FDialogueNodeEvent& GetDialogueNodeStartedEventHandle() override
	{ return OnDialogueNodeStarted; };
	virtual FDialogueNodeEvent& GetDialogueNodeFinishedEventHandle() override
	{ return OnDialogueNodeFinished; };
	virtual FDialogueRowEvent& GetDialogueRowStartedEventHandle() override
	{ return OnDialogueRowStarted; };
	virtual FDialogueRowEvent& GetDialogueRowFinishedEventHandle() override
	{ return OnDialogueRowFinished; };
	virtual FDialogueFailed& GetDialogueFailedEventHandle() override
	{ return OnDialogueFailed; };
	virtual FDialogueManagerStateChanged& GetDialogueManagerStateChangedEventHandle() override
	{ return OnDialogueManagerStateChanged; };
	virtual FDialogueWidgetCommand& GetDialogueWidgetCommandHandle() override
	{ return OnDialogueWidgetCommandRequested; };
	virtual FTimerHandle& GetDialogueRowTimerHandle() override
	{ return TimerHandle_RowTimer; };
	virtual FDialogueUISignalEvent& GetDialogueUISignalEventHandle() override
	{ return OnDialogueUISignalRequested; };

protected:

	void ProcessStateUpdated();
	UFUNCTION()
	void DialogueFailed(const FString& ErrorMessage);

	void StartParticipants();
	void StopParticipants();
	void NotifyParticipants(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& Participants);
	
public:

	virtual AActor* GetOwningActor_Implementation() const override;
	virtual EDialogueManagerState GetManagerState_Implementation() const override;
	virtual void SetManagerState(const EDialogueManagerState NewState) override;
	virtual EDialogueManagerState GetDefaultManagerState_Implementation() const override;
	virtual void SetDefaultManagerState(const EDialogueManagerState NewState) override;
	virtual EDialogueManagerType GetDialogueManagerType() const override;

	virtual bool CanStartDialogue_Implementation() const override;;
	virtual UMounteaDialogueContext* GetDialogueContext_Implementation() const override;
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) override;
	virtual void UpdateDialogueContext_Implementation(UMounteaDialogueContext* NewContext) override;

	virtual void RequestStartDialogue_Implementation(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants) override;
	virtual void RequestCloseDialogue_Implementation() override;
	UFUNCTION()
	virtual void DialogueStartRequestReceived(const bool bResult, const FString& ResultMessage) override;
	virtual void StartDialogue_Implementation() override;
	virtual void CloseDialogue_Implementation() override;
	virtual void CleanupDialogue_Implementation() override;
	
	virtual void PrepareNode_Implementation() override;
	virtual void NodePrepared_Implementation() override;
	virtual void ProcessNode_Implementation() override;
	virtual void NodeProcessed_Implementation() override;
	virtual void SelectNode_Implementation(const FGuid& NodeGuid) override;

	virtual void ProcessDialogueRow_Implementation() override;
	virtual void DialogueRowProcessed_Implementation(const bool bForceFinish = false) override;
	virtual void SkipDialogueRow_Implementation() override;
	
	virtual void UpdateWorldDialogueUI_Implementation(const FString& Command) override;
	virtual bool AddDialogueUIObject_Implementation(UObject* NewDialogueObject) override;
	virtual bool AddDialogueUIObjects_Implementation(const TArray<UObject*>& NewDialogueObjects) override;
	virtual bool RemoveDialogueUIObject_Implementation(UObject* DialogueObjectToRemove) override;
	virtual bool RemoveDialogueUIObjects_Implementation(const TArray<UObject*>& DialogueObjectsToRemove) override;
	virtual void SetDialogueUIObjects_Implementation(const TArray<UObject*>& NewDialogueObjects) override;
	virtual void ResetDialogueUIObjects_Implementation() override;

	virtual bool CreateDialogueUI_Implementation(FString& Message) override;
	virtual bool UpdateDialogueUI_Implementation(FString& Message, const FString& Command) override;
	virtual bool CloseDialogueUI_Implementation() override;

	virtual void ExecuteWidgetCommand_Implementation(const FString& Command) override;
	virtual TSubclassOf<UUserWidget> GetDialogueWidgetClass() const override;
	virtual void SetDialogueWidgetClass(TSubclassOf<UUserWidget> NewWidgetClass) override;
	virtual void SetDialogueWidget_Implementation(UUserWidget* NewDialogueWidget) override;
	virtual UUserWidget* GetDialogueWidget_Implementation() const override;
	virtual int32 GetDialogueWidgetZOrder_Implementation() const override;
	virtual void SetDialogueWidgetZOrder_Implementation(const int32 NewZOrder) override;

	/**
	 * Called by UMounteaDialogueSession (via OnRep or NotifyLocalManagers) when the replicated
	 * context payload is updated. Rebuilds the local UMounteaDialogueContext read-only view
	 * and fires the existing context-updated delegates.
	 *
	 * @param Payload  The latest payload received from the session.
	 */
	void OnContextPayloadUpdated(const FMounteaDialogueContextPayload& Payload);

private:

	UFUNCTION(Server, Reliable)
	void SetManagerState_Server(const EDialogueManagerState NewState);
	UFUNCTION(Server, Unreliable)
	void SetDefaultManagerState_Server(const EDialogueManagerState NewState);

	/**
	 * Routes a dialogue start request to the server.
	 * Server validates participants and allocates a UMounteaDialogueSession via the world subsystem.
	 */
	UFUNCTION(Server, Reliable)
	void RequestStartDialogue_Server(const FDialogueStartRequest& Request);

	/**
	 * Routes a node selection to the server for validation and graph advancement.
	 */
	UFUNCTION(Server, Reliable)
	void RequestSelectNode_Server(FGuid SessionGUID, FGuid NodeGUID);

	/**
	 * Routes a row skip request to the server.
	 */
	UFUNCTION(Server, Reliable)
	void RequestSkipRow_Server(FGuid SessionGUID);

	UFUNCTION(Server, Reliable)
	void RequestNodeProcessed_Server(FGuid SessionGUID);

	UFUNCTION(Server, Reliable)
	void RequestDialogueRowProcessed_Server(FGuid SessionGUID, bool bForceFinish);

	/**
	 * Routes a dialogue close request to the server.
	 */
	UFUNCTION(Server, Reliable)
	void RequestCloseDialogue_Server(FGuid SessionGUID);

	UFUNCTION(Server, Reliable)
	void CleanupDialogue_Server();

	/**
	 * Delivers a version-stamped UI signal to the owning client.
	 * Broadcasts OnDialogueUISignalRequested locally on the client.
	 * On a listen server the _Implementation fires in-process — host is treated as a client.
	 *
	 * @param Signal  Version-stamped UI command to deliver.
	 */
	UFUNCTION(Client, Reliable)
	void Client_DispatchUISignal(const FMounteaDialogueUISignal& Signal);

	/**
	 * Tells the owning client to purge all pending signals that belong to a completed session.
	 * Delivered as a sentinel signal with RequiredContextVersion = INT32_MAX.
	 *
	 * @param SessionGUID  Session whose queued signals should be discarded.
	 */
	UFUNCTION(Client, Reliable)
	void Client_ClearUISignals(const FGuid& SessionGUID);

	UFUNCTION()
	void OnRep_ManagerState();

	void ProcessWorldWidgetUpdate(const FString& Command);
	void ApplyReplicatedWidgetCommand(const FString& Command);
	void ReconcileClientUIFromPayload(const FMounteaDialogueContextPayload& Payload);
	void ReconcileClientAudioFromPayload(const FMounteaDialogueContextPayload& Payload, bool bShouldPlayRowAudio);
	bool IsCoreWidgetCommand(const FString& Command) const;
	bool ShouldReplayPayloadCommand(const FMounteaDialogueContextPayload& Payload) const;
	bool IsPredictionEnabled();
	uint32 BuildAllowedChildrenHash(const TArray<FGuid>& AllowedChildren) const;
	void BeginSelectPrediction(const FGuid& NodeGuid);
	void BeginClosePrediction(const FGuid& SessionGuid);
	void ResolvePredictionFromPayload(const FMounteaDialogueContextPayload& Payload);
	void ResolvePredictionFromManagerState();
	void RollbackPrediction(const FString& Reason);
	void ClearPredictionState();
	void ApplyPredictedUICommand(const FString& Command);
	void OnPredictionTimeout();
	void ResetClientSyncCaches(const FGuid& SessionGUID);

public:

	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueStartRequested OnDialogueStartRequested;
	
	/**
	 * Even called when Dialogue is Initialized.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueStartRequestedResult OnDialogueStartRequestedResult;
	
	/**
	 * Event called when Dialogue has started.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueEvent OnDialogueStarted;
	
	/**
	 * Event called when Dialogue has been closed.
	 * Could be either manual or automatic.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueEvent OnDialogueClosed;
	

	/**
	 * Event called when Dialogue Context is updated.
	 * ❗ Could be Null
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueContextUpdated OnDialogueContextUpdated;
	
	/**
	 * Event called when Dialogue Widget Class or Widget have changed.
	 *❗ Dialogue Widget Could be Null
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueUserInterfaceChanged OnDialogueUserInterfaceChanged;
	

	/**
	 * Event called when new Dialogue Node has been selected.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueNodeEvent OnDialogueNodeSelected;

	/**
	 * Event called when Dialogue Node has started.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueNodeEvent OnDialogueNodeStarted;
	
	/**
	 * Event called when Dialogue Node has finished.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueNodeEvent OnDialogueNodeFinished;
	
	/**
	 * Event called when Dialogue Row has started.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueRowEvent OnDialogueRowStarted;
	
	/**
	 * Event called when Dialogue Row has finished.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueRowEvent OnDialogueRowFinished;
	
	/**
	 * Event called once Dialogue Row Data update has been requested.
	 * Is never called from Code and is bound to `FinishedExecuteDialogueRow` function.
	 * Should be used carefully. Suggested usage is with Dialogue Row Data which are using `RowDurationMode::Manual`.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueRowEvent OnNextDialogueRowDataRequested;

	/**
	 * Event called if Dialogue fails to execute.
	 * Provides Error Message with explanation.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueFailed OnDialogueFailed;

	/**
	 * Event called when Dialogue State changes.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueManagerStateChanged OnDialogueManagerStateChanged;

	/**
	* Event called for all listening Dialogue Objects.
	*/
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueWidgetCommand OnDialogueWidgetCommandRequested;

	/**
	 * Fired locally when a UI signal arrives via Client_DispatchUISignal.
	 * UMounteaDialogueParticipantUserInterfaceComponent instances bind to this
	 * delegate to receive UI lifecycle commands from the server.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueUISignalEvent OnDialogueUISignalRequested;

protected:

	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue|Manager", 
		meta=(DisplayThumbnail=false))
	TObjectPtr<UObject> DialogueInstigator;

	/**
	 * Manager based Dialogue Widget Class.
	 * ❔ Could be left empty if Project Settings are setup properly
	 * ❗ Must implement MounteaDialogueWBPInterface
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue|Manager", 
		DisplayName="Dialogue Widget Class Override", 
		meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface"))
	TSubclassOf<UUserWidget> DialogueWidgetClass = nullptr;

	/**
	 * The Z-order of the dialogue widget.
	 * ❔ This determines the order in which the widget is rendered relative to other UI elements.
	 * ❔ A higher Z-order means the widget will be rendered on top of others with lower Z-orders.
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue|Manager", 
		meta=(UIMin=0,ClampMin=0))
	int32 DialogueWidgetZOrder;

	/**
	 * Mountea Dialogue Manager Default State.
	 * ❔ Is used in BeginPlay to set ManagerState.
	 * ❔ Is used as fallback value once Dialogue Ends.
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue|Manager")
	EDialogueManagerState DefaultManagerState;
	
	/**
	* State of the Dialogue Manager.
	* ❗ In order to start Dialogue, this value must not be Disabled.
	* ❔ Can be updated using SetDialogueManagerState function.
	*/
	UPROPERTY(ReplicatedUsing=OnRep_ManagerState, SaveGame, VisibleAnywhere, Category="Mountea|Dialogue|Manager")
	EDialogueManagerState ManagerState;

	UPROPERTY(SaveGame, VisibleAnywhere, Category="Mountea|Dialogue|Manager")
	EDialogueManagerType DialogueManagerType;
	
	/**
	 * An array of dialogue objects. Serves purpose of listeners who receive information about UI events.
	 * Each must implement `IMounteaDialogueWBPInterface` interface.
	 */
	UPROPERTY(VisibleAnywhere, Category="Mountea", AdvancedDisplay, 
		meta=(DisplayThumbnail=false))
	TArray<TObjectPtr<UObject>> DialogueObjects;
	
	/**
	 * Dialogue Widget which has been created.
	 * ❔ Transient, for actual runtime only.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue|Manager", AdvancedDisplay, 
		meta=(DisplayThumbnail=false))
	TObjectPtr<UUserWidget> DialogueWidget = nullptr;

	/**
	 * Dialogue Context which is used to contain temporary data.
	 */
	UPROPERTY(VisibleAnywhere, Category="Mountea|Dialogue|Manager", AdvancedDisplay, 
		meta=(DisplayThumbnail=false))
	TObjectPtr<UMounteaDialogueContext> DialogueContext = nullptr;

	/**
	 * TimerHandle managing Dialogue Row.
	 * Once expires, Dialogue Row is finished.
	 * 
	 * ❔ Expiration is driven by Dialogue data Duration variable
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue|Manager", AdvancedDisplay, 
		meta=(DisplayThumbnail=false))
	FTimerHandle TimerHandle_RowTimer;
	FTimerHandle PendingPredictionHandle;

	UPROPERTY(Transient)
	FString LastDialogueCommand;

	bool bParticipantsStarted = false;
	bool bPredictionEnabled = true;
	EDialogueClientPredictionType PendingPredictionType = EDialogueClientPredictionType::None;
	FGuid PendingPredictionSessionGUID;
	FGuid PendingPredictionNodeGUID;
	int32 PendingPredictionStartContextVersion = 0;
	bool bPredictedCloseEventFired = false;
	FGuid PredictedCloseSessionGUID;
	int32 LastReceivedPayloadVersion = 0;
	int32 LastAppliedPayloadCommandVersion = 0;
	FGuid LastClientSyncSessionGUID;
	FGuid LastReconciledRowGUID;
	int32 LastReconciledRowIndex = INDEX_NONE;
	uint32 LastReconciledOptionsHash = 0;
	FGuid LastPlayedAudioRowGUID;
	int32 LastPlayedAudioRowIndex = INDEX_NONE;
	bool bClientAudioPlaying = false;
	uint8 LastReconciledViewMode = 0;

protected:
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};

namespace MounteaDialogueManagerHelpers
{
	struct FDialogueRowDataInfo
	{
		int32	IncreasedIndex;
		bool	bIsActiveRowValid;
		bool	bDialogueRowDataValid;
		ERowExecutionMode	NextRowExecutionMode;
		ERowExecutionMode	ActiveRowExecutionMode;
	};

	inline FDialogueRowDataInfo GetDialogueRowDataInfo(const UMounteaDialogueContext* DialogueContext);
}
