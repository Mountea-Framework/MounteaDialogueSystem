// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"

#include "Data/MounteaDialogueGraphDataTypes.h"


// Add default functionality here for any IMounteaDialogueOptionInterface functions that are not pure virtual.
FDialogueOptionData::FDialogueOptionData() :
	OptionGuid(FGuid()),
	OptionIcon(nullptr),
	UIRowID(0)
{
}

FDialogueOptionData::FDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow) :
	OptionGuid(Node),
	OptionTitle(DialogueRow.RowTitle),
	OptionIcon(DialogueRow.RowOptionalIcon),
	UIRowID(DialogueRow.UIRowID)
{
}
