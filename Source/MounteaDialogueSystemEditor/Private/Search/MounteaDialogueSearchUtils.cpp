// Copyright Dominik Pavlicek 2023. All Rights Reserved.

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