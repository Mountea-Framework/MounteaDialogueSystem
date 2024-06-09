// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManager.generated.h"


/**
 *  Mountea Dialogue Manager Component
 * 
 * Should be attached to Player Controller or any other Controller which should be able to trigger dialogues.
 * ❔ Allows any Actor to be Dialogue Manager
 * ❔ Implements 'IMounteaDialogueManagerInterface'.
 * ❗ If attached to non-Controller class, then it will show Dialogue UI to first found Player Controller
 */
UCLASS(ClassGroup=(Mountea), Blueprintable,  AutoExpandCategories=("Mountea", "Dialogue", "Mountea|Dialogue"), meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManager : public UActorComponent, public IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueManager();

protected:
	
	virtual void BeginPlay() override;

public:
	
#pragma region EventFunctions
	
public:
	
	virtual void CallDialogueNodeSelected_Implementation(const FGuid& NodeGUID) override;

	/**
	 * Even called when Dialogue is Initialized.
	 * ❗ In order to use native logic, call Parent node
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Initialized, Start"))
	void OnDialogueInitializedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	virtual void OnDialogueInitializedEvent_Internal(UMounteaDialogueContext* Context);
	
	/**
	 * Event called when Dialogue Context is updated.
	 * ❗ Could be Null
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Update, Context"))
	void OnDialogueContextUpdatedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	virtual void OnDialogueContextUpdatedEvent_Internal(UMounteaDialogueContext* NewContext);

	/**
	 * Event called when Dialogue Widget Class or Widget have changed.
	 * ❗ Dialogue Widget Could be Null
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Update, Context"))
	void OnDialogueUserInterfaceChangedEvent(TSubclassOf<UUserWidget> DialogueUIClass, UUserWidget* DialogueUIWidget);
	UFUNCTION()
	void OnDialogueUserInterfaceChangedEvent_Internal(TSubclassOf<UUserWidget> DialogueUIClass, UUserWidget* DialogueUIWidget);

	/**
	 * Event called when Dialogue has Started.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Update, Context"))
	void OnDialogueStartedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueStartedEvent_Internal(UMounteaDialogueContext* Context);

	/**
	 * Event called when Dialogue has Closed.
	 * Could be either by manual request or automatic, as there are no nodes to follow.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Close, Context"))
	void OnDialogueClosedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueClosedEvent_Internal(UMounteaDialogueContext* Context);

	/**
	 * Event called when new Node is selected.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Start, Begin"))
	void OnDialogueNodeSelectedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueNodeSelectedEvent_Internal(UMounteaDialogueContext* Context);
	/**
	 * Event called when Dialogue Node has Started.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Start, Begin"))
	void OnDialogueNodeStartedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueNodeStartedEvent_Internal(UMounteaDialogueContext* Context);
	/**
	 * Event called when Dialogue Node has Finished.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Finish, End, Complete"))
	void OnDialogueNodeFinishedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueNodeFinishedEvent_Internal(UMounteaDialogueContext* Context);

	UFUNCTION() void OnDialogueRowStartedEvent_Internal(UMounteaDialogueContext* Context);
	UFUNCTION() void OnDialogueRowFinishedEvent_Internal(UMounteaDialogueContext* Context);
	
	/**
	 * Event called when a new voice line is requested to play in dialogue.
	 * ❗ VoiceToStart may be null.
	 * 
	 * @param VoiceToStart The voice line to start playing.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Finish, End, Complete"))
	void OnDialogueVoiceStartRequestEvent(USoundBase* VoiceToStart);
	UFUNCTION()
	void OnDialogueVoiceStartRequestEvent_Internal(USoundBase* VoiceToStart);
	/**
	 * Event called when the user requests to skip a dialogue voice line.
	 * ❗ VoiceToSkip could be null.
	 * 
	 * @param VoiceToSkip - The voice line to be skipped.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Finish, End, Complete"))
	void OnDialogueVoiceSkipRequestEvent(USoundBase* VoiceToSkip);
	UFUNCTION()
	void OnDialogueVoiceSkipRequestEvent_Internal(USoundBase* VoiceToSkip);

#pragma endregion

protected:
	
#pragma region EventVariables
	
protected:
	/**
	 * Even called when Dialogue is Initialized.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueInitialized OnDialogueInitialized;
	/**
	 * Event called when Dialogue has started.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueEvent OnDialogueStarted;
	/**
	 * Event called when Dialogue has been closed.
	 * Could be either manual or automatic.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueEvent OnDialogueClosed;
	
	
	/**
	 * Event called when Dialogue Context is updated.
	 * ❗ Could be Null
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueContextUpdated OnDialogueContextUpdated;
	/**
	 * Event called when Dialogue Widget Class or Widget have changed.
	 *❗ Dialogue Widget Could be Null
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueUserInterfaceChanged OnDialogueUserInterfaceChanged;
	

	/**
	 * Event called when new Dialogue Node has been selected.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueNodeEvent OnDialogueNodeSelected;

	/**
	 * Event called when Dialogue Node has started.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueNodeEvent OnDialogueNodeStarted;
	/**
	 * Event called when Dialogue Node has finished.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueNodeEvent OnDialogueNodeFinished;
	
	/**
	 * Event called when Dialogue Row has started.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueRowEvent OnDialogueRowStarted;
	/**
	 * Event called when Dialogue Row has finished.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueRowEvent OnDialogueRowFinished;
	/**
	 * Event called once Dialogue Row Data update has been requested.
	 * Is never called from Code and is bound to `FinishedExecuteDialogueRow` function.
	 * Should be used carefully. Suggested usage is with Dialogue Row Data which are using `RowDurationMode::Manual`.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueRowEvent OnNextDialogueRowDataRequested;

	/**
	 * Event called if Dialogue fails to execute.
	 * Provides Error Message with explanation.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueFailed OnDialogueFailed;

	/**
	 * Event called when Dialogue State changes.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueManagerStateChanged OnDialogueManagerStateChanged;

	/**
	 * Event called when Dialogue Voice is requested to Start.
	 * Provides info what Dialogue Voice is requested to Start.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueVoiceEvent OnDialogueVoiceStartRequest;
	/**
	 * Event called when Dialogue Voice is requested to Skip.
	 * Provides info what Dialogue Voice is requested to Skip.
	 */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueVoiceEvent OnDialogueVoiceSkipRequest;

#pragma endregion

#pragma region InterfaceImplementations

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual AActor* GetOwningActor_Implementation() const override;

	/**
	 * Returns Dialogue Widget Class if any exists already.
	 * ❗ If none specified per Manager will return Class from Project Settings
	 * ❗ Could return null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="UI, Widget"))
	virtual TSubclassOf<UUserWidget> GetDialogueWidgetClass() const override;

	/**
	 * Returns Dialogue Context if any exists.
	 * ❗ Could return null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="Context, Get"))
	virtual UMounteaDialogueContext* GetDialogueContext() const override
	{ return DialogueContext; };

	/**
	 * Returns the current state of the Dialogue Manager.
	 * @return The current state of the Dialogue Manager.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="Context, Get"))
	virtual EDialogueManagerState GetDialogueManagerState() const override
	{ return  ManagerState; };

	/**
	 * Returns the default state of the Dialogue Manager.
	 * @return The default state of the Dialogue Manager.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="Context, Get"))
	virtual EDialogueManagerState GetDefaultDialogueManagerState() const override
	{ return DefaultManagerState; };
	
	virtual void PrepareNode_Implementation() override;
	
public:

	virtual void InitializeDialogue_Implementation(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants) override;

	virtual void StartDialogue_Implementation() override;
	virtual void CloseDialogue_Implementation() override;
	virtual void ProcessNode();
	
	virtual bool InvokeDialogueUI_Implementation(FString& Message) override;
	virtual bool UpdateDialogueUI_Implementation(FString& Message, const FString& Command) override;
	virtual bool CloseDialogueUI_Implementation() override;
	
	virtual void SetDialogueWidgetClass(TSubclassOf<UUserWidget> NewWidgetClass) override;
	virtual void SetDialogueUIPtr(UUserWidget* NewDialogueWidgetPtr) override;
	virtual UUserWidget* GetDialogueWidget_Implementation() const override;

	UFUNCTION() virtual void StartExecuteDialogueRow() override;
	UFUNCTION() virtual void FinishedExecuteDialogueRow() override;
	UFUNCTION() void NextDialogueRowDataRequested(UMounteaDialogueContext* Context);
	
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) override;
	
	virtual void SetDialogueManagerState(const EDialogueManagerState NewState) override;
	virtual void SetDefaultDialogueManagerState(const EDialogueManagerState NewState) override;
	
	virtual FDialogueInitialized& GetDialogueInitializedEventHandle() override
	{ return OnDialogueInitialized; };
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
	virtual FDialogueVoiceEvent& GetDialogueVoiceSkipRequestEventHandle() override
	{ return OnDialogueVoiceSkipRequest; };
	virtual FDialogueVoiceEvent& GetDialogueVoiceStartRequestEventHandle() override
	{ return OnDialogueVoiceStartRequest; };
	virtual FTimerHandle& GetDialogueRowTimerHandle() override
	{ return TimerHandle_RowTimer; }; 

#pragma endregion 

#pragma region Variables
protected:

	/**
	 * Manager based Dialogue Widget Class.
	 * ❔ Could be left empty if Project Settings are setup properly
	 * ❗ Must implement MounteaDialogueWBPInterface
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface"))
	TSubclassOf<UUserWidget> DialogueWidgetClass = nullptr;

	/**
	 * Mountea Dialogue Manager Default State.
	 * ❔ Is used in BeginPlay to set ManagerState.
	 * ❔ Is used as fallback value once Dialogue Ends.
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue")
	EDialogueManagerState DefaultManagerState;
	
	/**
	* State of the Dialogue Manager.
	* ❗ In order to start Dialogue, this value must not be Disabled.
	* ❔ Can be updated using SetDialogueManagerState function.
	*/
	UPROPERTY(ReplicatedUsing=OnResp_ManagerState, SaveGame, VisibleAnywhere, Category="Mountea|Dialogue", ReplicatedUsing="OnRep_ManagerState")
	EDialogueManagerState ManagerState;
	
	/**
	 * Dialogue Widget which has been created.
	 * ❔ Transient, for actual runtime only.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea", AdvancedDisplay, meta=(DisplayThumbnail=false))
	TObjectPtr<UUserWidget> DialogueWidgetPtr = nullptr;

	/**
	 * Dialogue Context which is used to contain temporary data.
	 */
	UPROPERTY(/*ReplicatedUsing=OnRep_DialogueContext,*/ VisibleAnywhere, Category="Mountea", AdvancedDisplay, meta=(DisplayThumbnail=false))
	TObjectPtr<UMounteaDialogueContext> DialogueContext = nullptr;

	/** replicated struct*/
	UPROPERTY()
	FMounteaDialogueContextReplicatedStruct ReplicatedDialogueContext;

	/**
	 * TimerHandle managing Dialogue Row.
	 * Once expires, Dialogue Row is finished.
	 * 
	 * ❔ Expiration is driven by Dialogue data Duration variable
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea", AdvancedDisplay, meta=(DisplayThumbnail=false))
	FTimerHandle TimerHandle_RowTimer;

	 /**
	 * Is saved once Dialogue starts.
	 * Once Dialogue ends, cached value is set back again.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea", AdvancedDisplay)
	uint8 bWasCursorVisible : 1;

	/**
	 * 
	 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Mountea", AdvancedDisplay)
	int DialogueContextReplicationKey = 0;

#pragma endregion

#pragma region Functions

protected:

	UFUNCTION(Server, Reliable)
	void SetDialogueManagerState_Server(const EDialogueManagerState NewState);
	UFUNCTION(Server, Reliable)
	void SetDialogueDefaultManagerState_Server(const EDialogueManagerState NewState);
	UFUNCTION(Server, Reliable)
	void SetDialogueContext_Server(UMounteaDialogueContext* NewContext);
	UFUNCTION(Server, Reliable)
	void SetDialogueWidgetClass_Server(TSubclassOf<UUserWidget> NewDialogueWidgetClass);
	UFUNCTION(Server, Reliable)
	void InitializeDialogue_Server(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants);

	UFUNCTION(Server, Reliable)
	void CallDialogueNodeSelected_Server(const FGuid& NodeGuid);
	
	UFUNCTION(Client, Reliable)
	void UpdateDialogueContext_Client(const FMounteaDialogueContextReplicatedStruct& NewDialogueContext);

	UFUNCTION(Server, Reliable)
	void StartDialogue_Server();
	UFUNCTION(Server, Reliable)
	void CloseDialogue_Server();
	UFUNCTION(Client, Reliable)
	void InvokeDialogueUI_Client();
	UFUNCTION(Client, Reliable)
	void UpdateDialogueUI_Client(const FString& Command);
	UFUNCTION(Client, Reliable)
	void CloseDialogueUI_Client();

	UFUNCTION(Server, Reliable)
	void FinishedExecuteDialogueRow_Server();
	UFUNCTION(Client, Reliable)
	void StartExecuteDialogueRow_Client();
	UFUNCTION(Client, Unreliable)
	void RequestVoiceStart_Client(USoundBase* SoundBase);
	UFUNCTION(Client, Unreliable)
	void RequestVoiceStop_Client(USoundBase* SoundBase);

	UFUNCTION(Server, Reliable)
	void PostUIInitialized();

	UFUNCTION()
	void OnRep_ManagerState();
	/*UFUNCTION()
	void OnRep_DialogueContext();*/

	void NetPushDialogueContext();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	
#pragma endregion
};