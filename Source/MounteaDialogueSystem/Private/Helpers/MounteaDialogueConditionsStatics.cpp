// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools


#include "Helpers/MounteaDialogueConditionsStatics.h"

#include "Conditions/MounteaDialogueConditionBase.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Helpers/MounteaDialogueSystemConsts.h"


bool UMounteaDialogueConditionsStatics::EvaluateCondition(UMounteaDialogueConditionBase* Condition, const TScriptInterface<IMounteaDialogueConditionContextInterface>& Context)
{
	return IsValid(Condition) ? Condition->EvaluateCondition(Context) : false;
}

FString UMounteaDialogueConditionsStatics::GetConditionName(UMounteaDialogueConditionBase* Condition)
{
	return IsValid(Condition) ? Condition->GetConditionName() : TEXT("none");
}

FString UMounteaDialogueConditionsStatics::GetConditionDocumentationLink(UMounteaDialogueConditionBase* Condition)
{
	return IsValid(Condition) ? Condition->GetConditionDocumentationLink() : MounteaDialogueSystemConsts::DialogueDocumentationLink;
}

bool UMounteaDialogueConditionsStatics::EvaluateEdgeConditions(const UMounteaDialogueGraphEdge* Edge, const TScriptInterface<IMounteaDialogueConditionContextInterface>& Context)
{
	if (!IsValid(Edge))
		return true;

	const FMounteaDialogueEdgeConditions& conds = Edge->GetEdgeConditions();
	if (conds.Rules.IsEmpty())
		return true;

	const bool bAll = (conds.Mode == EConditionEvaluationMode::All);
	for (const FMounteaDialogueCondition& rule : conds.Rules)
	{
		if (!IsValid(rule.ConditionClass))
			continue;

		bool bResult = rule.ConditionClass->EvaluateCondition(Context);
		if (rule.bNegate)
			bResult = !bResult;

		if (bAll && !bResult)
			return false;

		if (!bAll && bResult)
			return true;
	}

	return bAll;
}
