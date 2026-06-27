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

#include "Helpers/MounteaDialogueContextStatics.h"

#include "Data/MounteaDialogueContext.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"

bool UMounteaDialogueContextStatics::IsContextValid(const UMounteaDialogueContext* Context)
{
	return IsValid(Context) && Context->IsValid();
}

UMounteaDialogueGraphNode* UMounteaDialogueContextStatics::FindNodeByGUID(const UMounteaDialogueGraph* FromGraph, const FGuid ByGUID)
{
	if (!IsValid(FromGraph) || !ByGUID.IsValid())
		return nullptr;

	const TArray<UMounteaDialogueGraphNode*> allNodes = FromGraph->GetAllNodes();
	for (UMounteaDialogueGraphNode* node : allNodes)
	{
		if (IsValid(node) && node->GetNodeGUID() == ByGUID)
			return node;
	}

	return nullptr;
}

TArray<UMounteaDialogueGraphNode*> UMounteaDialogueContextStatics::FindNodesByGUID(const UMounteaDialogueGraph* FromGraph, const TArray<FGuid>& Guids)
{
	TArray<UMounteaDialogueGraphNode*> result;
	for (const FGuid& guid : Guids)
	{
		if (UMounteaDialogueGraphNode* node = FindNodeByGUID(FromGraph, guid))
			result.Add(node);
	}

	return result;
}
