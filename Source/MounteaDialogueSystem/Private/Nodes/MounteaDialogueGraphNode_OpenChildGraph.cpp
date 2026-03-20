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

#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_OpenChildGraph"

UMounteaDialogueGraphNode_OpenChildGraph::UMounteaDialogueGraphNode_OpenChildGraph()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_OpenChildGraph_Title", "Open Child Graph");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_OpenChildGraph_InternalTitle", "Open Child Graph");

#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_OpenChildGraph_ContextMenuName", "Open Child Graph");
	EditorNodeColour = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("06b6d4")));
	EditorHeaderForegroundColour = FLinearColor::White;
	bAllowOutputNodes = false;
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_OpenChildGraph_Tooltip", "* Placeholder node for opening another dialogue graph.\n* Runtime behavior is currently terminal-safe and non-destructive.");
#endif

	bAutoStarts = true;
	MaxChildrenNodes = 0;
	AllowedInputClasses.Add(UMounteaDialogueGraphNode::StaticClass());
}

void UMounteaDialogueGraphNode_OpenChildGraph::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	Super::ProcessNode_Implementation(Manager);

	if (!Manager || !Manager.GetObject())
	{
		LOG_WARNING(TEXT("[Open Child Graph Node] Invalid Dialogue Manager, node cannot finalize processing."));
		return;
	}

	LOG_WARNING(TEXT("[Open Child Graph Node] Not implemented yet. Node will finalize as terminal placeholder."));
	Manager->Execute_NodeProcessed(Manager.GetObject());
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_OpenChildGraph::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_OpenChildGraph_Description", "Open another dialogue in this project.");
}

FText UMounteaDialogueGraphNode_OpenChildGraph::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_OpenChildGraph_Category", "Utility Nodes");
}

#endif

#undef LOCTEXT_NAMESPACE
