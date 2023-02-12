// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueParticipant.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode.h"

UMounteaDialogueParticipant::UMounteaDialogueParticipant()
{
	
}

void UMounteaDialogueParticipant::BeginPlay()
{
	Super::BeginPlay();

	OnDialogueGraphChanged.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueGraphChangedEvent);
	OnDialogueStarted.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueStartedEvent);
	OnDialogueFinished.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueFinishedEvent);
	OnDialogueActiveNodeChanged.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueActiveNodeChangedEvent);

	OnDialogueExitRequested.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueExitRequestedEvent);
	OnDialogueSkipRequested.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueSkipRequestedEvent);
	OnDialogueSkipFinished.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueSkipFinishedEvent);
}

bool UMounteaDialogueParticipant::CanStartDialogue() const
{
	return
	Execute_CanStartDialogueEvent(this) &&
	DialogueGraph != nullptr &&
	DialogueGraph->GetStartNode() != nullptr &&
	DialogueGraph->GetStartNode()->GetChildrenNodes().Num() > 0;
}

bool UMounteaDialogueParticipant::StartDialogue()
{
	 if (Execute_StartDialogueEvent(this))
	 {
	 	UMounteaDialogueGraphNode* NodeToStartWith = UMounteaDialogueSystemBFC::GetChildrenNodeFromIndex(0, DialogueGraph->GetStartNode());
		
	 	if (!NodeToStartWith) return false;

	 	SetActiveNode(NodeToStartWith);

	 	return true;
	 }
	
	return false;
}

void UMounteaDialogueParticipant::SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph)
{
	if (NewDialogueGraph != DialogueGraph)
	{
		DialogueGraph = NewDialogueGraph;

		OnDialogueGraphChanged.Broadcast(NewDialogueGraph);
	}
}

UMounteaDialogueGraphNode* UMounteaDialogueParticipant::GetStartNode() const
{
	if(DialogueGraph)
	{
		return DialogueGraph->GetStartNode();
	}

	return nullptr;
}

UMounteaDialogueGraphNode* UMounteaDialogueParticipant::GetActiveNode() const
{
	return ActiveNode;
}

void UMounteaDialogueParticipant::SetActiveNode(UMounteaDialogueGraphNode* NewActiveNode)
{
	if(ActiveNode != NewActiveNode)
	{
		ActiveNode = NewActiveNode;

		OnDialogueActiveNodeChanged.Broadcast(NewActiveNode);
	}
}