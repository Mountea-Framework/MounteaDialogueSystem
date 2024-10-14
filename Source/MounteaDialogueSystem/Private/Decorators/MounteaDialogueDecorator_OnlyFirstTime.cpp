// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OnlyFirstTime.h"

#include "Data/MounteaDialogueContext.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_OnlyFirstTime"

void UMounteaDialogueDecorator_OnlyFirstTime::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
}

bool UMounteaDialogueDecorator_OnlyFirstTime::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetDecoratorName();
	
	if (GetOwningNode() && GetOwningNode()->IsA(UMounteaDialogueGraphNode_StartNode::StaticClass()))
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(
			LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_StartNode",
				"Decorator {0}: is not allowed for Start Nodes!\nAttach this decorator to subsequent nodes instead."),
				Name);
		ValidationMessages.Add(TempText);
	}
	
	const auto* OwningNode = GetOwningNode();
	const auto* ParentGraph = OwningNode ? OwningNode->Graph : nullptr;
	const UMounteaDialogueGraphNode* startNode = ParentGraph ? ParentGraph->StartNode : nullptr;

	if (startNode && startNode->ChildrenNodes.IsValidIndex(0))
	{
		if (OwningNode == startNode->ChildrenNodes[0])
		{
			bSatisfied = false;
			const FText TempText = FText::Format(
				LOCTEXT("MounteaDialogueDecorator_OnlyFirstTime_Validation03",
				"Decorator {0}: is not allowed for the first dialogue Node after the Start node!\nAttach this decorator to subsequent nodes instead."),
				Name);
			ValidationMessages.Add(TempText);
		}
	}

	return bSatisfied;
}

bool UMounteaDialogueDecorator_OnlyFirstTime::EvaluateDecorator_Implementation()
{
	bool bSatisfied = Super::EvaluateDecorator_Implementation();

	if (!OwningManager)
	{
		return false;
	}
	// Let's return BP Updatable Context rather than Raw
	if (!Context)
	{
		Context = OwningManager->GetDialogueContext();
	}

	// We can live for a moment without Context, because this Decorator might be called before Context is initialized
	bSatisfied = GetOwnerParticipant() != nullptr  || Context != nullptr;
	if (!bSatisfied)
	{
		return bSatisfied;
	}
	
	return IsFirstTime();
}

void UMounteaDialogueDecorator_OnlyFirstTime::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();
	
	if (!OwningManager) return;

	// Let's return BP Updatable Context rather than Raw
	if (!Context) Context = OwningManager->GetDialogueContext();
}

bool UMounteaDialogueDecorator_OnlyFirstTime::IsFirstTime() const
{
	if (!GetOwningNode()) return false;

	TScriptInterface<IMounteaDialogueParticipantInterface> ParticipantInterface = GetOwnerParticipant();
	if (Context)
	{
		ParticipantInterface = Context->GetDialogueParticipant();
	}

	return !UMounteaDialogueSystemBFC::HasNodeBeenTraversed(GetOwningNode(), ParticipantInterface) || UMounteaDialogueSystemBFC::HasNodeBeenTraversedV2(GetOwningNode(), Context);
}

#undef LOCTEXT_NAMESPACE
