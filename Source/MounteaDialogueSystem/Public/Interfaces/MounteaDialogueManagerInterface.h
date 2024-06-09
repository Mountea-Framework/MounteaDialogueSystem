// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "UObject/Interface.h"
#include "MounteaDialogueManagerInterface.generated.h"

class IMounteaDialogueParticipantInterface;

/**
 * A helper structure that provides easy way to move Participants around.
 */
USTRUCT(BlueprintType)
struct FDialogueParticipants
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue")
	TObjectPtr<AActor> MainParticipant = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue")
	TArray<TObjectPtr<AActor>> OtherParticipants;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueManagerInterface : public UInterface
{
	GENERATED_BODY()
};

class UMounteaDialogueContext;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueInitialized, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdated, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueUserInterfaceChanged, TSubclassOf<UUserWidget>, DialogueWidgetClass, UUserWidget*, DialogueWidget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeEvent, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueRowEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueFailed, const FString&, ErrorMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueManagerStateChanged, const EDialogueManagerState&, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueVoiceEvent, class USoundBase*, NewDialogueVoice);

/**
 * Mountea Dialogue Manager Interface.
 * 
 * Should attached directly to Player Controller or used for Components that are attached to some Controller.
 * Provides options to start and stop dialogue as well as ability to select dialogue options.
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	/**
	 * Notifies the Dialogue  that a node has been selected.
	 *
	 * @param NodeGUID The GUID of the selected node.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="select, chosen, option"))
	void CallDialogueNodeSelected(const FGuid& NodeGUID);
	
	/**
	 * Starts the Dialogue if possible.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="show, widget"))
	void StartDialogue();
	virtual void StartDialogue_Implementation() = 0;
	/**
	 * Closes the Dialogue if is active.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="show, widget"))
	void CloseDialogue();
	virtual void CloseDialogue_Implementation() = 0;

	/**
	 * Tries to Invoke Dialogue UI.
	 * This function servers a purpose to try showing Dialogue UI to player.
	 * ❔ If this function fails, Message will be populated with error message explaining what went wrong.
	 * 
	 * @param Message InMessage to be populated with error message explaining why returns false
	 * @return true if UI can be added to screen, false if cannot
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="show, widget"))
	bool InvokeDialogueUI(FString& Message);
	virtual bool InvokeDialogueUI_Implementation(FString& Message) = 0;

	/**
	 * Tries to Update Dialogue UI.
	 * This function servers a purpose to try update Dialogue UI to player using given command.
	 * ❔ If this function fails, Message will be populated with error message explaining what went wrong.
	 * 
	 * @param Message				InMessage to be populated with error message explaining why returns false
	 * @param Command				Command to be processed.
	 * @return								true if UI can be update, false if cannot
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="show, widget"))
	bool UpdateDialogueUI(FString& Message, const FString& Command);
	virtual bool UpdateDialogueUI_Implementation(FString& Message, const FString& Command) = 0;

	/**
	 * Tries to Close Dialogue UI.
	 * This function servers a purpose to try tear down Dialogue UI from player.
	 * 
	 * @return true if UI can be removed from screen, false if cannot
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="close, exit, stop, widget"))
	bool CloseDialogueUI();
	virtual bool CloseDialogueUI_Implementation() = 0;
	
	/**
	 * Gets the widget class used to display Dialogue UI.
	 * 
	 * @return The widget class used to display Dialogue UI.
	 */
	virtual TSubclassOf<UUserWidget> GetDialogueWidgetClass() const = 0;
	
	/**
	 * Sets the widget class for the Dialogue UI.
	 * ❗ This is a pure virtual function that must be implemented in derived classes.
	 *
	 * @param NewWidgetClass	The new widget class to set.
	 */
	virtual void SetDialogueWidgetClass(TSubclassOf<UUserWidget> NewWidgetClass) = 0;
	
	/**
	 * Sets Dialogue UI pointer.
	 * 
	 * ❔ Using null value resets saved value
	 * @param DialogueUIPtr	UserWidget pointer to be saved as Dialogue UI
	 */
	virtual void SetDialogueUIPtr(UUserWidget* DialogueUIPtr) = 0;

	/**
	 * Starts Dialogue Row execution.
	 * ❔ Dialogue Data contain Dialogue Data Rows, which are individual dialogue lines, which can be skipped.
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 */
	virtual void StartExecuteDialogueRow() = 0;
	/**
	 * Function responsible for cleanup once Dialogue Row is finished.
	 * ❔ Dialogue Data contain Dialogue Data Rows, which are individual dialogue lines, which can be skipped.
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 */
	virtual void FinishedExecuteDialogueRow() = 0;

	/**
	 * Retrieves the current dialogue context associated with this dialogue instance.
	 *
	 * @return The dialogue context object for this instance.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	UMounteaDialogueContext* GetDialogueContextEvent() const;
	UMounteaDialogueContext* GetDialogueContextEvent_Implementation() const
	{
		return GetDialogueContext();
	}
	
	/**
	 * Returns the widget used to display the current dialogue.
	 *
	 * @return The widget used to display the current dialogue.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="UI, Widget"))
	UUserWidget* GetDialogueWidget() const;
	virtual UUserWidget* GetDialogueWidget_Implementation() const = 0;

	/**
	 * Returns the owning actor for this Dialogue Manager Component.
	 *
	 * @return The owning actor for this Dialogue Manager Component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	AActor* GetOwningActor() const;
	virtual AActor* GetOwningActor_Implementation() const
	{
		return nullptr;
	};

	/**
	 * Prepares the node for execution.
	 * Asks Active Node to 'PreProcessNode' and then to 'ProcessNode'.
	 * In this preparation stage, Nodes are asked to process all Decorators.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	void PrepareNode();
	virtual void PrepareNode_Implementation() {};
	
	/**
	 * Retrieves current Dialogue Context.
	 * 
	 * ❗ Could be null
	 * @return DialogueContext	Dialogue Context is transient data holder for current dialogue instance.
	 */
	virtual UMounteaDialogueContext* GetDialogueContext() const = 0;
	/**
	 * Sets new Dialogue Context.
	 * 
	 * ❔ Null value clears saved data
	 * @param NewContext	Dialogue Context to be set as Dialogue Context
	 */
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) = 0;

	/**
	 * Interface call.
	 * Retrieves current Dialogue Manager State.
	 * State defines whether Manager can start/close dialogue or not.
	 * 
	 * @return ManagerState	Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	EDialogueManagerState GetState() const;
	EDialogueManagerState GetState_Implementation() const
	{ return GetDialogueManagerState(); };
	/**
	 * Retrieves current Dialogue Manager State.
	 * State defines whether Manager can start/close dialogue or not.
	 * 
	 * @return ManagerState	Manager state value
	 */
	virtual EDialogueManagerState GetDialogueManagerState() const = 0;
	/**
	 * Sets new Dialogue Manager State.
	 * 
	 * @param NewState	Manager State to be set as Manager State
	 */
	virtual void SetDialogueManagerState(const EDialogueManagerState NewState) = 0;
	/**
	 * Retrieves current Default Dialogue Manager State.
	 * Default Dialogue Manager State sets Dialogue Manager state upon BeginPlay and is used as fallback once Dialogue ends.
	 * 
	 * @return ManagerState	Default Manager state value
	 */
	virtual EDialogueManagerState GetDefaultDialogueManagerState() const = 0;
	/**
	 * Sets new Default Dialogue Manager State.
	 * 
	 * @param NewState	Manager State to be set as Default Manager State
	 */
	virtual void SetDefaultDialogueManagerState(const EDialogueManagerState NewState) = 0;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue", meta=(Keywords="UI, Widget"))
	void InitializeDialogue(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants);
	virtual void InitializeDialogue_Implementation(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants) = 0;
	
	virtual FDialogueInitialized& GetDialogueInitializedEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueStartedEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueClosedEventHandle() = 0;
	
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() = 0;
	virtual FDialogueUserInterfaceChanged& GetDialogueUserInterfaceChangedEventHandle() = 0;

	virtual FDialogueNodeEvent& GetDialogueNodeSelectedEventHandle() = 0;

	virtual FDialogueNodeEvent& GetDialogueNodeStartedEventHandle() = 0;
	virtual FDialogueNodeEvent& GetDialogueNodeFinishedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowStartedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowFinishedEventHandle() = 0;

	virtual FDialogueFailed& GetDialogueFailedEventHandle() = 0;

	virtual FDialogueManagerStateChanged& GetDialogueManagerStateChangedEventHandle() = 0;

	virtual FDialogueVoiceEvent& GetDialogueVoiceStartRequestEventHandle() = 0;
	virtual FDialogueVoiceEvent& GetDialogueVoiceSkipRequestEventHandle() = 0;

	virtual FTimerHandle& GetDialogueRowTimerHandle() = 0;
};
