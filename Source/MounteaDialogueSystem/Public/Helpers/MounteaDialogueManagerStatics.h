// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/MounteaDialogueManagerInterface.h"

#include "MounteaDialogueGraphHelpers.h"
#include "MounteaDialogueManagerStatics.generated.h"

enum class EDialogueManagerState : uint8;

/**
 * This library contains implementations of Dialogue Manager Function.
 * Dialogue Manager Interface does not have any BlueprintCallable functions.
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
	 * @param Target	Dialogue Manager interface.
	 * @return The owning actor for this Dialogue Manager.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Owning Actor", meta=(CustomTag="MounteaK2Getter"))
	static AActor* GetOwningActor(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Returns the Dialogue Manager as Object.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return The Dialogue Manager as Object.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Manager Object", meta=(CustomTag="MounteaK2Getter"))
	static UObject* GetManagerObject(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Retrieves current Dialogue Context.
	 * 
	 * ❗ Could be null
	 * @param Target	Dialogue Manager interface.
	 * @return DialogueContext	Dialogue Context is transient data holder for current dialogue instance.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Context", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueContext* GetDialogueContext(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Retrieves current Dialogue Manager State.
	 * State defines whether Manager can start/close dialogue or not.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return ManagerState	Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Manager State", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Sets new Dialogue Manager State.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewState	Manager State to be set as Manager State
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue Manager State", meta=(CustomTag="MounteaK2Getter"))
	static void SetDialogueManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const EDialogueManagerState NewState);

	/**
	 * Retrieves current Default Dialogue Manager State.
	 * Default Dialogue Manager State sets Dialogue Manager state upon BeginPlay and is used as fallback once Dialogue ends.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return ManagerState	Default Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Default Dialogue Manager State", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetDefaultDialogueManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);
	
	/**
	 * Adds a single dialogue UI object to the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObject The object to add.
	 * @return True if the object was successfully added, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Add Dialogue UI Object", meta=(CustomTag="MounteaK2Setter"))
	static bool AddDialogueUIObject(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* NewDialogueObject);

	/**
	 * Adds an array of dialogue UI objects to the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObjects Array of objects to add.
	 * @return Number of objects successfully added.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Add Dialogue UI Objects", meta=(CustomTag="MounteaK2Setter"))
	static bool AddDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects);

	/**
	 * Removes a single dialogue UI object from the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueObjectToRemove The object to remove.
	 * @return True if the object was successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Remove Dialogue UI Object", meta=(CustomTag="MounteaK2Setter"))
	static bool RemoveDialogueUIObject(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* DialogueObjectToRemove);

	/**
	 * Removes multiple dialogue UI objects from the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueObjectsToRemove Array of objects to remove.
	 * @return Number of objects successfully removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Remove Dialogue UI Objects", meta=(CustomTag="MounteaK2Setter"))
	static bool RemoveDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& DialogueObjectsToRemove);

	/**
	 * Sets the entire array of dialogue UI objects, replacing any existing objects.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObjects The new array of dialogue UI objects.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue UI Objects", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects);

	/**
	 * Resets the dialogue manager, removing all dialogue UI objects.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Reset Dialogue UI Objects", meta=(CustomTag="MounteaK2Setter"))
	static void ResetDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);
	
	// --- Node functions ------------------------------

	/**
	 * Notifies the Dialogue that a node has been selected.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @param NodeGUID	The GUID of the selected node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Call Dialogue Node Selected", meta=(Keywords="select,chosen,option"), meta=(CustomTag="MounteaK2Setter"))
	static void CallDialogueNodeSelected(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FGuid& NodeGUID);

	/**
	 * Prepares the node for execution.
	 * Asks Active Node to 'PreProcessNode' and then to 'ProcessNode'.
	 * In this preparation stage, Nodes are asked to process all Decorators.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Prepare Node", meta=(CustomTag="MounteaK2Setter"))
	static void PrepareNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Calls to the Node to Process it.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Process Node", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	// --- Dialogue functions ------------------------------

	/**
	 * Closes the Dialogue if is active.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Close Dialogue", meta=(Keywords="exit,close"), meta=(CustomTag="MounteaK2Setter"))
	static void CloseDialogue(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	// --- Widget functions ------------------------------

	/**
	 * Tries to Create Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @return			true if UI can be added to screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Create Dialogue UI", meta=(Keywords="show,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool CreateDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message);

	/**
	 * Tries to Update Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @param Command	Command to be processed.
	 * @return			true if UI can be updated, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Update Dialogue UI", meta=(Keywords="show,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool UpdateDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message, const FString& Command);

	/**
	 * Tries to Close Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return			true if UI can be removed from screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Close Dialogue UI", meta=(Keywords="close,exit,stop,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool CloseDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Executes a specified command on a dialogue UI widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Command	A string representing the command to be executed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Execute Widget Command", meta=(Keywords="UI,process"), meta=(CustomTag="MounteaK2Setter"))
	static void ExecuteWidgetCommand(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FString& Command);

	/**
	 * Sets Dialogue UI pointer.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueUIPtr	UserWidget pointer to be saved as Dialogue UI
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue Widget", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueWidget(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UUserWidget* DialogueUIPtr);

	/**
	 * Returns the widget used to display the current dialogue.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @return The widget used to display the current dialogue.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Widget", meta=(Keywords="UI"), meta=(CustomTag="MounteaK2Getter"))
	static UUserWidget* GetDialogueWidget(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Gets the widget class used to display Dialogue UI.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @return The widget class used to display Dialogue UI.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Widget Class", meta=(Keywords="UI, Widget"), meta=(CustomTag="MounteaK2Getter"))
	static TSubclassOf<UUserWidget> GetDialogueWidgetClass(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Retrieves the Z-order of the dialogue widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return The current Z-order value of the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", DisplayName="Get Dialogue Widget ZOrder", meta=(CustomTag="MounteaK2Getter"))
	static int32 GetDialogueWidgetZOrder(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Sets a new Z-order for the dialogue widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewZOrder The new Z-order value to be applied to the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Set Dialogue Widget ZOrder", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueWidgetZOrder(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const int32 NewZOrder);

	// --- Dialogue Row functions ------------------------------

	/**
	 * Starts Dialogue Row execution.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Start Execute Dialogue Row", meta=(CustomTag="MounteaK2Setter"))
	static void StartExecuteDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);
	
	/**
	 * Function responsible for cleanup once Dialogue Row is finished.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Finished Execute Dialogue Row", meta=(CustomTag="MounteaK2Setter"))
	static void FinishedExecuteDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Requests next dialogue row.
	 * Contains validation that current row must be 'ExecutionMode::AwaitInput'.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Trigger Next Dialogue Row", meta=(CustomTag="MounteaK2Setter"))
	static void TriggerNextDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Skips the current dialogue row.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", DisplayName="Skip DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	static void SkipDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

};
