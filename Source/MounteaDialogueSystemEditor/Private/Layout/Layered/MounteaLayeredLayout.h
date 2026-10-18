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
#include "Layout/Layered/MounteaDialogueLayoutTypes.h"

/**
 * Layered (Sugiyama-style) graph layout with channel routing. Pure value-in / value-out; safe on any thread.
 *
 * Pipeline: classify edges (iterative DFS, feedback edges) -> longest-path layering with routing vertices ->
 * crossing reduction (<= 16 median sweeps + adjacent swaps) -> Brandes-Koepf coordinates -> channel routing with
 * per-connection control slots -> component packing -> validation.
 */
class FMounteaLayeredLayout
{
public:
	/**
	 * Runs the pipeline and publishes validated checkpoints to State (first = complete baseline, then improvements).
	 * Ends with phase Finished, Cancelled or Failed (diagnostics in State). An empty graph finishes without a checkpoint.
	 * @return true when the run finished (or was cancelled) without failing.
	 */
	static bool Run(const FDialogueLayoutSnapshot& Snapshot, FDialogueLayoutTaskState& State);

	/** Geometric validation of a checkpoint against its snapshot. Appends human readable problems to OutProblems. */
	static bool Validate(const FDialogueLayoutSnapshot& Snapshot, const FDialogueLayoutCheckpoint& Checkpoint, TArray<FString>& OutProblems);

	/** True when no segment of the polyline runs through the interior of any obstacle (0.5 unit tolerance). */
	static bool IsRouteClear(const TArray<FVector2D>& Points, const TArray<FBox2D>& Obstacles);
};
