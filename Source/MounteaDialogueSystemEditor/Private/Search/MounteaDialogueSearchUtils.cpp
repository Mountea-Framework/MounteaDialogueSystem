// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "MounteaDialogueSearchUtils.h"

#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Toolkits/AssetEditorManager.h"

bool FMounteaDialogueSearchUtils::OpenEditorAndJumpToGraphNode(const UEdGraphNode* GraphNode, bool bFocusIfOpen)
{
	if (!IsValid(GraphNode))
	{
		return false;
	}

	// Open if not already.
	UEdGraph_MounteaDialogueGraph* Dialogue = GetDialogueFromGraphNode(GraphNode);
	if (!OpenEditorForAsset(Dialogue))
	{
		return false;
	}

	// Could still fail focus on the graph node
	if (IAssetEditorInstance* EditorInstance = FindEditorForAsset(Dialogue, bFocusIfOpen))
	{
		EditorInstance->FocusWindow(const_cast<UEdGraphNode*>(GraphNode));
		return true;
	}

	return false;
}

IAssetEditorInstance* FMounteaDialogueSearchUtils::FindEditorForAsset(UObject* Asset, bool bFocusIfOpen)
{
	if (!IsValid(Asset) || !GEditor)
	{
		return nullptr;
	}

	return GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Asset, bFocusIfOpen);
}

bool FMounteaDialogueSearchUtils::OpenEditorForAsset(const UObject* Asset)
{
	if (!IsValid(Asset) || !GEditor)
	{
		return false;
	}

	return GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(const_cast<UObject*>(Asset));
}


UEdGraph_MounteaDialogueGraph* FMounteaDialogueSearchUtils::GetDialogueFromGraphNode(const UEdGraphNode* GraphNode)
{
	if (const UEdNode_MounteaDialogueGraphNode* DialogueBaseNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		return DialogueBaseNode->GetDialogueGraphEdGraph();
	}

	// Last change
	if (const UMounteaDialogueGraph* DialogueGraph = Cast<UMounteaDialogueGraph>(GraphNode->GetGraph()))
	{
		return Cast<UEdGraph_MounteaDialogueGraph>(DialogueGraph->EdGraph);
	}

	return nullptr;
}

TSharedPtr<SDockTab> FMounteaDialogueSearchHelpers::InvokeTab(TSharedPtr<FTabManager> TabManager, const FTabId& TabID)
{
	if (!TabManager.IsValid())
	{
		return nullptr;
	}

	return TabManager->TryInvokeTab(TabID);
}