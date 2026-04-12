// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/HUD/MounteaDialogueUIBaseInterface.h"
#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "MounteaDialogue.generated.h"

/**
 * UMounteaDialogueOptionsContainer
 *
 * 
 */
UCLASS(DisplayName="Mountea Dialogue", ClassGroup=Mountea)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogue : public UUserWidget, public IMounteaDialogueWBPInterface, public IMounteaDialogueUIBaseInterface
{
	GENERATED_BODY()

public:
		
	UMounteaDialogue(const FObjectInitializer& ObjectInitializer);
	
public:
	
	// MounteaDialogueUIBaseInterface ---
	
	virtual bool BindEvents_Implementation() override
	{
		return true;
	};
	
	virtual bool UnbindEvents_Implementation() override
	{
		return true;
	}
	
	virtual void ProcessStringCommand_Implementation(const FString& Command, UObject* OptionalPayload = nullptr) override
	{
		
	}
	
	virtual void ApplyTheme_Implementation() override
	{	
		
	}
	
protected:
	
	/**
	 * The class type of the dialogue options container widget. Must Implement 'MounteaDialogueOptionsContainerInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", 
		meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueOptionsContainerInterface", NoResetToDefault))
	TSoftClassPtr<UUserWidget>	DialogueOptionsContainerClass;

	/**
	 * The class type of the dialogue option widget. Must Implement 'MounteaDialogueOptionInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", 
		meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueOptionInterface", NoResetToDefault))
	TSoftClassPtr<UUserWidget>	DialogueOptionClass;
	
	/**
	 * The class type of the dialogue row widget. Must Implement 'MounteaDialogueRowInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", 
		meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueRowInterface", NoResetToDefault))
	TSoftClassPtr<UUserWidget>	DialogueRowClass;

	/**
	 * The class type of the dialogue row widget. Must Implement 'MounteaDialogueRowInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", 
		meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueSkipInterface", NoResetToDefault))
	TSoftClassPtr<UUserWidget>	DialogueSkipClass;
};
