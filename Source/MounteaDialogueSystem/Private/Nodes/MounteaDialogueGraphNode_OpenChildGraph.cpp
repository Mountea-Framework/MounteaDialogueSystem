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

#include "Data/MounteaDialogueContext.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Misc/DataValidation.h"

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
		LOG_WARNING(TEXT("[Open Child Graph Node] Invalid Dialogue Manager."));
		return;
	}

	if (!TargetDialogue.IsValid())
	{
		LOG_ERROR(TEXT("[Open Child Graph Node] No target dialogue assigned on node '%s'. Terminating."), *GetName());
		Manager->Execute_NodeProcessed(Manager.GetObject());
		return;
	}

	UMounteaDialogueGraph* childGraph = TargetDialogue.LoadSynchronous();
	if (!IsValid(childGraph))
	{
		LOG_ERROR(TEXT("[Open Child Graph Node] Failed to load target dialogue on node '%s'. Terminating."), *GetName());
		Manager->Execute_NodeProcessed(Manager.GetObject());
		return;
	}

	UMounteaDialogueContext* context = Manager->Execute_GetDialogueContext(Manager.GetObject());
	if (!IsValid(context))
	{
		LOG_ERROR(TEXT("[Open Child Graph Node] Invalid Dialogue Context."));
		Manager->Execute_NodeProcessed(Manager.GetObject());
		return;
	}

	UMounteaDialogueGraphNode* startNode = childGraph->GetStartNode();
	if (!IsValid(startNode))
	{
		LOG_ERROR(TEXT("[Open Child Graph Node] Target dialogue '%s' has no start node."), *childGraph->GetName());
		Manager->Execute_NodeProcessed(Manager.GetObject());
		return;
	}

	UMounteaDialogueGraphNode* firstNode = UMounteaDialogueSystemBFC::GetFirstChildNode(startNode);
	if (!IsValid(firstNode))
	{
		LOG_ERROR(TEXT("[Open Child Graph Node] Target dialogue '%s' start node has no children."), *childGraph->GetName());
		Manager->Execute_NodeProcessed(Manager.GetObject());
		return;
	}

	TArray<UMounteaDialogueGraphNode*> allowedChildren = UMounteaDialogueSystemBFC::GetAllowedChildNodes(firstNode);
	context->SetDialogueContext(firstNode, allowedChildren);
	context->UpdateActiveDialogueRow(UMounteaDialogueSystemBFC::GetSpeechData(firstNode));
	context->UpdateActiveDialogueRowDataIndex(0);

	Manager->Execute_PrepareNode(Manager.GetObject());
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

bool UMounteaDialogueGraphNode_OpenChildGraph::ValidateTargetDialogueReference(FDataValidationContext& Context, const bool RichFormat) const
{
	if (!IsValid(Graph))
		return true;

	UMounteaDialogueGraph* parentGraph = Graph;
	UMounteaDialogueGraph* rootTargetGraph = TargetDialogue.LoadSynchronous();
	if (!IsValid(rootTargetGraph))
		return true;

	TSet<const UMounteaDialogueGraph*> visitedGraphs;
	TArray<UMounteaDialogueGraph*> graphsToVisit;
	graphsToVisit.Add(rootTargetGraph);

	while (graphsToVisit.Num() > 0)
	{
		UMounteaDialogueGraph* currentGraph = graphsToVisit.Pop(EAllowShrinking::No);
		if (!IsValid(currentGraph) || visitedGraphs.Contains(currentGraph))
			continue;
		visitedGraphs.Add(currentGraph);

		const TArray<UMounteaDialogueGraphNode*> currentNodes = currentGraph->GetAllNodes();
		for (const auto* Node : currentNodes)
		{
			const auto* OpenChildNode = Cast<UMounteaDialogueGraphNode_OpenChildGraph>(Node);
			if (!IsValid(OpenChildNode))
				continue;

			UMounteaDialogueGraph* nextGraph = OpenChildNode->TargetDialogue.LoadSynchronous();
			if (!IsValid(nextGraph))
				continue;

			if (nextGraph == parentGraph)
			{
				const FString currentGraphName = parentGraph->GetName();
				const FString targetGraphName = rootTargetGraph->GetName();
				const FString richTextReturn =
				FString(TEXT("* ")).
				Append(TEXT("<RichTextBlock.Bold>")).
				Append(NodeTitle.ToString()).
				Append(TEXT("</>: Invalid child graph reference. Target graph ")).
				Append(TEXT("<RichTextBlock.Bold>")).
				Append(targetGraphName).
				Append(TEXT("</> creates a recursive reference back to parent graph ")).
				Append(TEXT("<RichTextBlock.Bold>")).
				Append(currentGraphName).
				Append(TEXT("</>."));

				const FString textReturn =
				FString(NodeTitle.ToString()).
				Append(TEXT(": Invalid child graph reference. Target graph ")).
				Append(targetGraphName).
				Append(TEXT(" creates a recursive reference back to parent graph ")).
				Append(currentGraphName).
				Append(TEXT("."));

				Context.AddError(FText::FromString(RichFormat ? richTextReturn : textReturn));
				return false;
			}

			if (!visitedGraphs.Contains(nextGraph))
				graphsToVisit.Add(nextGraph);
		}
	}

	return true;
}

bool UMounteaDialogueGraphNode_OpenChildGraph::ValidateNode(FDataValidationContext& Context, const bool RichFormat) const
{
	bool bSatisfied = Super::ValidateNode(Context, RichFormat);
	return ValidateTargetDialogueReference(Context, RichFormat) && bSatisfied;
}

void UMounteaDialogueGraphNode_OpenChildGraph::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property
		&& PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_OpenChildGraph, TargetDialogue))
	{
		if (IsValid(TargetDialogue.LoadSynchronous()))
		{
			FDataValidationContext validationContext;
			if (!ValidateTargetDialogueReference(validationContext, false))
			{
				TargetDialogue = nullptr;
				LOG_ERROR(TEXT("[Open Child Graph Node] Invalid child graph reference on node '%s'. Target graph would create recursive graph dependency."), *GetName());
			}
		}
	}
}

#endif

#undef LOCTEXT_NAMESPACE
