// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideDialogue.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_OverrideDialogue"

void UMounteaDialogueDecorator_OverrideDialogue::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
}

bool UMounteaDialogueDecorator_OverrideDialogue::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetDecoratorName();
	
	if (DataTable == nullptr)
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_DT", "Decorator {0} has no Data Table!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	if (RowName.IsNone() || RowName.IsNone())
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_DT", "Decorator {0}: Invalid Row Name!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	return bSatisfied;
}

void UMounteaDialogueDecorator_OverrideDialogue::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (!OwningManager) return;
	
	// Let's return BP Updatable Context rather than Raw
	Context = OwningManager->GetDialogueContext();

	// We assume Context and Manager are already valid, but safety is safety
	if (!Context || !OwningManager.GetInterface() || !UMounteaDialogueSystemBFC::IsContextValid(Context) ) return;
	
	const auto NewRow = UMounteaDialogueSystemBFC::FindDialogueRow(DataTable, RowName);
	
	Context->UpdateActiveDialogueRow( UMounteaDialogueSystemBFC::FindDialogueRow(DataTable, RowName) );
}

#undef LOCTEXT_NAMESPACE