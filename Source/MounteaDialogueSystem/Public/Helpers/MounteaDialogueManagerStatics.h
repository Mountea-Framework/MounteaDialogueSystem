// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"

#include "MounteaDialogueGraphHelpers.h"
#include "MounteaDialogueManagerStatics.generated.h"

enum class EDialogueManagerState : uint8;
class UMounteaDialogueGraph;
class IMounteaDialogueParticipantInterface;
class AActor;

/**
 * This library contains implementations of Dialogue Manager Functions.
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
	
	static TScriptInterface<IMounteaDialogueManagerInterface> FindDialogueManagerInterface(UObject* ManagerActor, bool& bResult);
	
	static TScriptInterface<IMounteaDialogueManagerInterface> FindDialogueManagerInterface(AActor* CandidateActor, int& SearchDepth);

	// --- Actor functions ------------------------------

	/**
	 * Returns the owning actor for this Dialogue Manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return The owning actor for this Dialogue Manager.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static AActor* GetOwningActor(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Retrieves current Dialogue Manager State.
	 * State defines whether Manager can start/close dialogue or not.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return ManagerState	Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Sets new Dialogue Manager State.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewState	Manager State to be set as Manager State
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static void SetManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const EDialogueManagerState NewState);

	/**
	 * Retrieves current Default Dialogue Manager State.
	 * Default Dialogue Manager State sets Dialogue Manager state upon BeginPlay and is used as fallback once Dialogue ends.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return ManagerState	Default Manager state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerState GetDefaultDialogueManagerState(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueManagerType GetDialogueManagerType(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Validate"))
	static bool IsServer(const AActor* Owner);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Validate"))
	static bool IsLocalPlayer(const AActor* Owner);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Validate"))
	static bool ShouldExecuteCosmetics(const AActor* Owner);

	static UMounteaDialogueGraph* ResolveGraphByGuid(
		const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& Participants,
		const FGuid& GraphGuid);

	// --- Dialogue functions ------------------------------

	/**
	 * Retrieves current Dialogue Context.
	 * 
	 * ❗ Could be null
	 * @param Target	Dialogue Manager interface.
	 * @return DialogueContext	Dialogue Context is transient data holder for current dialogue instance.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueContext* GetDialogueContext(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager", meta=(Keywords="UI"), meta=(CustomTag="MounteaK2Validate"))
	static bool CanStartDialogue(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void RequestStartDialogue(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	
	/**
	 * Closes the Dialogue if is active.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="exit,close"), meta=(CustomTag="MounteaK2Setter"))
	static void RequestCloseDialogue(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);
	
	// --- Node functions ------------------------------
	
	/**
	 * Prepares the node for execution.
	 * Asks Active Node to 'PreProcessNode' and then to 'ProcessNode'.
	 * In this preparation stage, Nodes are asked to process all Decorators.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void PrepareNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);
	
	/**
	 * Calls to the Node to Process it.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Notifies the Dialogue that a node has been selected.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @param NodeGUID	The GUID of the selected node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="select,chosen,option"), meta=(CustomTag="MounteaK2Setter"))
	static void SelectNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FGuid& NodeGUID);
	
	// --- World UI functions ------------------------------
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static void UpdateWorldDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FString& Command)
	{
		LOG_ERROR(TEXT("[UpdateWorldDialogueUI] Deprecated API called. Use Participant UI Component + UI statics."));
	}
	
	/**
	 * Adds a single dialogue UI object to the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObject The object to add.
	 * @return True if the object was successfully added, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static bool AddDialogueUIObject(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* NewDialogueObject)
	{
		LOG_ERROR(TEXT("[AddDialogueUIObject] Deprecated API called. Use Participant UI Component + UI statics."));
		return false;
	}

	/**
	 * Adds an array of dialogue UI objects to the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObjects Array of objects to add.
	 * @return Number of objects successfully added.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static bool AddDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects)
	{
		LOG_ERROR(TEXT("[AddDialogueUIObjects] Deprecated API called. Use Participant UI Component + UI statics."));
		return false;
	}

	/**
	 * Removes a single dialogue UI object from the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueObjectToRemove The object to remove.
	 * @return True if the object was successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static bool RemoveDialogueUIObject(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UObject* DialogueObjectToRemove)
	{
		LOG_ERROR(TEXT("[RemoveDialogueUIObject] Deprecated API called. Use Participant UI Component + UI statics."));
		return false;
	}

	/**
	 * Removes multiple dialogue UI objects from the manager.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueObjectsToRemove Array of objects to remove.
	 * @return Number of objects successfully removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static bool RemoveDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& DialogueObjectsToRemove)
	{
		LOG_ERROR(TEXT("[RemoveDialogueUIObjects] Deprecated API called. Use Participant UI Component + UI statics."));
		return false;
	}

	/**
	 * Sets the entire array of dialogue UI objects, replacing any existing objects.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewDialogueObjects The new array of dialogue UI objects.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static void SetDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const TArray<UObject*>& NewDialogueObjects)
	{
		LOG_ERROR(TEXT("[SetDialogueUIObjects] Deprecated API called. Use Participant UI Component + UI statics."));
	}

	/**
	 * Resets the dialogue manager, removing all dialogue UI objects.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static void ResetDialogueUIObjects(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
	{
		LOG_ERROR(TEXT("[ResetDialogueUIObjects] Deprecated API called. Use Participant UI Component + UI statics."));
	}
	
	// --- Widget functions ------------------------------

	/**
	 * Tries to Create Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @return			true if UI can be added to screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(Keywords="show,widget"),
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static bool CreateDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message)
	{
		Message = TEXT("Deprecated API: CreateDialogueUI. Use Participant UI Component + UI statics.");
		LOG_ERROR(TEXT("[CreateDialogueUI] %s"), *Message);
		return false;
	}

	/**
	 * Tries to Update Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Message	InMessage to be populated with error message explaining why returns false
	 * @param Command	Command to be processed.
	 * @return			true if UI can be updated, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(Keywords="show,widget"),
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static bool UpdateDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, FString& Message, const FString& Command)
	{
		Message = TEXT("Deprecated API: UpdateDialogueUI. Use Participant UI Component + UI statics.");
		LOG_ERROR(TEXT("[UpdateDialogueUI] %s"), *Message);
		return false;
	}

	/**
	 * Tries to Close Dialogue UI.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return			true if UI can be removed from screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(Keywords="close,exit,stop,widget"),
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static bool CloseDialogueUI(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
	{
		LOG_ERROR(TEXT("[CloseDialogueUI] Deprecated API called. Use Participant UI Component + UI statics."));
		return false;
	}

	/**
	 * Executes a specified command on a dialogue UI widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param Command	A string representing the command to be executed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(Keywords="UI,process"),
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static void ExecuteWidgetCommand(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const FString& Command)
	{
		LOG_ERROR(TEXT("[ExecuteWidgetCommand] Deprecated API called. Use Participant UI Component + UI statics."));
	}

	/**
	 * Sets Dialogue UI pointer.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @param DialogueUIPtr	UserWidget pointer to be saved as Dialogue UI
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static void SetDialogueWidget(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, UUserWidget* DialogueUIPtr)
	{
		LOG_ERROR(TEXT("[SetDialogueWidget] Deprecated API called. Use Participant UI Component + UI statics."));
	}

	/**
	 * Returns the widget used to display the current dialogue.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @return The widget used to display the current dialogue.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager",
		meta=(Keywords="UI"),
		meta=(CustomTag="MounteaK2Getter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static UUserWidget* GetDialogueWidget(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
	{
		LOG_ERROR(TEXT("[GetDialogueWidget] Deprecated API called. Use Participant UI Component + UI statics."));
		return nullptr;
	}

	/**
	 * Gets the widget class used to display Dialogue UI.
	 * 
	 * @param Target	Dialogue Manager interface.
	 * @return The widget class used to display Dialogue UI.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager",
		meta=(Keywords="UI"),
		meta=(CustomTag="MounteaK2Getter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static TSubclassOf<UUserWidget> GetDialogueWidgetClass(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
	{
		LOG_ERROR(TEXT("[GetDialogueWidgetClass] Deprecated API called. Use Participant UI Component + UI statics."));
		return nullptr;
	}

	/**
	 * Retrieves the Z-order of the dialogue widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @return The current Z-order value of the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Getter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static int32 GetDialogueWidgetZOrder(const TScriptInterface<IMounteaDialogueManagerInterface>& Target)
	{
		LOG_ERROR(TEXT("[GetDialogueWidgetZOrder] Deprecated API called. Use Participant UI Component + UI statics."));
		return INDEX_NONE;
	}

	/**
	 * Sets a new Z-order for the dialogue widget.
	 *
	 * @param Target	Dialogue Manager interface.
	 * @param NewZOrder The new Z-order value to be applied to the dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager",
		meta=(CustomTag="MounteaK2Setter"),
		meta=(DeprecatedFunction, DeprecationMessage="Manager UI APIs are deprecated. Use Participant UI Component + UI statics."))
	UE_DEPRECATED(5.5, "Manager UI APIs are deprecated. Use Participant UI Component + UI statics.")
	static void SetDialogueWidgetZOrder(const TScriptInterface<IMounteaDialogueManagerInterface>& Target, const int32 NewZOrder)
	{
		LOG_ERROR(TEXT("[SetDialogueWidgetZOrder] Deprecated API called. Use Participant UI Component + UI statics."));
	}

	// --- Dialogue Row functions ------------------------------

	/**
	 * Starts Dialogue Row execution.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);
	
	/**
	 * Function responsible for cleanup once Dialogue Row is finished.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void DialogueRowProcessed(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

	/**
	 * Skips the current dialogue row.
	 *
	 * @param Target	Dialogue Manager interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void SkipDialogueRow(const TScriptInterface<IMounteaDialogueManagerInterface>& Target);

};
