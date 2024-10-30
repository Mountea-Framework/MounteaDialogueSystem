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

	/**
	 * Notifies the Dialogue  that a node has been selected.
	 * 
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NodeGUID	The GUID of the selected node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="select,chosen,option"))
	static void CallDialogueNodeSelected(UObject* Target, const FGuid& NodeGUID);

	/**
	 * Starts the Dialogue if possible.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="start"), meta=(CustomTag="MounteaK2Setter"))
	static void StartDialogue(UObject* Target);

	/**
	 * Closes the Dialogue if is active.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="exit,close"), meta=(CustomTag="MounteaK2Setter"))
	static void CloseDialogue(UObject* Target);
	
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
	 * Tries to Close Dialogue UI.
	 * This function servers a purpose to try tear down Dialogue UI from player.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @return			true if UI can be removed from screen, false if cannot
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="close,exit,stop,widget"), meta=(CustomTag="MounteaK2Setter"))
	static bool CloseDialogueUI(UObject* Target);

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
	 * Sets Dialogue UI pointer.
	 * 
	 * ❔ Using null value resets saved value
	 * @param Target	The target UObject containing the dialogue widget to receive the command. Must implement MounteaDialogueManagerInterface.
	 * @param DialogueUIPtr	UserWidget pointer to be saved as Dialogue UI
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueWidget(UObject* Target, UUserWidget* DialogueUIPtr);

	/**
	 * Sets new Dialogue Manager State.
	 *
	 * @param Target	Dialogue Manager object. Must implement `IMounteaDialogueManager` interface.
	 * @param NewState	Manager State to be set as Manager State
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Manager", meta=(Keywords="state,status"), meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueManagerState(UObject* Target, const EDialogueManagerState NewState);

};
