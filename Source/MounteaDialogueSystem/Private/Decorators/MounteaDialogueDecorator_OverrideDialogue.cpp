// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideDialogue.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_OverrideDialogue"

bool UMounteaDialogueDecorator_OverrideDialogue::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = FText::FromString(GetName());
	
	if (DataTable == nullptr)
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_DT", "{0} has no Data Table!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	if (RowName.IsNone() || RowName.IsNone())
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_DT", "[{0} Validation]: Invalid Row Name!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	return bSatisfied;
}

void UMounteaDialogueDecorator_OverrideDialogue::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	UMounteaDialogueContext* Context = nullptr;
	const auto Manager = UMounteaDialogueSystemBFC::GetDialogueManager(GetOwningWorld());

	// Let's return BP Updatable Context rather than Raw
	Context = Manager->GetDialogueContext();

	if (!Context || !UMounteaDialogueSystemBFC::IsContextValid(Context)) return;

	const auto NewRow = UMounteaDialogueSystemBFC::FindDialogueRow(DataTable, RowName);
	
	Context->UpdateActiveDialogueRow( UMounteaDialogueSystemBFC::FindDialogueRow(DataTable, RowName) );
}

#undef LOCTEXT_NAMESPACE