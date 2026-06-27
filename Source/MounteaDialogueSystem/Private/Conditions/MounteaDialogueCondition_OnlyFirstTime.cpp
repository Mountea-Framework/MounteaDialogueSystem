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

#include "Conditions/MounteaDialogueCondition_OnlyFirstTime.h"

#include "Components/MounteaDialogueSession.h"
#include "Data/MounteaDialogueContext.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"
#include "Nodes/MounteaDialogueGraphNode.h"

namespace MounteaDialogueConditionOnlyFirstTime
{
	bool HasTraversedNode(
		const TArray<FDialogueTraversePath>& TraversedPath,
		const UMounteaDialogueGraphNode* TargetNode,
		const UMounteaDialogueGraph* TargetGraph)
	{
		if (!IsValid(TargetNode))
			return false;

		const FGuid targetNodeGuid = TargetNode->GetNodeGUID();
		const FGuid targetGraphGuid = IsValid(TargetGraph) ? TargetGraph->GetGraphGUID() : TargetNode->GetGraphGUID();

		if (!targetNodeGuid.IsValid() || !targetGraphGuid.IsValid())
			return false;

		const FDialogueTraversePath* foundEntry = TraversedPath.FindByPredicate(
			[&](const FDialogueTraversePath& Entry)
			{
				return Entry.NodeGuid == targetNodeGuid && Entry.GraphGuid == targetGraphGuid;
			});

		return foundEntry != nullptr;
	}

	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> GetParticipantsFromContextObject(const UObject* ContextObject)
	{
		if (const auto* dialogueContext = Cast<UMounteaDialogueContext>(ContextObject))
			return dialogueContext->DialogueParticipants;

		if (const auto* dialogueSession = Cast<UMounteaDialogueSession>(ContextObject))
			return dialogueSession->GetContextPayload().DialogueParticipants;

		return TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>();
	}
}

UMounteaDialogueCondition_OnlyFirstTime::UMounteaDialogueCondition_OnlyFirstTime()
{
	ConditionName = TEXT("OnlyFirstTime");
}

bool UMounteaDialogueCondition_OnlyFirstTime::EvaluateCondition_Implementation(const TScriptInterface<IMounteaDialogueConditionContextInterface>& Context) const
{
	if (!Context.GetObject() || !Context.GetInterface())
		return true;

	const UMounteaDialogueGraphEdge* owningEdge = GetTypedOuter<UMounteaDialogueGraphEdge>();
	if (!IsValid(owningEdge) || !IsValid(owningEdge->EndNode))
		return true;

	const TArray<FDialogueTraversePath> conditionPath =
		IMounteaDialogueConditionContextInterface::Execute_GetConditionTraversedPath(Context.GetObject());
	if (MounteaDialogueConditionOnlyFirstTime::HasTraversedNode(conditionPath, owningEdge->EndNode, owningEdge->Graph))
		return false;

	TScriptInterface<IMounteaDialogueParticipantInterface> participantToCheck;
	const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> participants =
		MounteaDialogueConditionOnlyFirstTime::GetParticipantsFromContextObject(Context.GetObject());
	participantToCheck = UMounteaDialogueParticipantStatics::GetGraphOwnerParticipant(participants);

	if (!participantToCheck.GetObject() || !participantToCheck.GetInterface())
	{
		participantToCheck = IMounteaDialogueConditionContextInterface::Execute_GetConditionActiveParticipant(Context.GetObject());
	}

	if (participantToCheck.GetObject() && participantToCheck.GetInterface())
	{
		const TArray<FDialogueTraversePath> participantPath = UMounteaDialogueParticipantStatics::GetTraversedPath(participantToCheck);
		if (MounteaDialogueConditionOnlyFirstTime::HasTraversedNode(participantPath, owningEdge->EndNode, owningEdge->Graph))
			return false;
	}

	return true;
}
