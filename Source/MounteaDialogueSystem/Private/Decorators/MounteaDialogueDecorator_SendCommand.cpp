// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Decorators/MounteaDialogueDecorator_SendCommand.h"

#include "Interfaces/MounteaDialogueParticipantInterface.h"

#define LOCTEXT_NAMESPACE "UMounteaDialogueDecorator_SendCommand"

void UMounteaDialogueDecorator_SendCommand::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();
}

bool UMounteaDialogueDecorator_SendCommand::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied =  Super::ValidateDecorator_Implementation(ValidationMessages);

	const FText Name = GetDecoratorName();

	if (Command.IsEmpty())
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_SendCommand_Validation", "Decorator {0}: StringCommand is empty! Sending Command would fail."), Name);
		ValidationMessages.Add(TempText);
	}
	
	return bSatisfied;
}

void UMounteaDialogueDecorator_SendCommand::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (!OwningManager) return;
	if (!OwnerParticipant) return;

	OwnerParticipant->Execute_ProcessDialogueCommand(OwnerParticipant.GetObject(), Command, OptionalPayload);
}

#undef LOCTEXT_NAMESPACE