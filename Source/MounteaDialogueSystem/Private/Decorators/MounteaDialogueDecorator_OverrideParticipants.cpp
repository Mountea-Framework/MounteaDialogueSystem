// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideParticipants.h"

#include "Helpers/MounteaDialogueSystemBFC.h"

void UMounteaDialogueDecorator_OverrideParticipants::InitializeDecorator_Implementation(UWorld* World)
{
	Super::InitializeDecorator_Implementation(World);

	if (World)
	{
		Manager = UMounteaDialogueSystemBFC::GetDialogueManager(GetOwningWorld());
		
		// Let's return BP Updatable Context rather than Raw
		Context = Manager->GetDialogueContext();
	}
}

void UMounteaDialogueDecorator_OverrideParticipants::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
	Manager = nullptr;
}

bool UMounteaDialogueDecorator_OverrideParticipants::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied =  Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = FText::FromString(GetName());

	return bSatisfied;
}

void UMounteaDialogueDecorator_OverrideParticipants::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	// We assume Context and Manager are already valid, but safety is safety
	if (!Context|| !Manager.GetInterface() || !UMounteaDialogueSystemBFC::IsContextValid(Context) ) return;
	
	if (bOverridePlayerParticipant)
	{
		//TODO
	}

	if (bOverrideDialogueParticipant)
	{
		//TODO
	}
	
	if (bOverrideActiveParticipant)
	{
		//TODO
	}
}
