// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_SelectRandomDialogueRow.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_SelectRandomDialogueRow"

void UMounteaDialogueDecorator_SelectRandomDialogueRow::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
	OwningManager = nullptr;
}

bool UMounteaDialogueDecorator_SelectRandomDialogueRow::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetDecoratorName();

	if (!GetOwningNode())
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_SelectRandomDialogueRow_Validation", "Decorator {0}: is not allowed in Graph Decorators!\nAttach this Decorator to Node instead."), Name);
		ValidationMessages.Add(TempText);
	}

	return bSatisfied;
}

void UMounteaDialogueDecorator_SelectRandomDialogueRow::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (!OwningManager) return;
	if (!Context) Context = OwningManager->GetDialogueContext();
	if (!Context)
	{
		LOG_ERROR(TEXT("[ExecuteDecorator] %s Has no Context!\nExecution is skipped."), *(GetDecoratorName().ToString()));
		return;
	}

	if (!Context->GetActiveDialogueRow().IsValid())
	{
		LOG_WARNING(TEXT("[ExecuteDecorator] %s ActiveDialogueRow is invalid!!\nExecution is skipped."), *(GetDecoratorName().ToString()));
		return;
	}
	if (Context->GetActiveDialogueRow().DialogueRowData.Num() == 0)
	{
		LOG_WARNING(TEXT("[ExecuteDecorator] %s DialogueRowData is empty!\nExecution is skipped."), *(GetDecoratorName().ToString()));
		return;
	}
	
	FIntPoint ClampedRange;
	if (RandomRange.X > RandomRange.Y)
	{
		ClampedRange.X = RandomRange.Y;
		ClampedRange.Y = RandomRange.X;
	}
	else
	{
		ClampedRange = RandomRange;
	}

	const int32 MaxValue = Context->GetActiveDialogueRow().DialogueRowData.Num() - 1;
	const int32 Range = FMath::RandRange
	(
		FMath::Max(0, ClampedRange.X),
		FMath::Min(MaxValue, ClampedRange.Y)
	);

	Context->UpdateActiveDialogueRowDataIndex(Range);
}


#undef LOCTEXT_NAMESPACE