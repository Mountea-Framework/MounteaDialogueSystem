// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Interfaces/UMG/MounteaFocusableWidgetInterface.h"
#include "MounteaDialogueOption.generated.h"

class UButton;
/**
 * UMounteaDialogueOption
 * 
 * A UserWidget class that implements the 'MounteaDialogueOptionInterface', providing functionalities for dialogue options in the Mountea Dialogue System.
 */
UCLASS(DisplayName="Mountea Dialogue Option", ClassGroup=Mountea)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueOption : public UUserWidget, public IMounteaDialogueOptionInterface, public IMounteaFocusableWidgetInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueOption(const FObjectInitializer& ObjectInitializer);

public:

	virtual bool NativeSupportsKeyboardFocus() const override { return true; };
	virtual void NativeConstruct() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;

protected:
	
	// IMounteaDialogueOptionInterface implementation
	virtual	FDialogueOptionData	GetDialogueOptionData_Implementation	() const	override;
	virtual	void	SetNewDialogueOptionData_Implementation	(const FDialogueOptionData& NewData)	override;
	virtual	void	ResetDialogueOptionData_Implementation	()	override;
	virtual	void	ProcessOptionSelected_Implementation	()	override;
	virtual	void	InitializeDialogueOption_Implementation	()	override;

	virtual	FOnDialogueOptionSelected&	GetDialogueOptionSelectedHandle	()	override
	{ return OnDialogueOptionSelected; };

public:

	virtual EDialogueOptionState GetFocusState_Implementation() const override
	{ return DialogueOptionState; };
	virtual void SetFocusState_Implementation(const bool IsSelected) override;

	virtual FOnMounteaFocusChanged& GetOnMounteaFocusChangedEventHandle() override
	{ return OnOptionFocusChanged; };

protected:

	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue|Focus")
	void OnOptionFocused(UUserWidget* FocusedWidget, const bool IsFocused);

protected:

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue|Data")
	EDialogueOptionState DialogueOptionState;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Mountea|Dialogue|Style", meta=(ExposeOnSpawn=true))
	FButtonStyle DialogueOptionStyle;
	
	/**
	 * Dialogue Option Data.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue", meta=(ExposeOnSpawn=true))
	FDialogueOptionData	DialogueOptionData;

	/**
	 * Event called upon selecting Dialogue Option.
	 */
	UPROPERTY(BlueprintReadOnly, BlueprintCallable, VisibleAnywhere, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Delegate"))
	FOnDialogueOptionSelected	OnDialogueOptionSelected;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, VisibleAnywhere, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Delegate"))
	FOnMounteaFocusChanged OnOptionFocusChanged;
};
