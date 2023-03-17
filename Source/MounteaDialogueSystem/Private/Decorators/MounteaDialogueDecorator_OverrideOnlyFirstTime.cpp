// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideOnlyFirstTime.h"

void UMounteaDialogueDecorator_OverrideOnlyFirstTime::InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant)
{
	Super::InitializeDecorator_Implementation(World, OwningParticipant);
}

void UMounteaDialogueDecorator_OverrideOnlyFirstTime::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();
}

bool UMounteaDialogueDecorator_OverrideOnlyFirstTime::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	return Super::ValidateDecorator_Implementation(ValidationMessages);
}

void UMounteaDialogueDecorator_OverrideOnlyFirstTime::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();
}
