// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueParticipant.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"

UMounteaDialogueParticipant::UMounteaDialogueParticipant()
{ }

void UMounteaDialogueParticipant::BeginPlay()
{
	Super::BeginPlay();

	OnDialogueGraphChanged.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueGraphChangedEvent);
}

bool UMounteaDialogueParticipant::CanStartDialogue() const
{
	return
	Execute_CanStartDialogueEvent(this) &&
	DialogueGraph != nullptr &&
	DialogueGraph->GetStartNode() != nullptr &&
	DialogueGraph->GetStartNode()->GetChildrenNodes().Num() > 0;
}

void UMounteaDialogueParticipant::SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph)
{
	if (NewDialogueGraph != DialogueGraph)
	{
		DialogueGraph = NewDialogueGraph;

		OnDialogueGraphChanged.Broadcast(NewDialogueGraph);
	}
}