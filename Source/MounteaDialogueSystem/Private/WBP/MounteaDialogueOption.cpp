// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueOption.h"

#include "Components/Button.h"

UMounteaDialogueOption::UMounteaDialogueOption(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), DialogueOptionState(EDialogueOptionState::EDOS_Unfocused)
{
	bIsFocusable = true;
	bStopAction = true;
}

FReply UMounteaDialogueOption::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	DialogueOptionState = EDialogueOptionState::EDOS_Focused;
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
