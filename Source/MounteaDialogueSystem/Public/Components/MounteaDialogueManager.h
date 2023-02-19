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
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueInitializedEvent(UMounteaDialogueContext* Context);

	UFUNCTION()
	virtual void OnDialogueInitializedEvent_Internal(UMounteaDialogueContext* Context);
	
	/**
	 * Event called when Dialogue Context is updated.
	 * ❗Could be Null ❗
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueContextUpdatedEvent(UMounteaDialogueContext* Context);
	UFUNCTION()
	virtual void OnDialogueContextUpdatedEvent_Internal(UMounteaDialogueContext* NewContext);

#pragma endregion

#pragma region EventVariables
protected:
	/**
	 * Even called when Dialogue is Initialized.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueInitialized OnDialogueInitialized;
	
	/**
	 * Event called when Dialogue Context is updated.
	 * ❗Could be Null ❗
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueContextUpdated OnDialogueContextUpdated;

#pragma endregion

#pragma region InterfaceImplementations

protected:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueContext* GetDialogueContext() const override
	{ return DialogueContext; };
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) override;

	virtual FDialogueInitialized& GetDialogueInitializedEventHandle() override
	{ return OnDialogueInitialized; };
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() override
	{ return OnDialogueContextUpdated; };

#pragma endregion 

protected:

	/**
	 * Dialogue Context which is used to contain temporary data.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Mountea|Dialogue")
	UMounteaDialogueContext* DialogueContext = nullptr;
	
};
