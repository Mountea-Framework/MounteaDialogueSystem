// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/UMG/MounteaDialogueOptionsContainerInterface.h"
#include "MounteaDialogueOptionsContainer.generated.h"

class UMounteaDialogueGraphNode_DialogueNodeBase;

/**
 * UMounteaDialogueOptionsContainer
 *
 * A UserWidget class that implements the 'MounteaDialogueOptionsContainerInterface', providing functionalities for dialogue option containers in the Mountea Dialogue System.
 */
UCLASS(DisplayName="Mountea Dialogue Options Container", ClassGroup=Mountea)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueOptionsContainer : public UUserWidget, public IMounteaDialogueOptionsContainerInterface
{
	GENERATED_BODY()

protected:
	// IMounteaDialogueOptionsContainerInterface implementation
	virtual void SetParentDialogueWidget_Implementation(UUserWidget* NewParentDialogueWidget) override;
	virtual UUserWidget* GetParentDialogueWidget_Implementation() const override;
	virtual TSoftClassPtr<UUserWidget> GetDialogueOptionClass_Implementation() const override;
	virtual void SetDialogueOptionClass_Implementation(const TSoftClassPtr<UUserWidget>& NewDialogueOptionClass) override;
	virtual void AddNewDialogueOption_Implementation(UMounteaDialogueGraphNode_DialogueNodeBase* Node) override;
	virtual void AddNewDialogueOptions_Implementation(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& NewDialogueOptions) override;
	virtual void RemoveDialogueOption_Implementation(UMounteaDialogueGraphNode_DialogueNodeBase* DirtyDialogueOption) override;
	virtual void RemoveDialogueOptions_Implementation(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& DirtyDialogueOptions) override;
	virtual void ClearDialogueOptions_Implementation() override;
	virtual void ProcessOptionSelected_Implementation(const FGuid& SelectedOption, UUserWidget* CallingWidget) override;
	virtual TArray<UUserWidget*> GetDialogueOptions_Implementation() const override;

protected:
	/**
	 * The class type of the dialogue option widget. Must Implement 'MounteaDialogueOptionInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueOptionInterface", NoResetToDefault))
	TSoftClassPtr<UUserWidget> DialogueOptionClass;

	/**
	 * The parent dialogue widget. Must implement 'MounteaDialogueWBPInterface'.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface", NoResetToDefault))
	TObjectPtr<UUserWidget> ParentDialogueWidget;

	/**
	 * All Dialogue options associated with this container.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue")
	TMap<FGuid, TObjectPtr<UUserWidget>> DialogueOptions;
};
