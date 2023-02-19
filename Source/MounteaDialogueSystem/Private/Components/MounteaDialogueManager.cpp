// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueManager.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

UMounteaDialogueManager::UMounteaDialogueManager()
{
	DialogueContext = nullptr;
}

void UMounteaDialogueManager::BeginPlay()
{
	Super::BeginPlay();

	OnDialogueInitialized.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueInitializedEvent_Internal);
	OnDialogueContextUpdated.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueContextUpdatedEvent_Internal);
	OnDialogueStarted.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueStartedEvent_Internal);
	OnDialogueClosed.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueClosedEvent_Internal);
}

void UMounteaDialogueManager::OnDialogueInitializedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (Context)
	{
		OnDialogueInitializedEvent(Context);

		OnDialogueContextUpdated.Broadcast(Context);

		OnDialogueStarted.Broadcast(Context);
	}
	else
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}
}

void UMounteaDialogueManager::OnDialogueContextUpdatedEvent_Internal(UMounteaDialogueContext* NewContext)
{
	SetDialogueContext(NewContext);
}

void UMounteaDialogueManager::OnDialogueStartedEvent_Internal(UMounteaDialogueContext* Context)
{
	StartDialogue();

	OnDialogueStartedEvent(Context);
}

void UMounteaDialogueManager::OnDialogueClosedEvent_Internal(UMounteaDialogueContext* Context)
{
	OnDialogueClosedEvent(DialogueContext);

	CloseDialogue();
}

void UMounteaDialogueManager::StartDialogue()
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}
	
	//TODO: Add ability to start from specific Node
	ProcessNode();
}

void UMounteaDialogueManager::CloseDialogue()
{
	//TODO: Close dialogue, destroy Context, clean Timer
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	if(!DialogueContext) return;

	DialogueContext->ConditionalBeginDestroy();
	DialogueContext = nullptr;
}

void UMounteaDialogueManager::ProcessNode()
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}

	if (UMounteaDialogueGraphNode_DialogueNodeBase* DialogueLeadNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(DialogueContext->ActiveNode) )
	{
		ProcessNode_Dialogue();
		return;
	}
	
	if (UMounteaDialogueGraphNode_CompleteNode* CompleteNode = Cast<UMounteaDialogueGraphNode_CompleteNode>(DialogueContext->ActiveNode) )
	{
		ProcessNode_Complete();
		return;
	}
}

void UMounteaDialogueManager::ProcessNode_Complete()
{
	CloseDialogue();
}

void UMounteaDialogueManager::ProcessNode_Dialogue()
{
	if (!GetWorld())
	{
		OnDialogueFailed.Broadcast(TEXT("Cannot find World!"));
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const FDialogueRow DialogueRow = UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode);
	if (UMounteaDialogueSystemBFC::IsDialogueRowValid(DialogueRow) && DialogueRow.DialogueRowData.Array().IsValidIndex(0))
	{
		DialogueContext->UpdateActiveDialogueRow(DialogueRow);
		DialogueContext->UpdateActiveDialogueRowDataIndex(0);
		OnDialogueContextUpdated.Broadcast(DialogueContext);
		
		StartExecuteDialogueRow();
	}
	else
	{
		OnDialogueFailed.Broadcast(TEXT("Dialogue Row data contain Invalid Rows!"));
		return;
	}

	OnDialogueNodeStarted.Broadcast(DialogueContext);
}

void UMounteaDialogueManager::StartExecuteDialogueRow()
{
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &UMounteaDialogueManager::FinishedExecuteDialogueRow);

	const int32 Index = DialogueContext->GetActiveDialogueRowDataIndex();
	const auto Row = DialogueContext->GetActiveDialogueRow();
	const auto RowData = Row.DialogueRowData.Array()[Index];

	GetWorld()->GetTimerManager().SetTimer
	(
		TimerHandle_RowTimer,
		Delegate,
		UMounteaDialogueSystemBFC::GetRowDuration(RowData),
		false
	);

	OnDialogueRowStarted.Broadcast(DialogueContext);
}

void UMounteaDialogueManager::FinishedExecuteDialogueRow()
{
	if (!GetWorld())
	{
		OnDialogueFailed.Broadcast(TEXT("Cannot find World!"));
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const int32 IncreasedIndex = DialogueContext->GetActiveDialogueRowDataIndex() + 1;

	const bool bIsActiveRowValid = UMounteaDialogueSystemBFC::IsDialogueRowValid(DialogueContext->GetActiveDialogueRow());
	const bool bDialogueRowDataValid = DialogueContext->GetActiveDialogueRow().DialogueRowData.Array().IsValidIndex(IncreasedIndex);

	if (bIsActiveRowValid && bDialogueRowDataValid)
	{
		DialogueContext->UpdateActiveDialogueRowDataIndex(IncreasedIndex);
		OnDialogueContextUpdated.Broadcast(DialogueContext);
		
		StartExecuteDialogueRow();
	}
	else
	{
		OnDialogueNodeFinished.Broadcast(DialogueContext);
	}

	OnDialogueRowFinished.Broadcast(DialogueContext);
}

void UMounteaDialogueManager::SetDialogueContext(UMounteaDialogueContext* NewContext)
{
	DialogueContext = NewContext;

	OnDialogueContextUpdatedEvent(DialogueContext);
}