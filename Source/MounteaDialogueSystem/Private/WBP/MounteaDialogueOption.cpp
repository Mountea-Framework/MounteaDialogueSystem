// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueOption.h"

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
