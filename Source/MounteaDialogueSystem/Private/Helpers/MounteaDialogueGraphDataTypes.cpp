// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueGraphDataTypes.h"

/* Serialization is not needed, data can be found locally for Clients -> saving bandwith as well
void FDialogueRow::SerializeDialogueRowData()
{
	ReplicatedDialogueRowData.Reset();
	for (const FDialogueRowData& RowData : DialogueRowData)
	{
		ReplicatedDialogueRowData.Add(RowData);
	}
}

void FDialogueRow::DeserializeDialogueRowData()
{
	DialogueRowData.Reset();
	for (const FDialogueRowData& RowData : ReplicatedDialogueRowData)
	{
		DialogueRowData.Add(RowData);
	}
}
*/