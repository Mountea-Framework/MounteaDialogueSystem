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

	if (!Context)
	{
		// Let's return BP Updatable Context rather than Raw
		Context = OwningManager->Execute_GetDialogueContext(OwningManager.GetObject());
	}

	if (!Context) return;

	const bool bIsPlayerActive = Context->GetActiveDialogueParticipant() == Context->GetDialoguePlayerParticipant();
	
	const TScriptInterface<IMounteaDialogueParticipantInterface> NewActiveParticipant =
		bIsPlayerActive
		?
		Context->GetDialogueParticipant() :
		Context->GetDialoguePlayerParticipant();

	UMounteaDialogueSystemBFC::GetMatchingDialogueParticipant(Context, NewActiveParticipant);
}

#undef LOCTEXT_NAMESPACE