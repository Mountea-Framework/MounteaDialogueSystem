// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideDialogue.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

bool UMounteaDialogueDecorator_OverrideDialogue::ValidateDecorator_Implementation()
{
	bool bSatisfied = Super::ValidateDecorator_Implementation();
	
	if (DataTable == nullptr) bSatisfied = false;
	if (RowName.IsNone()) bSatisfied = false;
	if (RowName.IsNone()) bSatisfied = false;
	
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
