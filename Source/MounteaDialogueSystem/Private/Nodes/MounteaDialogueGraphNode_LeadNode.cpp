// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

#include "Data/MounteaDialogueContext.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_LeadNode"

UMounteaDialogueGraphNode_LeadNode::UMounteaDialogueGraphNode_LeadNode()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_LeadNodeBaseTitle", "Lead Node");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_LeadNodeBaseInternalTitle", "Lead Node");
#if WITH_EDITORONLY_DATA	
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_LeadNodeContextMenuName", "Lead Node");
	
	BackgroundColor = FLinearColor(FColor::Orange);
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_LeadTooltip", "* This Node represents NPC lines.\n* This Node starts automatically upon reaching in the Dialogue Tree.\n* Requires Dialogue Data Table to work properly.");
#endif
	
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());
	AllowedInputClasses.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass());

	bAutoStarts = true;
	bUseGameplayTags = true;
}

void UMounteaDialogueGraphNode_LeadNode::PreProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (!bUseGameplayTags)
	{
		// Switch Active Participant to NPC
		if (Manager.GetInterface())
		{
			if (const auto TempContext = Manager->Execute_GetDialogueContext(Manager.GetObject()))
			{
				TempContext->SetActiveDialogueParticipant(TempContext->GetDialogueParticipant());
			}
		}
	}

	Super::PreProcessNode_Implementation(Manager);
}

void UMounteaDialogueGraphNode_LeadNode::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	Super::ProcessNode_Implementation(Manager);
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode_LeadNode::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_LeadNodeDescription", "Lead Node is a Node which usually a contains NPC parts of the Dialogue.");
}

FText UMounteaDialogueGraphNode_LeadNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_LeadNodeCategory", "Mountea Dialogue Branche Nodes");
}

#endif

#undef LOCTEXT_NAMESPACE
