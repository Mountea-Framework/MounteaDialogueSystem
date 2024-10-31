// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/MounteaDialogueManagerInterface.h"

#include "MounteaDialogueGraphHelpers.h"
#include "MounteaDialogueManagerStatics.generated.h"

enum class EDialogueManagerState : uint8;

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManagerStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// --- Template functions ------------------------------

	template<typename ReturnType, typename Func, typename... Args>
	static ReturnType ExecuteIfImplements(UObject* Target, const TCHAR* FunctionName, Func Function, Args&&... args)
	{
		if (!IsValid(Target))
		{
			LOG_ERROR(TEXT("[%s] Invalid Target provided!"), FunctionName);
			if constexpr (!std::is_void_v<ReturnType>)
				return ReturnType{};
			else return;
		}

		if (Target->Implements<UMounteaDialogueManagerInterface>())
		{
			if constexpr (std::is_void_v<ReturnType>)
			{
				Function(Target, Forward<Args>(args)...);
				return;
			}
			return Function(Target, Forward<Args>(args)...);
		}

		LOG_ERROR(TEXT("[%s] Target does not implement 'MounteaDialogueManagerInterface'!"), FunctionName);
		if constexpr (!std::is_void_v<ReturnType>)
			return ReturnType{};
		else return;
	}

public:

	// --- Actor functions ------------------------------

	/**
	 * Returns the owning actor for this Dialogue Manager.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return The owning actor for this Dialogue Manager.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static AActor* GetOwningActor(UObject* Target);

	/**
	 * Returns the owning actor for this Dialogue Manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return The owning actor for this Dialogue Manager.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Owning Actor (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static AActor* GetOwningActorV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Returns the Dialogue Manager as Object.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return The Dialogue Manager as Object.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static UObject* GetManagerObject(UObject* Target);

	/**
	 * Returns the Dialogue Manager as Object.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return The Dialogue Manager as Object.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Manager Object (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static UObject* GetManagerObjectV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Retrieves current Dialogue Context.
	 * 
	 * ❗ Could be null
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return DialogueContext	Dialogue Context is transient data holder for current dialogue instance.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Context", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueContext* GetDialogueContext(UObject* Target);

	/**
	 * Retrieves current Dialogue Context.
	 * 
	 * ❗ Could be null
	 * @param Target	Dialogue Manager interface.
	 * @return DialogueContext	Dialogue Context is transient data holder for current dialogue instance.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Context (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueContext* GetDialogueContextV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Retrieves current Dialogue Manager State.
	 * State defines whether Manager can start/close dialogue or not.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return ManagerState	Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Manager State", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetState(UObject* Target);

	/**
	 * Retrieves current Dialogue Manager State.
	 * State defines whether Manager can start/close dialogue or not.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return ManagerState	Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Manager State (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetStateV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Sets new Dialogue Manager State.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NewState	Manager State to be set as Manager State
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static void SetDialogueManagerState(UObject* Target, const EDialogueManagerState NewState);

	/**
	 * Sets new Dialogue Manager State.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewState	Manager State to be set as Manager State
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue Manager State (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static void SetDialogueManagerStateV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const EDialogueManagerState NewState);

	/**
	 * Retrieves current Default Dialogue Manager State.
	 * Default Dialogue Manager State sets Dialogue Manager state upon BeginPlay and is used as fallback once Dialogue ends.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return ManagerState	Default Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetDefaultDialogueManagerState(UObject* Target);

	/**
	 * Retrieves current Default Dialogue Manager State.
	 * Default Dialogue Manager State sets Dialogue Manager state upon BeginPlay and is used as fallback once Dialogue ends.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return ManagerState	Default Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Default Dialogue Manager State (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetDefaultDialogueManagerStateV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Adds a single dialogue UI object to the manager.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NewDialogueObject The object to add.
	 * @return True if the object was successfully added, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static bool AddDialogueUIObject(UObject* Target, UObject* NewDialogueObject);
	
	/**
	 * Adds a single dialogue UI object to the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObject The object to add.
	 * @return True if the object was successfully added, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Add Dialogue UI Object (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static bool AddDialogueUIObjectV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* NewDialogueObject);

	/**
	 * Adds an array of dialogue UI objects to the manager.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NewDialogueObjects Array of objects to add.
	 * @return Number of objects successfully added.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static bool AddDialogueUIObjects(UObject* Target,const TArray<UObject*>& NewDialogueObjects);

	/**
	 * Adds an array of dialogue UI objects to the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObjects Array of objects to add.
	 * @return Number of objects successfully added.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Add Dialogue UI Objects (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static bool AddDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects);

	/**
	 * Removes a single dialogue UI object from the manager.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param DialogueObjectToRemove The object to remove.
	 * @return True if the object was successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static bool RemoveDialogueUIObject(UObject* Target, UObject* DialogueObjectToRemove);

	/**
	 * Removes a single dialogue UI object from the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueObjectToRemove The object to remove.
	 * @return True if the object was successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Remove Dialogue UI Object (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static bool RemoveDialogueUIObjectV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* DialogueObjectToRemove);

	/**
	 * Removes multiple dialogue UI objects from the manager.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param DialogueObjectsToRemove Array of objects to remove.
	 * @return Number of objects successfully removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static bool RemoveDialogueUIObjects(UObject* Target, const TArray<UObject*>& DialogueObjectsToRemove);

	/**
	 * Removes multiple dialogue UI objects from the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueObjectsToRemove Array of objects to remove.
	 * @return Number of objects successfully removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Remove Dialogue UI Objects (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static bool RemoveDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& DialogueObjectsToRemove);

	/**
	 * Sets the entire array of dialogue UI objects, replacing any existing objects.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NewDialogueObjects The new array of dialogue UI objects.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueUIObjects(UObject* Target, const TArray<UObject* >& NewDialogueObjects);

	/**
	 * Sets the entire array of dialogue UI objects, replacing any existing objects.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObjects The new array of dialogue UI objects.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue UI Objects (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects);

	/**
	 * Resets the dialogue manager, removing all dialogue UI objects.
	 *
	 *  @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void ResetDialogueUIObjects(UObject* Target);

	/**
	 * Resets the dialogue manager, removing all dialogue UI objects.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Reset Dialogue UI Objects (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void ResetDialogueUIObjectsV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);
	
	// --- Node functions ------------------------------

	/**
	 * Notifies the Dialogue  that a node has been selected.
	 * 
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NodeGUID	The GUID of the selected node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="select,chosen,option"), meta=(CustomTag="MounteaK2Setter"))
	static void CallDialogueNodeSelected(UObject* Target, const FGuid& NodeGUID);

	/**
	 * Notifies the Dialogue that a node has been selected.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @param NodeGUID	The GUID of the selected node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Call Dialogue Node Selected (Interface)", meta=(Keywords="select,chosen,option"), meta=(CustomTag="MounteaK2Setter"))
	static void CallDialogueNodeSelectedV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FGuid& NodeGUID);

	/**
	 * Prepares the node for execution.
	 * Asks Active Node to 'PreProcessNode' and then to 'ProcessNode'.
	 * In this preparation stage, Nodes are asked to process all Decorators.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void PrepareNode(UObject* Target);

	/**
	 * Prepares the node for execution.
	 * Asks Active Node to 'PreProcessNode' and then to 'ProcessNode'.
	 * In this preparation stage, Nodes are asked to process all Decorators.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Prepare Node (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void PrepareNodeV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Calls to the Node to Process it.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessNode(UObject* Target);

	/**
	 * Calls to the Node to Process it.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Process Node (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessNodeV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	// --- Dialogue functions ------------------------------

	/**
	 * Initializes the dialogue with the provided player state and participants. Provides the Manager and World to Nodes and Dialogue Graph.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param OwningPlayerState		The player state that owns this dialogue instance.
	 * @param Participants					A structure containing all the participants involved in the dialogue.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="start,begin,initialize,initiate"), meta=(CustomTag="MounteaK2Setter"))
	static void InitializeDialogue(UObject* Target, APlayerState* OwningPlayerState, const FDialogueParticipants& Participants);

	/**
	 * Initializes the dialogue with the provided player state and participants.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param OwningPlayerState		The player state that owns this dialogue instance.
	 * @param Participants			A structure containing all the participants involved in the dialogue.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Initialize Dialogue (Interface)", meta=(Keywords="start,begin,initialize,initiate"), meta=(CustomTag="MounteaK2Setter"))
	static void InitializeDialogueV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, APlayerState* OwningPlayerState, const FDialogueParticipants& Participants);
	
	/**
	 * Starts the Dialogue if possible.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="start"), meta=(CustomTag="MounteaK2Setter"))
	static void StartDialogue(UObject* Target);

	/**
	 * Starts the Dialogue if possible.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Start Dialogue (Interface)", meta=(Keywords="start"), meta=(CustomTag="MounteaK2Setter"))
	static void StartDialogueV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Closes the Dialogue if is active.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="exit,close"), meta=(CustomTag="MounteaK2Setter"))
	static void CloseDialogue(UObject* Target);

	/**
	 * Closes the Dialogue if is active.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Close Dialogue (Interface)", meta=(Keywords="exit,close"), meta=(CustomTag="MounteaK2Setter"))
	static void CloseDialogueV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	// --- Widget functions ------------------------------
	
	/**
	 * Tries to Invoke Dialogue UI.
	 * This function servers a purpose to try showing Dialogue UI to player.
	 * ❔ If this function fails, Message will be populated with error message explaining what went wrong.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @return			true if UI can be added to screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="show,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool InvokeDialogueUI(UObject* Target, FString& Message);

	/**
	 * Tries to Invoke Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @return			true if UI can be added to screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Invoke Dialogue UI (Interface)", meta=(Keywords="show,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool InvokeDialogueUIV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message);

	/**
	 * Tries to Update Dialogue UI.
	 * This function servers a purpose to try update Dialogue UI to player using given command.
	 * ❔ If this function fails, Message will be populated with error message explaining what went wrong.
	 * 
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @param Command	Command to be processed.
	 * @return			true if UI can be updated, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="show,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool UpdateDialogueUI(UObject* Target, FString& Message, const FString& Command);

	/**
	 * Tries to Update Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @param Command	Command to be processed.
	 * @return			true if UI can be updated, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Update Dialogue UI (Interface)", meta=(Keywords="show,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool UpdateDialogueUIV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message, const FString& Command);

	/**
	 * Tries to Close Dialogue UI.
	 * This function servers a purpose to try tear down Dialogue UI from player.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return			true if UI can be removed from screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="close,exit,stop,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool CloseDialogueUI(UObject* Target);

	/**
	 * Tries to Close Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return			true if UI can be removed from screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Close Dialogue UI (Interface)", meta=(Keywords="close,exit,stop,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool CloseDialogueUIV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Executes a specified command on a dialogue UI widget within the target object, if it supports the required interface.
	 * 
	 * This function allows developers to send specific commands to a dialogue widget, enabling customization or control over UI
	 * elements, provided the target implements the MounteaDialogueManagerInterface.
	 *
	 * @param Target	The target UObject containing the dialogue widget to receive the command. Must implement MounteaDialogueManagerInterface.
	 * @param Command	A string representing the command to be executed on the target widget (e.g., "Open", "Close", "Refresh").
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="UI,process"), meta=(CustomTag="MounteaK2Setter"))
	static void ExecuteWidgetCommand(UObject* Target, const FString& Command);

	/**
	 * Executes a specified command on a dialogue UI widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Command	A string representing the command to be executed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Execute Widget Command (Interface)", meta=(Keywords="UI,process"), meta=(CustomTag="MounteaK2Setter"))
	static void ExecuteWidgetCommandV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FString& Command);

	/**
	 * Sets Dialogue UI pointer.
	 * 
	 * ❔ Using null value resets saved value
	 * @param Target	The target UObject containing the dialogue widget to receive the command. Must implement MounteaDialogueManagerInterface.
	 * @param DialogueUIPtr	UserWidget pointer to be saved as Dialogue UI
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueWidget(UObject* Target, UUserWidget* DialogueUIPtr);

	/**
	 * Sets Dialogue UI pointer.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueUIPtr	UserWidget pointer to be saved as Dialogue UI
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue Widget (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueWidgetV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UUserWidget* DialogueUIPtr);

	/**
	 * Returns the widget used to display the current dialogue.
	 * 
	 * @param Target	The target UObject containing the dialogue widget to receive the command. Must implement MounteaDialogueManagerInterface.
	 * @return The widget used to display the current dialogue.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(Keywords="UI"), meta=(CustomTag="MounteaK2Getter"))
	static UUserWidget* GetDialogueWidget(UObject* Target);

	/**
	 * Returns the widget used to display the current dialogue.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @return The widget used to display the current dialogue.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Widget (Interface)", meta=(Keywords="UI"), meta=(CustomTag="MounteaK2Getter"))
	static UUserWidget* GetDialogueWidgetV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Gets the widget class used to display Dialogue UI.
	 * 
	 * @return The widget class used to display Dialogue UI.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(Keywords="UI, Widget"), meta=(CustomTag="MounteaK2Getter"))
	static TSubclassOf<UUserWidget> GetDialogueWidgetClass(UObject* Target);

	/**
	 * Gets the widget class used to display Dialogue UI.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @return The widget class used to display Dialogue UI.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Widget Class (Interface)", meta=(Keywords="UI, Widget"), meta=(CustomTag="MounteaK2Getter"))
	static TSubclassOf<UUserWidget> GetDialogueWidgetClassV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Retrieves the Z-order of the dialogue widget.
	 * The Z-order determines the rendering order of the widget, with higher values rendering on top of lower values.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return The current Z-order value of the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static int32 GetDialogueWidgetZOrder(UObject* Target);

	/**
	 * Retrieves the Z-order of the dialogue widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return The current Z-order value of the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Widget ZOrder (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static int32 GetDialogueWidgetZOrderV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Sets a new Z-order for the dialogue widget.
	 * The Z-order determines the rendering order of the widget, with higher values rendering on top of lower values.
	 * ❗ Runtime changes are not allowed!
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NewZOrder The new Z-order value to be applied to the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueWidgetZOrder(UObject* Target, const int32 NewZOrder);

	/**
	 * Sets a new Z-order for the dialogue widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewZOrder The new Z-order value to be applied to the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue Widget ZOrder (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueWidgetZOrderV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const int32 NewZOrder);

	// --- Dialogue Row functions ------------------------------

	/**
	 * Starts Dialogue Row execution.
	 * ❔ Dialogue Data contain Dialogue Data Rows, which are individual dialogue lines, which can be skipped.
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 *
	 * @param Target	The target UObject containing the dialogue widget to receive the command. Must implement MounteaDialogueManagerInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void StartExecuteDialogueRow(UObject* Target);

	/**
	 * Starts Dialogue Row execution.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Start Execute Dialogue Row (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void StartExecuteDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Function responsible for cleanup once Dialogue Row is finished.
	 * ❔ Dialogue Data contain Dialogue Data Rows, which are individual dialogue lines, which can be skipped.
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 * 
	 * @param Target	The target UObject containing the dialogue widget to receive the command. Must implement MounteaDialogueManagerInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void FinishedExecuteDialogueRow(UObject* Target);
	
	/**
	 * Function responsible for cleanup once Dialogue Row is finished.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Finished Execute Dialogue Row (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void FinishedExecuteDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Requests next dialogue row.
	 * Contains validation that current row must be 'ExecutionMode::AwaitInput'.
	 *
	 * @param Target	The target UObject containing the dialogue widget to receive the command. Must implement MounteaDialogueManagerInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void TriggerNextDialogueRow(UObject* Target);

	/**
	 * Requests next dialogue row.
	 * Contains validation that current row must be 'ExecutionMode::AwaitInput'.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Trigger Next Dialogue Row (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void TriggerNextDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Skips the current dialogue row.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void SkipDialogueRow(UObject* Target);

	/**
	 * Skips the current dialogue row.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Skip DialogueRow (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SkipDialogueRowV2(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

};
