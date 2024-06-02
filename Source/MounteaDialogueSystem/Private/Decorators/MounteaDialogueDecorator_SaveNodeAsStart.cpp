// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_SaveNodeAsStart.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_SaveNodeAsStart"

void UMounteaDialogueDecorator_SaveNodeAsStart::InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager)
{
	Super::InitializeDecorator_Implementation(World, OwningParticipant, nullptr);
}

void UMounteaDialogueDecorator_SaveNodeAsStart::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
	OwningManager = nullptr;
}

bool UMounteaDialogueDecorator_SaveNodeAsStart::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetDecoratorName();

	if (!GetOwningNode())
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_SaveNodeAsStart_Validation", "Decorator {0}: is not allowed in Graph Decorators!\nAttach this Decorator to Node instead."), Name);
		ValidationMessages.Add(TempText);
	}

	return bSatisfied;
}

void UMounteaDialogueDecorator_SaveNodeAsStart::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (!OwningManager) return;

	// Let's return BP Updatable Context rather than Raw
	Context = OwningManager->GetDialogueContext();

	if (Context)
	{
		const auto Participant = Context->GetDialogueParticipant();
		Participant->Execute_SaveStartingNode(Participant.GetObject(), GetOwningNode());
	}
}

#undef LOCTEXT_NAMESPACE