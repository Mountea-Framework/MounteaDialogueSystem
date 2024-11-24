// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueOption.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"

UMounteaDialogueOption::UMounteaDialogueOption(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsFocusable = true;
}

FReply UMounteaDialogueOption::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	LOG_WARNING(TEXT("Dialogue Option %s received %s key!"), *DialogueOptionData.OptionTitle.ToString(), *InKeyEvent.GetKey().ToString())
	return FReply::Handled();
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
