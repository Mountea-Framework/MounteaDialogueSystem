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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Manager")
	TObjectPtr<AActor> MainParticipant = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Manager")
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueWidgetCommand, const TScriptInterface<IMounteaDialogueManagerInterface>&, DialogueManager, const FString&, WidgetCommand);

/**
 * Mountea Dialogue Manager Interface.
 * 
 * Should be attached directly to Player State.
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="select, chosen, option"), meta=(CustomTag="MounteaK2Setter"))
	void CallDialogueNodeSelected(const FGuid& NodeGUID);
	
	/**
	 * Starts the Dialogue if possible.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="start"), meta=(CustomTag="MounteaK2Setter"))
	void StartDialogue();
	virtual void StartDialogue_Implementation() = 0;
	
	/**
	 * Closes the Dialogue if is active.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="exit, close"), meta=(CustomTag="MounteaK2Setter"))
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="show, widget"), meta=(CustomTag="MounteaK2Setter"))
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="show, widget"), meta=(CustomTag="MounteaK2Setter"))
	bool UpdateDialogueUI(FString& Message, const FString& Command);
	virtual bool UpdateDialogueUI_Implementation(FString& Message, const FString& Command) = 0;

	/**
	 * Tries to Close Dialogue UI.
	 * This function servers a purpose to try tear down Dialogue UI from player.
	 * 
	 * @return true if UI can be removed from screen, false if cannot
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="close, exit, stop, widget"), meta=(CustomTag="MounteaK2Setter"))
	bool CloseDialogueUI();
	virtual bool CloseDialogueUI_Implementation() = 0;

	/**
	 * 
	 * @param Command	
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="UI, widget, process"), meta=(CustomTag="MounteaK2Setter"))
	void ExecuteWidgetCommand(const FString& Command);
	virtual void ExecuteWidgetCommand_Implementation(const FString& Command) = 0;
	
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
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void SetDialogueWidget(UUserWidget* DialogueUIPtr);
	virtual void SetDialogueWidget_Implementation(UUserWidget* DialogueUIPtr) = 0;

	/**
	 * Starts Dialogue Row execution.
	 * ❔ Dialogue Data contain Dialogue Data Rows, which are individual dialogue lines, which can be skipped.
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void StartExecuteDialogueRow();
	virtual void StartExecuteDialogueRow_Implementation() = 0;
	
	/**
	 * Function responsible for cleanup once Dialogue Row is finished.
	 * ❔ Dialogue Data contain Dialogue Data Rows, which are individual dialogue lines, which can be skipped.
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void FinishedExecuteDialogueRow();
	virtual void FinishedExecuteDialogueRow_Implementation() = 0;

	/**
	 * Requests next dialogue row.
	 * Contains validation that current row must be 'ExecutionMode::AwaitInput'.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void TriggerNextDialogueRow();
	virtual void TriggerNextDialogueRow_Implementation() = 0;

	/**
	 * Retrieves the current dialogue context associated with this dialogue instance.
	 *
	 * @return The dialogue context object for this instance.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", DisplayName="GetDialogueContext", meta=(CustomTag="MounteaK2Getter"))
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="UI, Widget"), meta=(CustomTag="MounteaK2Getter"))
	UUserWidget* GetDialogueWidget() const;
	virtual UUserWidget* GetDialogueWidget_Implementation() const = 0;

	/**
	 * Returns the owning actor for this Dialogue Manager Component.
	 *
	 * @return The owning actor for this Dialogue Manager Component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
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
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void PrepareNode();
	virtual void PrepareNode_Implementation() {};

	/**
	 * Calls to the Node to Process it.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void ProcessNode();
	virtual void ProcessNode_Implementation() = 0;
	
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
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="state, status"), meta=(CustomTag="MounteaK2Setter"))
	void SetDialogueManagerState(const EDialogueManagerState NewState);
	virtual void SetDialogueManagerState_Implementation(const EDialogueManagerState NewState) = 0;

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

	/**
	 * Initializes the dialogue with the provided player state and participants. Provides the Manager and World to Nodes and Dialogue Graph.
	 *
	 * @param OwningPlayerState		The player state that owns this dialogue instance.
	 * @param Participants					A structure containing all the participants involved in the dialogue.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="start,begin,initialize,initiate"), meta=(CustomTag="MounteaK2Setter"))
	void InitializeDialogue(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants);
	virtual void InitializeDialogue_Implementation(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants) = 0;

	/**
	 * Skips the current dialogue row.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="UI, Widget"), meta=(CustomTag="MounteaK2Setter"))
	void SkipDialogueRow();
	virtual void SkipDialogueRow_Implementation() = 0;

	/**
	 * Adds a single dialogue UI object to the manager.
	 * @param NewDialogueObject The object to add.
	 * @return True if the object was successfully added, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	bool AddDialogueUIObject(UObject* NewDialogueObject);
	virtual bool AddDialogueUIObject_Implementation(UObject* NewDialogueObject) = 0;

	/**
	 * Adds an array of dialogue UI objects to the manager.
	 * @param NewDialogueObjects Array of objects to add.
	 * @return Number of objects successfully added.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	bool AddDialogueUIObjects(const TArray<UObject* >& NewDialogueObjects);
	virtual bool AddDialogueUIObjects_Implementation(const TArray<UObject* >& NewDialogueObjects) = 0;

	/**
	 * Removes a single dialogue UI object from the manager.
	 * @param DialogueObjectToRemove The object to remove.
	 * @return True if the object was successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	bool RemoveDialogueUIObject(UObject* DialogueObjectToRemove);
	virtual bool RemoveDialogueUIObject_Implementation(UObject* DialogueObjectToRemove) = 0;

	/**
	 * Removes multiple dialogue UI objects from the manager.
	 * @param DialogueObjectsToRemove Array of objects to remove.
	 * @return Number of objects successfully removed.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	bool RemoveDialogueUIObjects(const TArray<UObject* >& DialogueObjectsToRemove);
	virtual bool RemoveDialogueUIObjects_Implementation(const TArray<UObject* >& DialogueObjectsToRemove) = 0;

	/**
	 * Sets the entire array of dialogue UI objects, replacing any existing objects.
	 * @param NewDialogueObjects The new array of dialogue UI objects.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void SetDialogueUIObjects(const TArray<UObject* >& NewDialogueObjects);
	virtual void SetDialogueUIObjects_Implementation(const TArray<UObject* >& NewDialogueObjects) = 0;

	/**
	 * Resets the dialogue manager, removing all dialogue UI objects.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void ResetDialogueUIObjects();
	virtual void ResetDialogueUIObjects_Implementation() = 0;

	/**
	 * Retrieves the Z-order of the dialogue widget.
	 * The Z-order determines the rendering order of the widget, with higher values rendering on top of lower values.
	 *
	 * @return The current Z-order value of the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	int32 GetDialogueWidgetZOrder() const;
	virtual int32 GetDialogueWidgetZOrder_Implementation() const = 0;

	/**
	 * Sets a new Z-order for the dialogue widget.
	 * The Z-order determines the rendering order of the widget, with higher values rendering on top of lower values.
	 * ❗ Runtime changes are not allowed!
	 *
	 * @param NewZOrder The new Z-order value to be applied to the dialogue widget.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	void SetDialogueWidgetZOrder(const int32 NewZOrder);
	virtual void SetDialogueWidgetZOrder_Implementation(const int32 NewZOrder) = 0;
	
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

	virtual FDialogueWidgetCommand& GetDialogueWidgetCommandHandle() = 0;

	virtual FTimerHandle& GetDialogueRowTimerHandle() = 0;
};
