// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueParticipant.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

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
	OnDialogueNodeExecuted.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueNodeExecutedEvent);
	OnDialogueRowExecuted.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueRowExecutedEvent);
	OnDialogueNodeFinished.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueFinishedEvent);
	
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
	 	TArray<UMounteaDialogueGraphNode*> AvailableChildrenNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(DialogueGraph->GetStartNode());

	 	if (AvailableChildrenNodes.Num() == 0) return false;
	 	if (AvailableChildrenNodes.IsValidIndex(0) == false) return false;

	 	// TODO: Add ability to start Dialogue from specified Node!
	 	SetActiveNode(AvailableChildrenNodes[0]);

	 	OnDialogueStarted.Broadcast(GetActiveNode());

	 	ExecuteDialogueNode(GetActiveNode());
	 	return true;
	 }
	
	return false;
}

void UMounteaDialogueParticipant::ExecuteDialogueNode(UMounteaDialogueGraphNode* Node)
{
	if (UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node) )
	{
		ExecuteNode_DialogueNode(DialogueNodeBase);

		OnDialogueNodeExecuted.Broadcast(DialogueNodeBase);
		return;
	}

	if (UMounteaDialogueGraphNode_CompleteNode* CompleteNode = Cast<UMounteaDialogueGraphNode_CompleteNode>(Node) )
	{
		ExecuteNode_CompleteNode(CompleteNode);

		OnDialogueNodeExecuted.Broadcast(CompleteNode);
		return;
	}
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

void UMounteaDialogueParticipant::ExecuteNode_DialogueNode(UMounteaDialogueGraphNode_DialogueNodeBase* Node)
{
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const FDialogueRow DialogueRow = UMounteaDialogueSystemBFC::GetDialogueRow(Node);
	if (UMounteaDialogueSystemBFC::IsDialogueRowValid(DialogueRow) && DialogueRow.DialogueRows.Array().IsValidIndex(0))
	{
		StartExecuteDialogueRow(DialogueRow, 0);
	}

}

void UMounteaDialogueParticipant::StartExecuteDialogueRow(const FDialogueRow& DialogueRow, int32 Index)
{
	FTimerDelegate Delegate;
	Delegate.BindUFunction(this, "FinishedExecuteDialogueRow", DialogueRow, Index);

	GetWorld()->GetTimerManager().SetTimer
	(
		TimerHandle_RowTimer,
		Delegate,
		UMounteaDialogueSystemBFC::GetRowDuration(DialogueRow.DialogueRows.Array()[Index]),
		false
	);

	OnDialogueRowExecuted.Broadcast(GetActiveNode(), DialogueRow, Index);
}

void UMounteaDialogueParticipant::FinishedExecuteDialogueRow(const FDialogueRow& DialogueRow, int32 Index)
{
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);
	
	if (UMounteaDialogueSystemBFC::IsDialogueRowValid(DialogueRow) && DialogueRow.DialogueRows.Array().IsValidIndex(Index + 1))
	{
		StartExecuteDialogueRow(DialogueRow, Index + 1);
	}
	else
	{
		// TODO: wait for new Node to be selected
		UE_LOG(LogTemp, Error, TEXT("No more Rows to execute"))
		OnDialogueNodeFinished.Broadcast(GetActiveNode());
	}
}

void UMounteaDialogueParticipant::ExecuteNode_CompleteNode(UMounteaDialogueGraphNode_CompleteNode* Node)
{
	// TODO: Close dialogue
}
