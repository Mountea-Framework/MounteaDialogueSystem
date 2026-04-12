// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaIDialogueDecorator_SwapParticipants.h"
#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"

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
	
	Context = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(OwningManager.GetObject());

	if (!Context) return;

	const TScriptInterface<IMounteaDialogueParticipantInterface> newParticipant =
		UMounteaDialogueTraversalStatics::FindParticipantByTag(Context, NewParticipantTag);
	if (!newParticipant.GetObject() || !newParticipant.GetInterface())
		return;
	if (newParticipant == Context->ActiveDialogueParticipant)
		return;

	UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(Context, newParticipant);
	OwningManager->GetDialogueContextUpdatedEventHande().Broadcast(Context);
}

#undef LOCTEXT_NAMESPACE
