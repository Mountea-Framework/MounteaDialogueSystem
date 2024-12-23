// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_AnswerNode"

UMounteaDialogueGraphNode_AnswerNode::UMounteaDialogueGraphNode_AnswerNode()
{
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeContextMenuName", "Answer Node");
	
	BackgroundColor = FLinearColor(FColor::Turquoise);

	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_AnswerTooltip", "* This Node represents Player's answers.\n* This Node requires Player's input to be started.\n* Requires Dialogue Data Table to work properly.");
#endif
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeTitle", "Answer Node");	
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_AnswerNodeInternalTitle", "Answer Node");
	
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());

	bAutoStarts = false;
	bUseGameplayTags = false;
}

void UMounteaDialogueGraphNode_AnswerNode::PreProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (!bUseGameplayTags)
	{
		if (Manager.GetInterface())
		{
			if (const auto TempContext = Manager->Execute_GetDialogueContext(Manager.GetObject()))
			{
				UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(TempContext, TempContext->GetDialoguePlayerParticipant());
			}
		}
	}

	Super::PreProcessNode_Implementation(Manager);
}

void UMounteaDialogueGraphNode_AnswerNode::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	Super::ProcessNode_Implementation(Manager);
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_AnswerNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AnswerNodeDescription", "Answear Node is a Node which usually contains Player parts of the Dialogue.");
}

FText UMounteaDialogueGraphNode_AnswerNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_AnswearNodeCategory", "Mountea Dialogue Branche Nodes");
}

#endif

#undef LOCTEXT_NAMESPACE
