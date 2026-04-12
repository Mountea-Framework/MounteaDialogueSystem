// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueGraphDataTypes.h"

#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"

FString FDialogueRow::ToString() const
{
	FString Result;

	Result += FString::Printf(TEXT("RowTitle: %s"), *RowTitle.ToString());
	Result += FString::Printf(TEXT(", DialogueParticipant: %s"), *DialogueParticipantName.ToString());
	Result += FString::Printf(TEXT(", UIRowID: %d"), UIRowID);
	Result += FString::Printf(TEXT(", RowGUID: %s"), *RowGUID.ToString(EGuidFormats::DigitsWithHyphensLower));

	Result += FString::Printf(TEXT(", DialogueRowData Count: %d"), RowData.Num());

	return Result;
}

void FDialogueRow::OnPostDataImport(const UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems)
{
	UpdateFromDialogueParticipantName();
}

void FDialogueRow::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
	UpdateFromDialogueParticipantName();
}

void FDialogueRow::UpdateFromDialogueParticipantName()
{
	if (DialogueParticipantName.IsNone() && !DialogueParticipant.IsEmpty())
		DialogueParticipantName = FName(*DialogueParticipant.ToString());

	if (DialogueParticipantName.IsNone())
		return;

	DialogueParticipant = FText::FromName(DialogueParticipantName);

	FGameplayTag participantTag = FGameplayTag();
	UMounteaDialogueParticipantStatics::FindParticipantDataRow(DialogueParticipantName, &participantTag);
	if (participantTag.IsValid() && !CompatibleTags.HasTagExact(participantTag))
		CompatibleTags.AddTag(participantTag);
}

