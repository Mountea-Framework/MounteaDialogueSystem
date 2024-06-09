// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/MounteaDialogueWBPInterface.h"
#include "MounteaDialogue.generated.h"

/**
 * UMounteaDialogueOptionsContainer
 *
 * 
 */
UCLASS(DisplayName="Mountea Dialogue", ClassGroup=Monutea)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogue : public UUserWidget, public IMounteaDialogueWBPInterface
{
	GENERATED_BODY()

protected:
	
	// IMounteaDialogueWBPInterface implementation

protected:

	/**
	 * The class type of the dialogue options container widget. Must Implement 'MounteaDialogueOptionsContainerInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueOptionsContainerInterface"))
	TSoftClassPtr<UUserWidget> 													DialogueOptionsContainerClass;

	/**
	 * The class type of the dialogue option widget. Must Implement 'MounteaDialogueOptionInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueOptionInterface"))
	TSoftClassPtr<UUserWidget> 													DialogueOptionClass;
	
	/**
	 * The class type of the dialogue row widget. Must Implement 'MounteaDialogueRowInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueRowInterface"))
	TSoftClassPtr<UUserWidget> 													DialogueRowClass;
};
