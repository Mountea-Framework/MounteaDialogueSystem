// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManager.generated.h"


/**
 *  Mountea Dialogue Manager Component
 * 
 * Should be attached to Player State in order to be replication ready.
 * ❔ Allows any Actor to be Dialogue Manager
 * ❔ Implements 'IMounteaDialogueManagerInterface'.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable,  AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"), meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManager : public UActorComponent, public IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueManager();

protected:
	
	virtual void BeginPlay() override;

public:

	virtual FDialogueStartRequestedResult& GetDialogueStartRequestedResultEventHandle() override
	{ return OnDialogueStartRequested; };
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

protected:

	void ProcessStateUpdated();
	UFUNCTION()
	void RequestBroadcastContext(UMounteaDialogueContext* Context);
	UFUNCTION(Server, Reliable)
	void RequestBroadcastContext_Server(const FMounteaDialogueContextReplicatedStruct& Context);
	UFUNCTION(NetMulticast, Unreliable)
	void RequestBroadcastContext_Multicast(const FMounteaDialogueContextReplicatedStruct& Context);
	UFUNCTION()
	void DialogueFailed(const FString& ErrorMessage);

	void StartParticipants() const;
	
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
	virtual void DialogueRowProcessed_Implementation() override;
	virtual void SkipDialogueRow_Implementation() override;
	
	virtual void UpdateWorldDialogueUI_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command) override;
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

private:

	UFUNCTION(Server, Reliable)
	void SetManagerState_Server(const EDialogueManagerState NewState);
	UFUNCTION(Server, Unreliable)
	void SetDefaultManagerState_Server(const EDialogueManagerState NewState);
	UFUNCTION(Server, Reliable)
	void SetDialogueContext_Server(UMounteaDialogueContext* NewContext);
	UFUNCTION(Server, Reliable)
	void UpdateDialogueContext_Server(UMounteaDialogueContext* NewContext);
	UFUNCTION(Server, Reliable)
	void RequestStartDialogue_Server(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	UFUNCTION(Server, Reliable)
	void CleanupDialogue_Server();
	UFUNCTION(Server, Unreliable)
	void UpdateWorldDialogueUI_Server(const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command);
	UFUNCTION(NetMulticast, Unreliable)
	void UpdateWorldDialogueUI_Multicast(const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command);
	
	UFUNCTION()
	void OnRep_ManagerState();

public:
	bool IsAuthority() const;

protected:
	
	/**
	 * Even called when Dialogue is Initialized.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Manager")
	FDialogueStartRequestedResult OnDialogueStartRequested;
	
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

protected:

	/**
	 * Manager based Dialogue Widget Class.
	 * ❔ Could be left empty if Project Settings are setup properly
	 * ❗ Must implement MounteaDialogueWBPInterface
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue|Manager", DisplayName="Dialogue Widget Class Override", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface"))
	TSubclassOf<UUserWidget> DialogueWidgetClass = nullptr;

	/**
	 * The Z-order of the dialogue widget.
	 * ❔ This determines the order in which the widget is rendered relative to other UI elements.
	 * ❔ A higher Z-order means the widget will be rendered on top of others with lower Z-orders.
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue|Manager", meta=(UIMin=0,ClampMin=0))
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
	
	/**
	 * An array of dialogue objects. Serves purpose of listeners who receive information about UI events.
	 * Each must implement `IMounteaDialogueWBPInterface` interface.
	 */
	UPROPERTY(VisibleAnywhere, Category="Mountea", AdvancedDisplay, meta=(DisplayThumbnail=false))
	TArray<TObjectPtr<UObject>> DialogueObjects;
	
	/**
	 * Dialogue Widget which has been created.
	 * ❔ Transient, for actual runtime only.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue|Manager", AdvancedDisplay, meta=(DisplayThumbnail=false))
	TObjectPtr<UUserWidget> DialogueWidget = nullptr;

	/**
	 * Dialogue Context which is used to contain temporary data.
	 */
	UPROPERTY(VisibleAnywhere, Category="Mountea|Dialogue|Manager", AdvancedDisplay, meta=(DisplayThumbnail=false))
	TObjectPtr<UMounteaDialogueContext> DialogueContext = nullptr;

	/**
	 * TimerHandle managing Dialogue Row.
	 * Once expires, Dialogue Row is finished.
	 * 
	 * ❔ Expiration is driven by Dialogue data Duration variable
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue|Manager", AdvancedDisplay, meta=(DisplayThumbnail=false))
	FTimerHandle TimerHandle_RowTimer;

protected:
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

};

namespace MounteaDialogueManagerHelpers
{
	struct FDialogueRowDataInfo
	{
		int32				IncreasedIndex;
		bool				bIsActiveRowValid;
		bool				bDialogueRowDataValid;
		ERowExecutionMode	NextRowExecutionMode;
		ERowExecutionMode	ActiveRowExecutionMode;
	};

	inline FDialogueRowDataInfo GetDialogueRowDataInfo(const UMounteaDialogueContext* DialogueContext);
}