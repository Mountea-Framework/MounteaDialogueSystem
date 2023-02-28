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
 * ❔Allows any Actor to be Dialogue Manager
 * ❔Implements 'IMounteaDialogueManagerInterface'.
 * ❗If attached to non-Controller class, then it will show Dialogue UI to first found Player Controller
 */
UCLASS(ClassGroup=(Mountea), Blueprintable,  meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManager : public UActorComponent, public IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueManager();

protected:
	
	virtual void BeginPlay() override;

#pragma region EventFunctions
	
protected:
	virtual void CallDialogueNodeSelected_Implementation(const FGuid& NodeGUID) override;

	/**
	 * Even called when Dialogue is Initialized.
	 * ❗In order to use native logic, call Parent node❗
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Initialized, Start"))
	void OnDialogueInitializedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	virtual void OnDialogueInitializedEvent_Internal(UMounteaDialogueContext* Context);
	
	/**
	 * Event called when Dialogue Context is updated.
	 * ❗Could be Null ❗
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Update, Context"))
	void OnDialogueContextUpdatedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	virtual void OnDialogueContextUpdatedEvent_Internal(UMounteaDialogueContext* NewContext);

	/**
	 * Event called when Dialogue Widget Class or Widget have changed.
	 *❗Dialogue Widget Could be Null ❗
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


#pragma endregion

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
	 * ❗Could be Null ❗
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueContextUpdated OnDialogueContextUpdated;
	/**
	 * Event called when Dialogue Widget Class or Widget have changed.
	 *❗Dialogue Widget Could be Null ❗
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
	 * Event called if Dialogue fails to execute.
	 * Provides Error Message with explanation.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueFailed OnDialogueFailed;

	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueManagerStateChanged OnDialogueManagerStateChanged;

#pragma endregion

#pragma region InterfaceImplementations

protected:

	virtual bool EvaluateNodeDecorators() override;
	virtual void ExecuteNodeDecorators() override;

	virtual void StartDialogue() override;
	virtual void CloseDialogue() override;
	virtual void ProcessNode() override;
	virtual void ProcessNode_Complete() override;
	virtual void ProcessNode_Dialogue() override;

	virtual bool InvokeDialogueUI(FString& Message) override;
	
	/**
	 * Returns Dialogue Widget Class if any exists already.
	 * ❗If none specified per Manager will return Class from Project Settings❗
	 * ❗Could return null❗
	 */
	virtual TSubclassOf<UUserWidget> GetDialogueWidgetClass() const override;
	virtual void SetDialogueWidgetClass(TSubclassOf<UUserWidget> NewWidgetClass) override;

	/**
	 * Returns Dialogue Widget Pointer if any exists already.
	 * ❗Could return null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="UI, Widget"))
	virtual UUserWidget* GetDialogueUIPtr() const override
	{ return DialogueWidgetPtr; };
	virtual void SetDialogueUIPtr(UUserWidget* NewDialogueWidgetPtr) override;

	UFUNCTION() virtual void StartExecuteDialogueRow() override;
	UFUNCTION() virtual void FinishedExecuteDialogueRow() override;

	/**
	 * Returns Dialogue Context if any exists.
	 * ❗Could return null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="Context, Get"))
	virtual UMounteaDialogueContext* GetDialogueContext() const override
	{ return DialogueContext; };
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="Context, Get"))
	virtual EDialogueManagerState GetDialogueManagerState() const override
	{ return  ManagerState; };
	virtual void SetDialogueManagerState(const EDialogueManagerState NewState) override;

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
	virtual FDialogueNodeEvent& GetDialogueNodeSelected() override
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

#pragma endregion 

#pragma region Functions

	

#pragma endregion 
protected:

	/**
	 * Manager based Dialogue Widget Class.
	 * ❔Could be left empty if Project Settings are setup properly❔
	 * ❗Must implement MounteaDialogueWBPInterface❗
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface"))
	TSubclassOf<UUserWidget> DialogueWidgetClass = nullptr;

	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue")
	EDialogueManagerState ManagerState;
	
	/**
	 * Dialogue Widget which has been created.
	 * Transient, for actual runtime only.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue")
	UUserWidget* DialogueWidgetPtr = nullptr;

	/**
	 * Dialogue Context which is used to contain temporary data.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue")
	UMounteaDialogueContext* DialogueContext = nullptr;

	FTimerHandle TimerHandle_RowTimer;
};
