// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "Ed/EdGraph_MounteaDialogueGraph.h"

class UEdNode_MounteaDialogueGraphNode;

class FMounteaDialogueSearchUtils
{
public:
	
	static bool OpenEditorAndJumpToGraphNode(const UEdGraphNode* GraphNode, bool bFocusIfOpen = false);
	
	static UEdGraph_MounteaDialogueGraph* GetDialogueFromGraphNode(const UEdGraphNode* GraphNode);
	static IAssetEditorInstance* FindEditorForAsset(UObject* Asset, bool bFocusIfOpen);
	static bool OpenEditorForAsset(const UObject* Asset);
};

class MOUNTEADIALOGUESYSTEMEDITOR_API FMounteaDialogueSearchHelpers
{
public:
	static TSharedPtr<SDockTab> InvokeTab(TSharedPtr<FTabManager> TabManager, const FTabId& TabID);
};