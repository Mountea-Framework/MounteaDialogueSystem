// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideParticipants.h"

#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_OverrideParticipants"

void UMounteaDialogueDecorator_OverrideParticipants::InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager)
{
	Super::InitializeDecorator_Implementation(World, OwningParticipant, NewOwningManager);

	if (World)
	{
		OwningManager = NewOwningManager;

		// Keep in mind that override cannot override nulls!
		if (bOverridePlayerParticipant)
		{
			Override_PlayerParticipantInterface = GetParticipantFromActorRef(NewPlayerParticipant);
		}
		if (bOverrideDialogueParticipant)
		{
			Override_ParticipantInterface = GetParticipantFromActorRef(NewDialogueParticipant);
		}
		if (bOverrideActiveParticipant)
		{
			Override_ActiveParticipantInterface = GetParticipantFromActorRef(NewActiveParticipant);
		}
	}
}

void UMounteaDialogueDecorator_OverrideParticipants::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
	OwningManager = nullptr;

	Override_PlayerParticipantInterface = nullptr;
	Override_ParticipantInterface = nullptr;
	Override_ActiveParticipantInterface = nullptr;
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
	
	if (!OwningManager) return;
	
	// Let's return BP Updatable Context rather than Raw
	Context = OwningManager->GetDialogueContext();

	// We assume Context and Manager are already valid, but safety is safety
	if (!Context|| !OwningManager.GetInterface() || !UMounteaDialogueSystemBFC::IsContextValid(Context) ) return;
	
	if (bOverridePlayerParticipant)
	{
		Context->UpdateDialoguePlayerParticipant(Override_PlayerParticipantInterface);
	}

	if (bOverrideDialogueParticipant)
	{
		Context->UpdateDialogueParticipant(Override_ParticipantInterface);
	}
	
	if (bOverrideActiveParticipant)
	{
		Context->UpdateActiveDialogueParticipant(Override_ActiveParticipantInterface);
	}
}

bool UMounteaDialogueDecorator_OverrideParticipants::ValidateInterfaceActor(const TSoftObjectPtr<AActor> Actor, TArray<FText>& ValidationMessages) const
{
	// Let's assume Class is valid
	const FText Name = GetDecoratorName();
	bool bSatisfied = true;

	const TScriptInterface<IMounteaDialogueParticipantInterface> ParticipantInterface = GetParticipantFromActorRef(Actor);
	if (ParticipantInterface == nullptr || ParticipantInterface.GetObject() == nullptr || ParticipantInterface.GetInterface() == nullptr)
	{
		const FText TempText = Actor.IsNull() ?
			FText::Format
			(
				LOCTEXT("MounteaDialogueDecorator_OverrideParticipants_ValidateInterfaceActorNoActor", "Decorator {0}: Actor reference is Empty!"),
				Name
			) :
			FText::Format
			(
				LOCTEXT("MounteaDialogueDecorator_OverrideParticipants_ValidateInterfaceActorInvalidActor", "Decorator {0}: Actor {1} does not Implement 'MounteaDialogueParticipantInterface' nor has any 'MounteaDialogueParticipantInterface' Components!"),
				Name, FText::FromString(Actor->GetName())
			);
				
		ValidationMessages.Add(TempText);
		bSatisfied = false;
	}

	return bSatisfied;
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueDecorator_OverrideParticipants::GetParticipantFromActorRef(const TSoftObjectPtr<AActor>& Actor) const
{
	TScriptInterface<IMounteaDialogueParticipantInterface> ReturnValue = nullptr;

	if (!Actor.IsValid()) return ReturnValue;

	if (Actor->Implements<UMounteaDialogueParticipantInterface>())
	{
		ReturnValue = Actor.Get();
		ReturnValue.SetObject(Actor.Get());
		ReturnValue.SetInterface(Cast<IMounteaDialogueParticipantInterface>(Actor.Get()));

		return ReturnValue;
	}

	// Get All Components and return first found
	TArray<UActorComponent*> ActorComponents =	Actor->GetComponentsByInterface(UMounteaDialogueParticipantInterface::StaticClass());
	for (const auto& Itr : ActorComponents)
	{
		if (Itr && Itr->Implements<UMounteaDialogueParticipantInterface>())
		{
			ReturnValue = Itr;
			ReturnValue.SetObject(Itr);
			ReturnValue.SetInterface(Cast<IMounteaDialogueParticipantInterface>(Itr));

			return ReturnValue;
		}
	}
	
	return ReturnValue;
}

#if WITH_EDITOR
void UMounteaDialogueDecorator_OverrideParticipants::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueDecorator_OverrideParticipants, NewPlayerParticipant))
	{
		if (!bOverridePlayerParticipant) NewPlayerParticipant = nullptr;
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueDecorator_OverrideParticipants, NewDialogueParticipant))
	{
		if (!bOverrideDialogueParticipant) NewDialogueParticipant = nullptr;
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueDecorator_OverrideParticipants, NewActiveParticipant))
	{
		if (!bOverrideActiveParticipant) NewActiveParticipant = nullptr;
	}
}
#endif

#undef LOCTEXT_NAMESPACE
