// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaIDialogueDecorator_SwapParticipants.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Data/MounteaDialogueContext.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_SwapParticipants"

void UMounteaDialogueDecorator_SwapParticipants::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
	OwningManager = nullptr;
}

void UMounteaDialogueDecorator_SwapParticipants::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (!OwningManager) return;
	
	Context = OwningManager->Execute_GetDialogueContext(OwningManager.GetObject());

	if (!Context) return;

	auto newParticipant = UMounteaDialogueSystemBFC::FindParticipantByTag(Context, NewParticipantTag);
	if (newParticipant != Context->ActiveDialogueParticipant)
		return;

	UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(Context, newParticipant);
	OwningManager->GetDialogueContextUpdatedEventHande().Broadcast(Context);
}

#undef LOCTEXT_NAMESPACE