// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideOnlyFirstTime.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_OverrideOnlyFirstTime"

bool UMounteaDialogueDecorator_OverrideOnlyFirstTime::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetDecoratorName();
	
	if (DataTable == nullptr)
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideOnlyFirstTime_Validation_DT", "Decorator {0} has no Data Table!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	if (RowName.IsNone() || RowName.IsNone())
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideOnlyFirstTime_Validation_DT", "Decorator {0}: Invalid Row Name!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	return bSatisfied;
}

void UMounteaDialogueDecorator_OverrideOnlyFirstTime::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (const auto TempContext = GetContext())
	{
		// We assume Context and Manager are already valid, but safety is safety
		if (!UMounteaDialogueSystemBFC::IsContextValid(TempContext) ) return;

		if (!IsFirstTime()) return;

		const auto NewRow = UMounteaDialogueSystemBFC::FindDialogueRow(DataTable, RowName);
	
		TempContext->UpdateActiveDialogueRow( UMounteaDialogueSystemBFC::FindDialogueRow(DataTable, RowName) );
	}
}

#undef LOCTEXT_NAMESPACE