// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueRow.h"

FWidgetDialogueRow UMounteaDialogueRow::GetDialogueWidgetRowData_Implementation() const
{
	return DialogueRowData;
}

void UMounteaDialogueRow::SetNewWidgetDialogueRowData_Implementation(const FWidgetDialogueRow& NewData)
{
	if (DialogueRowData != NewData)
	{
		DialogueRowData = NewData;
	}
}

void UMounteaDialogueRow::ResetWidgetDialogueRow_Implementation()
{
	DialogueRowData.ResetRow();
}
