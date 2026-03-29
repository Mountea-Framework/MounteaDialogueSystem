// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "Helpers/MounteaDialogueTraversalStatics.h"

#include "Data/MounteaDialogueContext.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Helpers/MounteaDialogueConditionsStatics.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"
#include "Interfaces/Nodes/MounteaDialogueSpeechDataInterface.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Settings/MounteaDialogueSystemSettings.h"
#include "Sound/SoundBase.h"

TArray<UMounteaDialogueGraphNode*> UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(
	const UMounteaDialogueGraphNode* ParentNode,
	const TScriptInterface<IMounteaDialogueConditionContextInterface>& ConditionContext)
{
	TArray<UMounteaDialogueGraphNode*> returnNodes;
	if (!IsValid(ParentNode))
		return returnNodes;

	for (UMounteaDialogueGraphNode* child : ParentNode->GetChildrenNodes())
	{
		if (!IsValid(child) || !child->CanStartNode())
			continue;

		const UMounteaDialogueGraphEdge* const* edgePtr = ParentNode->Edges.Find(child);
		const UMounteaDialogueGraphEdge* edge = edgePtr ? *edgePtr : nullptr;

		if (UMounteaDialogueConditionsStatics::EvaluateEdgeConditions(edge, ConditionContext))
			returnNodes.Add(child);
	}

	return returnNodes;
}

UMounteaDialogueGraphNode* UMounteaDialogueTraversalStatics::GetFirstChildNode(const UMounteaDialogueGraphNode* ParentNode)
{
	if (!IsValid(ParentNode))
		return nullptr;

	const TArray<UMounteaDialogueGraphNode*>& children = ParentNode->GetChildrenNodes();
	if (!children.IsValidIndex(0) || !IsValid(children[0]))
		return nullptr;

	return children[0]->CanStartNode() ? children[0] : nullptr;
}

void UMounteaDialogueTraversalStatics::SortNodes(TArray<UMounteaDialogueGraphNode*>& SortedNodes)
{
	SortedNodes.RemoveAll([](UMounteaDialogueGraphNode* Node)
	{
		return !IsValid(Node);
	});
	
	SortedNodes.Sort([](const UMounteaDialogueGraphNode& A, const UMounteaDialogueGraphNode& B)
	{
		return A.ExecutionOrder < B.ExecutionOrder;
	});
}

FDialogueRow UMounteaDialogueTraversalStatics::GetSpeechData(UMounteaDialogueGraphNode* Node)
{
	if (!IsValid(Node))
		return FDialogueRow();

	if (!Node->GetClass()->ImplementsInterface(UMounteaDialogueSpeechDataInterface::StaticClass()))
		return FDialogueRow();

	return IMounteaDialogueSpeechDataInterface::Execute_GetSpeechData(Node);
}

bool UMounteaDialogueTraversalStatics::IsDialogueRowValid(const FDialogueRow& Row)
{
	const bool bHasParticipant = !Row.DialogueParticipantName.IsNone() || !Row.DialogueParticipant.IsEmpty();
	return Row.RowGUID.IsValid() && bHasParticipant && Row.RowData.Num() > 0;
}

bool UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(const FDialogueRowData& Data)
{
	return Data.RowGUID.IsValid() && (!Data.RowText.IsEmpty() || !Data.RowText.EqualTo(FText::FromString("Dialogue Example")));
}

float UMounteaDialogueTraversalStatics::GetRowDuration(const FDialogueRowData& Row)
{
	float returnValue = 1.f;
	if (!IsDialogueRowDataValid(Row))
		return returnValue;

	switch (Row.RowDurationMode)
	{
	case ERowDurationMode::ERDM_Duration:
		if (Row.RowSound)
			returnValue = Row.RowSound->Duration;
		else
			returnValue = Row.RowDuration;
		break;
	case ERowDurationMode::EDRM_Override:
		returnValue = Row.RowDurationOverride;
		break;
	case ERowDurationMode::EDRM_Add:
		if (Row.RowSound)
			returnValue = Row.RowSound->Duration + Row.RowDurationOverride;
		else
			returnValue = Row.RowDurationOverride;
		break;
	case ERowDurationMode::ERDM_AutoCalculate:
		{
			const UMounteaDialogueSystemSettings* settings = GetDefault<UMounteaDialogueSystemSettings>();
			if (settings)
				returnValue = ((Row.RowText.ToString().Len() * settings->GetDurationCoefficient()) / 100.f);
			else
				returnValue = ((Row.RowText.ToString().Len() * 8.f) / 100.f);
			break;
		}
	}

	return FMath::Max(UE_KINDA_SMALL_NUMBER, returnValue);
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueTraversalStatics::ResolveActiveParticipant(const UMounteaDialogueContext* DialogueContext)
{
	if (!IsValid(DialogueContext) || !IsValid(DialogueContext->ActiveNode))
		return nullptr;

	const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& participants = DialogueContext->DialogueParticipants;
	if (participants.Num() == 0)
		return DialogueContext->ActiveDialogueParticipant;

	FGameplayTagContainer activeTags;
	if (DialogueContext->ActiveNode->IsA<UMounteaDialogueGraphNode_DialogueNodeBase>())
		activeTags.AppendTags(DialogueContext->ActiveDialogueRow.CompatibleTags);
	activeTags.AppendTags(DialogueContext->ActiveNode->NodeGameplayTags);

	if (activeTags.Num() == 0)
		return IsValid(DialogueContext->ActiveDialogueParticipant.GetObject()) ? DialogueContext->ActiveDialogueParticipant : participants[0];

	const auto* foundParticipant = participants.FindByPredicate(
		[&](const TScriptInterface<IMounteaDialogueParticipantInterface>& participant)
		{
			if (!IsValid(participant.GetObject()))
				return false;
			const FGameplayTag participantTag = IMounteaDialogueParticipantInterface::Execute_GetParticipantTag(participant.GetObject());
			return participantTag.MatchesAny(activeTags);
		});

	return (foundParticipant && *foundParticipant != DialogueContext->ActiveDialogueParticipant)
		? *foundParticipant
		: DialogueContext->ActiveDialogueParticipant;
}

bool UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(
	UMounteaDialogueContext* Context,
	const TScriptInterface<IMounteaDialogueParticipantInterface>& NewActiveParticipant)
{
	if (!IsValid(Context))
		return false;

	if (Context->ActiveDialogueParticipant == NewActiveParticipant)
		return false;

	Context->ActiveDialogueParticipant = NewActiveParticipant;
	Context->OnDialogueContextUpdated.Broadcast();
	return true;
}
