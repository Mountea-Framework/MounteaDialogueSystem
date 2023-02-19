// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManager.generated.h"

/**
 *  
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
	 * Event called when Dialogue has Started.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue", meta=(Keywords="Update, Context"))
	void OnDialogueStartedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueStartedEvent_Internal(UMounteaDialogueContext* Context);

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
	 * Event called when Dialogue Context is updated.
	 * ❗Could be Null ❗
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueContextUpdated OnDialogueContextUpdated;


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

#pragma endregion

#pragma region InterfaceImplementations

protected:

	virtual void StartDialogue() override;
	virtual void ProcessNode() override;
	virtual void ProcessNode_Complete() override;
	virtual void ProcessNode_Dialogue() override;

	UFUNCTION() virtual void StartExecuteDialogueRow() override;
	UFUNCTION() virtual void FinishedExecuteDialogueRow() override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="Context, Get"))
	virtual UMounteaDialogueContext* GetDialogueContext() const override
	{ return DialogueContext; };
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) override;

	virtual FDialogueInitialized& GetDialogueInitializedEventHandle() override
	{ return OnDialogueInitialized; };
	virtual FDialogueEvent& GetDialogueStartedEventHandle() override
	{ return OnDialogueStarted; };
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() override
	{ return OnDialogueContextUpdated; };
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

#pragma endregion 

#pragma region Functions

	

#pragma endregion 
protected:

	/**
	 * Dialogue Context which is used to contain temporary data.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue", AdvancedDisplay=true)
	UMounteaDialogueContext* DialogueContext = nullptr;

	FTimerHandle TimerHandle_RowTimer;
};
