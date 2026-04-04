// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/MounteaDialogueUITypes.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueParticipantUIInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueParticipantUIInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Mountea Dialogue Participant UI Interface.
 *
 * Implemented by UMounteaDialogueParticipantUserInterfaceComponent (or any actor component
 * that drives dialogue UI for a single bound manager).
 *
 * Responsibilities:
 * - Manager binding lifecycle (BindToManager / UnbindFromManager)
 * - Single UI target ownership (SetUserInterface / GetUserInterface)
 * - Screen UI lifecycle (CreateDialogueUI / UpdateDialogueUI / CloseDialogueUI)
 * - UI input forwarding to manager authority (RequestSelectNode, RequestSkipDialogueRow, etc.)
 * - Signal dispatch driven by Client_ RPC from the manager (DispatchUISignal)
 *
 * The component is purely local — SetIsReplicatedByDefault(false).
 * All network communication flows manager → component via FMounteaDialogueUISignal.
 *
 * @see UMounteaDialogueParticipantUserInterfaceComponent
 * @see IMounteaDialogueManagerInterface
 * @see FMounteaDialogueUISignal
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueParticipantUIInterface
{
	GENERATED_BODY()

public:

	// --- Manager Binding -----------------------------------------------------------

	/**
	 * Stores the manager reference without binding events.
	 * Prefer BindToManager for full setup.
	 *
	 * @param NewManager  Manager to associate with this UI component.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(CustomTag="MounteaK2Setter"))
	void SetParentManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager);
	virtual void SetParentManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager) = 0;

	/**
	 * Returns the currently associated manager.
	 */
	UFUNCTION(BlueprintNativeEvent,
		BlueprintPure,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(CustomTag="MounteaK2Getter"))
	TScriptInterface<IMounteaDialogueManagerInterface> GetParentManager() const;
	virtual TScriptInterface<IMounteaDialogueManagerInterface> GetParentManager_Implementation() const = 0;

	/**
	 * Stores the manager reference and binds all manager events and the UI signal delegate.
	 * Safe to call multiple times — subsequent calls unbind the previous manager first.
	 *
	 * @param Manager  Manager to listen to.
	 */
	UFUNCTION(BlueprintNativeEvent,
		BlueprintCallable,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(CustomTag="MounteaK2Setter"))
	void BindToManager(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);
	virtual void BindToManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) = 0;

	/**
	 * Removes all event and signal delegate bindings and clears the manager reference.
	 */
	UFUNCTION(BlueprintNativeEvent,
		BlueprintCallable,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(CustomTag="MounteaK2Setter"))
	void UnbindFromManager();
	virtual void UnbindFromManager_Implementation() = 0;

	// --- UI Target ----------------------------------------------------------------

	/**
	 * Sets the single UI target object for this component.
	 * The target must implement IMounteaDialogueWBPInterface.
	 * May be a UUserWidget or a UWidgetComponent.
	 *
	 * @param NewUserInterface  The new UI target object.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(CustomTag="MounteaK2Setter"))
	void SetUserInterface(UObject* NewUserInterface);
	virtual void SetUserInterface_Implementation(UObject* NewUserInterface) = 0;

	/**
	 * Returns the current UI target object.
	 */
	UFUNCTION(BlueprintNativeEvent,
		BlueprintPure,
		Category="Mountea|Dialogue|Participant|UI",
		meta=(CustomTag="MounteaK2Getter"))
	UObject* GetUserInterface() const;
	virtual UObject* GetUserInterface_Implementation() const = 0;

	// --- Screen UI Lifecycle -------------------------------------------------------

	/**
	 * Creates and shows the dialogue UI target, adding it to the owning player's screen.
	 * No-op when ShouldExecuteCosmetics returns false (server-side components).
	 *
	 * @param Message  Out: human-readable result message for debugging.
	 * @return True if the UI was successfully created.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	bool CreateDialogueUI(FString& Message);
	virtual bool CreateDialogueUI_Implementation(FString& Message) = 0;

	/**
	 * Updates the dialogue UI target by forwarding a widget command.
	 *
	 * @param Message  Out: human-readable result message for debugging.
	 * @param Command  Widget command (see MounteaDialogueWidgetCommands namespace).
	 * @return True if the update was dispatched successfully.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	bool UpdateDialogueUI(FString& Message, const FString& Command);
	virtual bool UpdateDialogueUI_Implementation(FString& Message, const FString& Command) = 0;

	/**
	 * Signals the UI target to close, then clears the UserInterface reference.
	 * Does not destroy the target — lifetime management is the target's responsibility.
	 *
	 * @return True if the close sequence was executed.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	bool CloseDialogueUI();
	virtual bool CloseDialogueUI_Implementation() = 0;

	/**
	 * Executes an arbitrary widget command on the UI target.
	 *
	 * @param Command  Widget command to execute.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	void ExecuteWidgetCommand(const FString& Command);
	virtual void ExecuteWidgetCommand_Implementation(const FString& Command) = 0;

	// --- UI Input Forwarding ------------------------------------------------------

	/**
	 * Forwards a node selection request to the manager authority.
	 * No state is mutated locally — the manager handles RPC routing.
	 *
	 * @param NodeGuid  GUID of the node to select.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	void RequestSelectNode(const FGuid& NodeGuid);
	virtual void RequestSelectNode_Implementation(const FGuid& NodeGuid) = 0;

	/**
	 * Forwards a row-skip request to the manager authority.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	void RequestSkipDialogueRow();
	virtual void RequestSkipDialogueRow_Implementation() = 0;

	/**
	 * Forwards a dialogue close request to the manager authority.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	void RequestCloseDialogue();
	virtual void RequestCloseDialogue_Implementation() = 0;

	/**
	 * Forwards a row-processed notification to the manager authority.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	void RequestProcessDialogueRow();
	virtual void RequestProcessDialogueRow_Implementation() = 0;

	// --- Signal Dispatch ----------------------------------------------------------

	/**
	 * Entry point called when the manager broadcasts OnDialogueUISignalRequested.
	 * Implementations should queue signals whose RequiredContextVersion has not yet
	 * been satisfied and dispatch immediately otherwise.
	 *
	 * @param Signal  Version-stamped UI command from the server.
	 */
	UFUNCTION(BlueprintNativeEvent,
		Category="Mountea|Dialogue|Participant|UI")
	void DispatchUISignal(const FMounteaDialogueUISignal& Signal);
	virtual void DispatchUISignal_Implementation(const FMounteaDialogueUISignal& Signal) = 0;
};
