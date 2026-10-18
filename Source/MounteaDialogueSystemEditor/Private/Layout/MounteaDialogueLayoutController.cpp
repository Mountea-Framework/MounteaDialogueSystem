// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not 
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "Layout/MounteaDialogueLayoutController.h"

#include "Async/Async.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Layout/Layered/MounteaLayeredLayout.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueLayoutController"

FMounteaDialogueLayoutController::~FMounteaDialogueLayoutController()
{
	Discard();
}

bool FMounteaDialogueLayoutController::Start(UEdGraph_MounteaDialogueGraph* InGraph, const FText& InTransactionLabel, FText& OutError)
{
	if (bBusy)
	{
		OutError = LOCTEXT("AlreadyRunning", "An arrangement is already being calculated.");
		return false;
	}
	if (!InGraph)
	{
		OutError = LOCTEXT("NoGraph", "There is no dialogue graph to arrange.");
		return false;
	}

	TSharedRef<FDialogueLayoutSnapshot> snapshot = MakeShared<FDialogueLayoutSnapshot>();
	TSharedRef<FDialogueLayoutBinding> binding = MakeShared<FDialogueLayoutBinding>();
	FString error;
	if (!InGraph->BuildLayoutSnapshot(*snapshot, *binding, error))
	{
		OutError = FText::FromString(error);
		return false;
	}

	if (snapshot->Nodes.Num() == 0)
	{
		// Empty graph: nothing to do, and nothing to report.
		OutError = FText::GetEmpty();
		return true;
	}
	if (snapshot->AnchorId == INDEX_NONE)
	{
		OutError = LOCTEXT("NoStart", "The Start node could not be found, so the graph cannot be arranged.");
		return false;
	}

	Graph = InGraph;
	Snapshot = snapshot;
	Binding = binding;
	State = MakeShared<FDialogueLayoutTaskState>();
	TransactionLabel = InTransactionLabel;
	bBusy = true;
	bCommitting = false;
	bCancelRequested = false;
	bDiscard = false;
	Progress = 0.0f;
	StatusText = LOCTEXT("Starting", "Preparing…");

	// The worker only ever sees these two shared value objects.
	const TSharedRef<const FDialogueLayoutSnapshot> workerSnapshot = snapshot;
	const TSharedRef<FDialogueLayoutTaskState> workerState = State.ToSharedRef();
	Worker = Async(EAsyncExecution::ThreadPool, [workerSnapshot, workerState]()
	{
		FMounteaLayeredLayout::Run(*workerSnapshot, *workerState);
	});

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &FMounteaDialogueLayoutController::HandleTick), 0.1f);
	return true;
}

void FMounteaDialogueLayoutController::Cancel()
{
	if (!CanCancel())
		return;
	bCancelRequested = true;
	State->RequestCancel();
	RefreshStatus();
}

void FMounteaDialogueLayoutController::Discard()
{
	if (!bBusy)
		return;

	bDiscard = true;
	bCancelRequested = true;
	if (State.IsValid())
		State->RequestCancel();
	WaitForWorker();

	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	bBusy = false;
	bCommitting = false;
	Progress.Reset();
	Snapshot.Reset();
	Binding.Reset();
	State.Reset();
}

void FMounteaDialogueLayoutController::WaitForWorker()
{
	if (Worker.IsValid())
		Worker.Wait();
	Worker = TFuture<void>();
}

bool FMounteaDialogueLayoutController::HandleTick(float DeltaTime)
{
	if (!bBusy)
		return false;

	RefreshStatus();
	if (State->IsCompleted() && Worker.IsValid() && Worker.IsReady())
	{
		Finish();
		return false;
	}
	return true;
}

void FMounteaDialogueLayoutController::RefreshStatus()
{
	if (!State.IsValid())
		return;

	const int32 done = State->GetDone();
	const int32 total = FMath::Max(State->GetTotal(), 1);
	const float fraction = FMath::Clamp(static_cast<float>(done) / total, 0.0f, 1.0f);

	if (bCommitting)
	{
		StatusText = LOCTEXT("Applying", "Applying arrangement…");
		Progress = 1.0f;
		return;
	}
	if (bCancelRequested)
	{
		StatusText = LOCTEXT("Stopping", "Stopping…");
		return;
	}

	switch (State->GetPhase())
	{
	case EDialogueLayoutPhase::Classifying:
		StatusText = LOCTEXT("Classifying", "Analysing connections…");
		Progress = 0.02f;
		break;
	case EDialogueLayoutPhase::Layering:
		StatusText = FText::Format(LOCTEXT("Layering", "Layering nodes: {0}/{1}"), done, State->GetTotal());
		Progress = 0.04f + 0.04f * fraction;
		break;
	case EDialogueLayoutPhase::Coordinates:
		StatusText = LOCTEXT("Coordinates", "Placing nodes…");
		Progress = 0.1f;
		break;
	case EDialogueLayoutPhase::Routing:
		StatusText = FText::Format(LOCTEXT("Routing", "Routing connections: {0}/{1}"), done, State->GetTotal());
		Progress = 0.1f + 0.2f * fraction;
		break;
	case EDialogueLayoutPhase::ReducingCrossings:
		StatusText = FText::Format(LOCTEXT("Crossings", "Reducing crossings: {0}/{1}"), done, State->GetTotal());
		Progress = 0.3f + 0.65f * fraction;
		break;
	case EDialogueLayoutPhase::Validating:
		StatusText = LOCTEXT("Validating", "Validating arrangement…");
		break;
	default:
		break;
	}
}

void FMounteaDialogueLayoutController::Finish()
{
	// From here on Cancel is disabled: the commit is short and atomic.
	bCommitting = true;
	RefreshStatus();
	WaitForWorker();

	const TSharedPtr<const FDialogueLayoutCheckpoint> checkpoint = State->GetLatestCheckpoint();
	const EDialogueLayoutPhase phase = State->GetPhase();
	const bool bUserStopped = bCancelRequested;
	for (const FString& diagnostic : State->GetDiagnostics())
		EditorLOG_WARNING(TEXT("[AutoArrange] %s"), *diagnostic);

	bool bApplied = false;
	FText message;
	UEdGraph_MounteaDialogueGraph* graph = Graph.Get();

	if (bDiscard)
	{
		// Editor is going away: nothing may be written.
	}
	else if (!graph)
	{
		message = LOCTEXT("GraphGone", "The dialogue is no longer available; the arrangement was discarded.");
	}
	else if (checkpoint.IsValid())
	{
		FString error;
		{
			FScopedTransaction transaction(TransactionLabel);
			bApplied = graph->ApplyLayoutCheckpoint(*Snapshot, *Binding, *checkpoint, error);
			if (!bApplied)
				transaction.Cancel();
		}

		if (bApplied)
		{
			graph->UEdGraph::NotifyGraphChanged();
			if (bUserStopped)
				message = LOCTEXT("StoppedApplied", "Arrangement stopped early; the latest complete arrangement was applied.");
		}
		else
			message = FText::FromString(error);
	}
	else if (phase == EDialogueLayoutPhase::Failed)
	{
		const TArray<FString> diagnostics = State->GetDiagnostics();
		message = diagnostics.Num() > 0 ? FText::FromString(diagnostics.Last()) : LOCTEXT("Failed", "The arrangement could not be calculated.");
	}
	else if (bUserStopped)
	{
		message = LOCTEXT("StoppedNothing", "Arrangement cancelled before a complete result existed; nothing was changed.");
	}

	bBusy = false;
	bCommitting = false;
	Progress.Reset();
	const FText finishedMessage = message;
	Snapshot.Reset();
	Binding.Reset();
	State.Reset();

	OnFinished.ExecuteIfBound(bApplied, finishedMessage);
}

#undef LOCTEXT_NAMESPACE
