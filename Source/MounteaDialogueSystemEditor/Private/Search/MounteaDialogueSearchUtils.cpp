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

#include "MounteaDialogueSearchUtils.h"

#include "AssetEditor/AssetEditor_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Graph/MounteaDialogueGraph.h"

TSharedPtr<SDockTab> FMounteaDialogueSearchHelpers::InvokeTab(TSharedPtr<FTabManager> TabManager, const FTabId& TabID)
{
	if (!TabManager.IsValid())
	{
		return nullptr;
	}

	return TabManager->TryInvokeTab(TabID);
}