// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueManagerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType, Blueprintable)
class UMounteaDialogueManagerInterface : public UInterface
{
	GENERATED_BODY()
};

class UMounteaDialogueContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueInitialized, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdated, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueUserInterfaceChanged, TSubclassOf<UUserWidget>, DialogueWidgetClass, UUserWidget*, DiaogueWidget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeEvent, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueRowEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueFailed, const FString&, ErrorMessage);

/**
 * Mountea Dialgue Manager Interface.
 * 
 * Should attached directly to Player Controller or used for Components that are attached to some Controller.
 * Provides options to start and stop dialogue as well as ability to select dialogue options.
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	/**
	 * 
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="select, chosen, option"))
	void CallDialogueNodeSelected(const FGuid& NodeGUID);

	virtual void StartDialogue() = 0;
	virtual void CloseDialogue() = 0;
	virtual void ProcessNode() = 0;
	virtual void ProcessNode_Complete() = 0;
	virtual void ProcessNode_Dialogue() = 0;

	virtual bool InvokeDialogueUI(FString& Message) = 0;
	virtual TSubclassOf<UUserWidget> GetDialogueWidgetClass() const = 0;
	virtual void SetDialogueWidgetClass(TSubclassOf<UUserWidget> NewWidgetClass) = 0;
	virtual UUserWidget* GetDialogueUIPtr() const = 0;
	virtual void SetDialogueUIPtr(UUserWidget* DialogueUIPtr) = 0;

	virtual void StartExecuteDialogueRow() = 0;
	virtual void FinishedExecuteDialogueRow() = 0;

	/**
	 * Returns Dialogue Context if any exists.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	UMounteaDialogueContext* GetDialogueContextEvent() const;
	UMounteaDialogueContext* GetDialogueContextEvent_Implementation() const
	{
		return GetDialogueContext();
	}
	
	virtual UMounteaDialogueContext* GetDialogueContext() const = 0;
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) = 0;

	virtual FDialogueInitialized& GetDialogueInitializedEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueStartedEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueClosedEventHandle() = 0;
	
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() = 0;
	virtual FDialogueUserInterfaceChanged& GetDialogueUserInterfaceChangedEventHandle() = 0;

	virtual FDialogueNodeEvent& GetDialogueNodeSelected() = 0;

	virtual FDialogueNodeEvent& GetDialogueNodeStartedEventHandle() = 0;
	virtual FDialogueNodeEvent& GetDialogueNodeFinishedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowStartedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowFinishedEventHandle() = 0;

	virtual FDialogueFailed& GetDialogueFailedEventHandle() = 0;
};
