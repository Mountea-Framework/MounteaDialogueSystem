// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueUIBaseInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueUIBaseInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueUIBaseInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	/**
	 * Generic helper function to provide a global way to bind UI events.
	 * @return Binding result.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	bool BindEvents();
	virtual bool BindEvents_Implementation() = 0;

	/**
	 * Generic helper function to provide a global way to unbind UI events.
	 * @return Binding result.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	bool UnbindEvents();
	virtual bool UnbindEvents_Implementation() = 0;

	/**
	 * Generic helper function to provide easy way of sending commands around without need of binding.
	 * @param Command				Required string command to drive inner logic.
	 * @param OptionalPayload	Optional payload which can contain data for command.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	void ProcessStringCommand(const FString& Command, UObject* OptionalPayload = nullptr);
	virtual void ProcessStringCommand_Implementation(const FString& Command, UObject* OptionalPayload = nullptr) = 0;

	/**
	 * Helper function to provide easy way to request Theme update.
	 * Theme logic not provided to abstract the idea from any code.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	void ApplyTheme();
	virtual void ApplyTheme_Implementation() = 0;
};
