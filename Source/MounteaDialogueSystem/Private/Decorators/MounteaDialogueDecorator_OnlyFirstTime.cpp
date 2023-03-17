// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OnlyFirstTime.h"

#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_OnlyFirstTime"

void UMounteaDialogueDecorator_OnlyFirstTime::InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant)
{
	Super::InitializeDecorator_Implementation(World, OwningParticipant);

	if (World)
	{
		Manager = UMounteaDialogueSystemBFC::GetDialogueManager(GetOwningWorld());
		Context = Manager->GetDialogueContext();
	}
}

void UMounteaDialogueDecorator_OnlyFirstTime::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
	Manager = nullptr;
}

bool UMounteaDialogueDecorator_OnlyFirstTime::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetClass()->GetDisplayNameText();

	if (!GetOwningNode())
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OnlyFirstTime_Validation01", "Decorator {0}: is not allowed in Graph Decorators!\nAttach this Decorator to Node instead."), Name);
		ValidationMessages.Add(TempText);
	}

	if (GetOwningNode()->IsA(UMounteaDialogueGraphNode_StartNode::StaticClass()))
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OnlyFirstTime_Validation02", "Decorator {0}: is not allowed for Start Nodes!!\nAttach this Decorator to following Nodes instead."), Name);
		ValidationMessages.Add(TempText);
	}

	return bSatisfied;
}

bool UMounteaDialogueDecorator_OnlyFirstTime::EvaluateDecorator_Implementation()
{
	bool bSatisfied = Super::EvaluateDecorator_Implementation();
	// Let's return BP Updatable Context rather than Raw
	if (!Context)
	{
		Context = Manager->GetDialogueContext();
	}

	// We can live for a moment without Context, because this Decorator might be called before Context is initialized
	bSatisfied = GetOwnerParticipant() != nullptr  || Context != nullptr;
	
	return bSatisfied;
}

void UMounteaDialogueDecorator_OnlyFirstTime::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	// Let's return BP Updatable Context rather than Raw
	if (!Context) Context = Manager->GetDialogueContext();
}

bool UMounteaDialogueDecorator_OnlyFirstTime::IsFirstTime() const
{
	if (!GetOwningNode()) return false;

	TScriptInterface<IMounteaDialogueParticipantInterface> ParticipantInterface = GetOwnerParticipant();
	if (Context)
	{
		ParticipantInterface = Context->GetDialogueParticipant();
	}

	return !UMounteaDialogueSystemBFC::HasNodeBeenTraversed(GetOwningNode(), ParticipantInterface);
}

#undef LOCTEXT_NAMESPACE
