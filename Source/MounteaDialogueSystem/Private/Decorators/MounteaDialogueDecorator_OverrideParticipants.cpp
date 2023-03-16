// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideParticipants.h"

#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE ""

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

	if (bOverridePlayerParticipant)
	{
		bSatisfied = ValidateInterfaceActor(NewPlayerParticipant, ValidationMessages);
	}
	if (bOverrideDialogueParticipant)
	{
		bSatisfied = ValidateInterfaceActor(NewDialogueParticipant, ValidationMessages);
	}
	if (bOverrideActiveParticipant)
	{
		bSatisfied = ValidateInterfaceActor(NewActiveParticipant, ValidationMessages);
	}

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

bool UMounteaDialogueDecorator_OverrideParticipants::ValidateInterfaceActor(const TSoftObjectPtr<AActor> Actor, TArray<FText>& ValidationMessages) const
{
	const FText Name = FText::FromString(GetName());
	bool bSatisfied = true;
	
	if (Actor.IsValid())
	{
		if (!Actor->Implements<UMounteaDialogueParticipantInterface>())
		{
			const TArray<UActorComponent*> Comps;
			Actor->GetComponentsByInterface(UMounteaDialogueParticipantInterface::StaticClass());

			if (Comps.Num() == 0)
			{
				const FText TempText = FText::Format
				(
					LOCTEXT("MounteaDialogueDecorator_OverrideParticipants_ValidateInterfaceActor", "{0}: Actor {1} does not Implement 'MounteaDialogueParticipantInterface' nor has any 'MounteaDialogueParticipantInterface' Components!"),
					Name, FText::FromString(Actor->GetName())
				);
				
				ValidationMessages.Add(TempText);
				bSatisfied = false;
			}
		}
	}

	return bSatisfied;
}

#undef LOCTEXT_NAMESPACE
