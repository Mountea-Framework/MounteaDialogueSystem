// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_SaveNodeAsStart.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_SaveNodeAsStart"

void UMounteaDialogueDecorator_SaveNodeAsStart::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
	OwningManager = nullptr;
}

bool UMounteaDialogueDecorator_SaveNodeAsStart::ValidateDecorator_Implementation(UPARAM(ref) TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetDecoratorName();
	const auto* OwningNode = GetOwningNode();

	if (!GetOwningNode())
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_SaveNodeAsStart_Validation", "Decorator {0}: is not allowed in Graph Decorators!\nAttach this Decorator to Node instead."), Name);
		ValidationMessages.Add(TempText);
	}

	if (OwningNode && OwningNode->IsA(UMounteaDialogueGraphNode_ReturnToNode::StaticClass()))
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(
			LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_ReturnNode",
				"Decorator {0}: is not allowed for Return Nodes!\nAttach this decorator to different nodes instead."),
				Name);
		ValidationMessages.Add(TempText);
	}

	if (OwningNode && OwningNode->IsA(UMounteaDialogueGraphNode_CompleteNode::StaticClass()))
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(
			LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_CompleteNode",
				"Decorator {0}: is not allowed for Complete Dialogue Nodes!\nAttach this decorator to different nodes instead."),
				Name);
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