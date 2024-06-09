// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "MounteaDialogueOption.generated.h"

/**
 * UMounteaDialogueOption
 * 
 * A UserWidget class that implements the IMounteaDialogueOptionInterface, providing functionalities 
 * for dialogue options in the Mountea Dialogue System.
 */
UCLASS(DisplayName="Mountea Dialogue Option", ClassGroup=Monutea)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueOption : public UUserWidget, public IMounteaDialogueOptionInterface
{
	GENERATED_BODY()

protected:
	
	// IMounteaDialogueOptionInterface implementation
	virtual		FDialogueOptionData				GetDialogueOptionData_Implementation			() const														override;
	virtual		void											SetNewDialogueOptionData_Implementation	(const FDialogueOptionData& NewData)	override;
	virtual		void											ResetDialogueOptionData_Implementation		()																	override;
	virtual		void											ProcessOptionSelected_Implementation			()																	override;
	virtual		void											InitializeDialogueOption_Implementation			()																	override;

	virtual		FOnDialogueOptionSelected& GetDialogueOptionSelectedHandle					()																	override
	{ return OnDialogueOptionSelected; };

protected:
	
	/**
	 * Dialogue Option Data.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue", meta=(ExposeOnSpawn=true))
	FDialogueOptionData 										DialogueOptionData;

	/**
	 * Event called upon selecting Dialogue Option.
	 */
	UPROPERTY(BlueprintReadOnly, BlueprintCallable, VisibleAnywhere, Category="Mountea|Dialogue")
	FOnDialogueOptionSelected							OnDialogueOptionSelected;
};
