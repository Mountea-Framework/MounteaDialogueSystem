// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "Ed/EdGraph_MounteaDialogueGraph.h"

class FAssetEditor_MounteaDialogueGraph;
class UEdNode_MounteaDialogueGraphNode;

class FMounteaDialogueSearchUtils
{
public:
	
	static bool OpenEditorAndJumpToGraphNode(TWeakPtr<FAssetEditor_MounteaDialogueGraph> DialogueEditorPtr, const UEdGraphNode* GraphNode, bool bFocusIfOpen = false);
	
	static UMounteaDialogueGraph* GetDialogueFromGraphNode(const UEdGraphNode* GraphNode);
	static IAssetEditorInstance* FindEditorForAsset(UObject* Asset, bool bFocusIfOpen);
	static bool OpenEditorForAsset(const UObject* Asset);
};

class MOUNTEADIALOGUESYSTEMEDITOR_API FMounteaDialogueSearchHelpers
{
public:
	static TSharedPtr<SDockTab> InvokeTab(TSharedPtr<FTabManager> TabManager, const FTabId& TabID);
};