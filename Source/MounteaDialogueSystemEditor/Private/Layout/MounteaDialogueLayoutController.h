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

#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Containers/Ticker.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Layout/Layered/MounteaDialogueLayoutTypes.h"

/**
 * Owns one Auto Arrange run for one dialogue editor.
 *
 * The calculation is a pure value task on the thread pool; this controller lives on the game thread, polls the task
 * at ~10 Hz, and is the only place that ever touches the graph: once, in one transaction, after the revision check.
 */
class FMounteaDialogueLayoutController : public TSharedFromThis<FMounteaDialogueLayoutController>
{
public:
	/** bApplied: a checkpoint was written. Message is empty when there is nothing to tell the user. */
	DECLARE_DELEGATE_TwoParams(FOnFinished, bool /*bApplied*/, const FText& /*Message*/);

	FMounteaDialogueLayoutController() = default;
	~FMounteaDialogueLayoutController();

	FOnFinished OnFinished;

	/** Captures the snapshot and launches the worker. Fails without side effects. */
	bool Start(UEdGraph_MounteaDialogueGraph* InGraph, const FText& InTransactionLabel, FText& OutError);

	/** Cooperative: the worker stops, then the latest validated checkpoint (if any) is applied. */
	void Cancel();

	/** Cancels and throws the result away (editor closing, asset going away). Blocks until the worker is done. */
	void Discard();

	bool IsBusy() const { return bBusy; }
	bool CanCancel() const { return bBusy && !bCommitting && !bCancelRequested; }
	bool IsStopping() const { return bBusy && bCancelRequested; }

	FText GetStatusText() const { return StatusText; }
	TOptional<float> GetProgress() const { return Progress; }

private:
	bool HandleTick(float DeltaTime);
	void RefreshStatus();
	void Finish();
	void WaitForWorker();

	TWeakObjectPtr<UEdGraph_MounteaDialogueGraph> Graph;
	TSharedPtr<const FDialogueLayoutSnapshot> Snapshot;
	TSharedPtr<FDialogueLayoutBinding> Binding;
	TSharedPtr<FDialogueLayoutTaskState> State;
	TFuture<void> Worker;
	FTSTicker::FDelegateHandle TickHandle;
	FText TransactionLabel;

	FText StatusText;
	TOptional<float> Progress;
	bool bBusy = false;
	bool bCommitting = false;
	bool bCancelRequested = false;
	bool bDiscard = false;
};
