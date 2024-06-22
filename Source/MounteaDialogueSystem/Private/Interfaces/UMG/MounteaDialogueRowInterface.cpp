// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "Interfaces/UMG/MounteaDialogueRowInterface.h"

#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueSystemBFC.h"


// Add default functionality here for any IMounteaDialogueRowInterface functions that are not pure virtual.

FWidgetDialogueRow::FWidgetDialogueRow()
	: DialogueRowParticipantName(FText::GetEmpty())
	, DialogueRowTitle(FText::GetEmpty())
	, DialogueRowBody(FText::GetEmpty())
	, RowDuration(0.0f)
	, UIRowID(0)
	, RowOptionalIcon(nullptr)
	, RowGuid(FGuid::NewGuid())
{
}

FWidgetDialogueRow::FWidgetDialogueRow(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData)
	: DialogueRowParticipantName(SourceRow.DialogueParticipant)
	, DialogueRowTitle(SourceRow.RowTitle)
	, DialogueRowBody(SourceRowData.RowText)
	, RowDuration(UMounteaDialogueSystemBFC::GetRowDuration(SourceRowData))
	, UIRowID(SourceRow.UIRowID)
	, RowOptionalIcon(SourceRow.RowOptionalIcon)
	, RowGuid(SourceRowData.RowGUID)
{
}
