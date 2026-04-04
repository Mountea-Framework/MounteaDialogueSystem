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
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/Core/MounteaDialogueParticipantUIInterface.h"
#include "MounteaDialogueParticipantUIStatics.generated.h"

/**
 * Blueprint Function Library exposing IMounteaDialogueParticipantUIInterface.
 *
 * Use these functions to interact with any actor that hosts a
 * UMounteaDialogueParticipantUserInterfaceComponent.
 *
 * @see IMounteaDialogueParticipantUIInterface
 * @see UMounteaDialogueParticipantUserInterfaceComponent
 */
UCLASS(meta=(DisplayName="Mountea Dialogue Participant UI Statics"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipantUIStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// --- Component Discovery --------------------------------------------------

	/**
	 * Finds the first component on Actor that implements IMounteaDialogueParticipantUIInterface.
	 *
	 * @param ForActor  Actor to search.
	 * @return          The UI interface, or an empty script interface if none found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Find Participant UI Component"),
		meta=(CustomTag="MounteaK2Getter"))
	static TScriptInterface<IMounteaDialogueParticipantUIInterface> FindParticipantUIComponent(AActor* ForActor);

	// --- Manager Binding ------------------------------------------------------

	/**
	 * Sets the manager reference on the UI component without binding events.
	 *
	 * @param Target      UI component to update.
	 * @param NewManager  Manager to associate.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Set Parent Manager"),
		meta=(CustomTag="MounteaK2Setter"))
	static void SetParentManager(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager);

	/**
	 * Returns the manager currently associated with this UI component.
	 *
	 * @param Target  UI component to query.
	 * @return        The associated manager interface.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Get Parent Manager"),
		meta=(CustomTag="MounteaK2Getter"))
	static TScriptInterface<IMounteaDialogueManagerInterface> GetParentManager(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target);

	/**
	 * Binds the UI component to a manager, registering for UI signals and lifecycle events.
	 * Safe to call multiple times — unbinds the previous manager first.
	 *
	 * @param Target   UI component to bind.
	 * @param Manager  Manager to listen to.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Bind To Manager"),
		meta=(CustomTag="MounteaK2Setter"))
	static void BindToManager(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);

	/**
	 * Removes all manager bindings and clears the manager reference.
	 *
	 * @param Target  UI component to unbind.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Unbind From Manager"),
		meta=(CustomTag="MounteaK2Setter"))
	static void UnbindFromManager(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target);

	// --- UI Target ------------------------------------------------------------

	/**
	 * Sets the single UI target object. Must implement IMounteaDialogueWBPInterface.
	 *
	 * @param Target          UI component to update.
	 * @param NewUserInterface  The new UI target.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Set User Interface"),
		meta=(CustomTag="MounteaK2Setter"))
	static void SetUserInterface(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		UObject* NewUserInterface);

	/**
	 * Returns the current UI target object.
	 *
	 * @param Target  UI component to query.
	 * @return        The current UI target, or null.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Get User Interface"),
		meta=(CustomTag="MounteaK2Getter"))
	static UObject* GetUserInterface(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target);

	// --- Screen UI Lifecycle --------------------------------------------------

	/**
	 * Creates and shows the dialogue UI target.
	 *
	 * @param Target   UI component to create UI for.
	 * @param Message  Out: human-readable result message.
	 * @return         True if the UI was successfully created.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Create Dialogue UI"),
		meta=(CustomTag="MounteaK2Setter"))
	static bool CreateDialogueUI(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		FString& Message);

	/**
	 * Updates the dialogue UI by forwarding a widget command.
	 *
	 * @param Target   UI component to update.
	 * @param Message  Out: human-readable result message.
	 * @param Command  Widget command to execute.
	 * @return         True if the update was dispatched successfully.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Update Dialogue UI"),
		meta=(CustomTag="MounteaK2Setter"))
	static bool UpdateDialogueUI(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		FString& Message,
		const FString& Command);

	/**
	 * Closes the dialogue UI and clears the UserInterface reference.
	 *
	 * @param Target  UI component to close UI for.
	 * @return        True if the close sequence was executed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Close Dialogue UI"),
		meta=(CustomTag="MounteaK2Setter"))
	static bool CloseDialogueUI(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target);

	/**
	 * Executes an arbitrary widget command on the UI target.
	 *
	 * @param Target   UI component.
	 * @param Command  Widget command to execute.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Execute Widget Command"),
		meta=(CustomTag="MounteaK2Setter"))
	static void ExecuteWidgetCommand(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		const FString& Command);

	// --- Input Forwarding -----------------------------------------------------

	/**
	 * Forwards a node selection request to the manager authority.
	 *
	 * @param Target    UI component.
	 * @param NodeGuid  GUID of the node to select.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Request Select Node"),
		meta=(CustomTag="MounteaK2Setter"))
	static void RequestSelectNode(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		const FGuid& NodeGuid);

	/**
	 * Forwards a row-skip request to the manager authority.
	 *
	 * @param Target  UI component.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Request Skip Dialogue Row"),
		meta=(CustomTag="MounteaK2Setter"))
	static void RequestSkipDialogueRow(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target);

	/**
	 * Forwards a dialogue close request to the manager authority.
	 *
	 * @param Target  UI component.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Request Close Dialogue"),
		meta=(CustomTag="MounteaK2Setter"))
	static void RequestCloseDialogue(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target);

	/**
	 * Forwards a row-processed notification to the manager authority.
	 *
	 * @param Target  UI component.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Request Process Dialogue Row"),
		meta=(CustomTag="MounteaK2Setter"))
	static void RequestProcessDialogueRow(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target);

	// --- Signal Dispatch ------------------------------------------------------

	/**
	 * Delivers a UI signal directly to the component. Normally called by the Manager
	 * via Client_DispatchUISignal, but exposed here for testing and scripting.
	 *
	 * @param Target  UI component.
	 * @param Signal  Version-stamped UI command to dispatch.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(DisplayName="Dispatch UI Signal"),
		meta=(CustomTag="MounteaK2Setter"))
	static void DispatchUISignal(
		const TScriptInterface<IMounteaDialogueParticipantUIInterface>& Target,
		const FMounteaDialogueUISignal& Signal);
};
