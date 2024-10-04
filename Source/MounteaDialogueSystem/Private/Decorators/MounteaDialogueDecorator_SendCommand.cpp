// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Decorators/MounteaDialogueDecorator_SendCommand.h"

#include "Interfaces/MounteaDialogueParticipantInterface.h"

void UMounteaDialogueDecorator_SendCommand::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();
}

bool UMounteaDialogueDecorator_SendCommand::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	return Super::ValidateDecorator_Implementation(ValidationMessages);
}

void UMounteaDialogueDecorator_SendCommand::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (!OwningManager) return;
	if (!OwnerParticipant) return;

	OwnerParticipant->Execute_ProcessDialogueCommand(OwnerParticipant.GetObject(), Command, OptionalPayload);
}
