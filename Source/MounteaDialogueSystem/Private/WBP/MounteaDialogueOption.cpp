// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueOption.h"

#include "Components/Button.h"

UMounteaDialogueOption::UMounteaDialogueOption(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), DialogueOptionState(EDialogueOptionState::EDOS_Unfocused)
{
	SetIsFocusable(true);
}

void UMounteaDialogueOption::NativeConstruct()
{
	Super::NativeConstruct();

	OnOptionFocusChanged.AddUniqueDynamic(this, &UMounteaDialogueOption::OnOptionFocused);
}

FReply UMounteaDialogueOption::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	Execute_SetFocusState(this, true);
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

FDialogueOptionData UMounteaDialogueOption::GetDialogueOptionData_Implementation() const
{
	return DialogueOptionData;
}

void UMounteaDialogueOption::SetNewDialogueOptionData_Implementation(const FDialogueOptionData& NewData)
{
	if (NewData != DialogueOptionData)
	{
		DialogueOptionData = NewData;
		Execute_InitializeDialogueOption(this);
	}
}

void UMounteaDialogueOption::ResetDialogueOptionData_Implementation()
{
	DialogueOptionData.ResetOption();
}

void UMounteaDialogueOption::ProcessOptionSelected_Implementation()
{
	OnDialogueOptionSelected.Broadcast(DialogueOptionData.OptionGuid, this);
}

void UMounteaDialogueOption::InitializeDialogueOption_Implementation()
{
	// ...
}

void UMounteaDialogueOption::SetFocusState_Implementation(const bool IsSelected)
{
	DialogueOptionState = IsSelected ? EDialogueOptionState::EDOS_Focused : EDialogueOptionState::EDOS_Unfocused;

	OnOptionFocusChanged.Broadcast(this, IsSelected);
}
