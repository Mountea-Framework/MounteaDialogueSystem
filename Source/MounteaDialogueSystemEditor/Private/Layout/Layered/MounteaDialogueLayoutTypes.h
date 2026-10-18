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
#include "HAL/CriticalSection.h"
#include "Templates/SharedPointer.h"

#include <atomic>

/**
 * Value-only types shared between the game thread (snapshot / commit) and the layout worker.
 * Nothing here references UObjects or Slate, so the worker can run on any thread.
 */
namespace MounteaLayout
{
	/** Used when neither a cached nor a desired size is available. */
	constexpr double FallbackNodeWidth = 250.0;
	constexpr double FallbackNodeHeight = 150.0;

	constexpr int32 MaxCrossingSweeps = 16;

	// Routing geometry (graph units).
	constexpr double StubBand = 24.0;        // free space under a source node before the first track
	constexpr double TrackSpacing = 18.0;    // distance between parallel horizontal tracks
	constexpr double LaneSpacing = 24.0;     // distance between exterior feedback lanes
	constexpr double LaneClearance = 40.0;   // node edge -> routing lane
	constexpr double BubbleSize = 38.0;      // edge control rectangle (matches MounteaDialogueWireConsts)
	constexpr double BubbleBand = 54.0;      // control top -> target top (BubbleYOffset)
	constexpr double BubblePitch = BubbleSize + 8.0;
}

struct FDialogueLayoutLimits
{
	int32 MaxRoutingVertices = 250000;     // real + temporary routing vertices
	int32 MaxExpansionsPerRoute = 100000;  // per-connection search/step budget
	int32 MaxTotalRoutePoints = 1000000;
};

struct FDialogueLayoutNode
{
	int32 Id = INDEX_NONE;
	FGuid Guid;
	FVector2D Size = FVector2D(MounteaLayout::FallbackNodeWidth, MounteaLayout::FallbackNodeHeight);
	int32 ExecutionOrder = INDEX_NONE;
	bool bIsStart = false;
};

struct FDialogueLayoutEdge
{
	int32 From = INDEX_NONE;
	int32 To = INDEX_NONE;
};

struct FDialogueLayoutSnapshot
{
	TArray<FDialogueLayoutNode> Nodes;
	TArray<FDialogueLayoutEdge> Edges;
	double HorizontalClearance = 300.0;
	double VerticalClearance = 200.0;
	/** Id of the actual Start node; INDEX_NONE only valid for an empty graph. */
	int32 AnchorId = INDEX_NONE;
	uint32 Revision = 0;
	FDialogueLayoutLimits Limits;
};

struct FDialogueLayoutRoute
{
	TArray<FVector2D> Points;
	/** Top-left of the edge control rectangle (BubbleSize square). */
	FIntPoint ControlPosition = FIntPoint::ZeroValue;
	bool bFeedback = false;
	bool IsValid() const { return Points.Num() >= 2; }
};

struct FDialogueLayoutCheckpoint
{
	/** Parallel to FDialogueLayoutSnapshot::Nodes: top-left, Start is (0,0). */
	TArray<FIntPoint> NodePositions;
	/** Parallel to FDialogueLayoutSnapshot::Edges; invalid edges have an empty route. */
	TArray<FDialogueLayoutRoute> Routes;

	int32 Crossings = 0;
	int32 Bends = 0;
	double TotalRouteLength = 0.0;
	uint32 InputRevision = 0;
	int32 Sequence = 0;
};

enum class EDialogueLayoutPhase : uint8
{
	Idle,
	Classifying,
	Layering,
	ReducingCrossings,
	Coordinates,
	Routing,
	Validating,
	Finished,
	Failed,
	Cancelled
};

/** Shared between worker (writes progress / checkpoints) and the editor controller (reads, cancels). */
class FDialogueLayoutTaskState
{
public:
	void RequestCancel() { bCancelRequested.store(true); }
	bool IsCancelRequested() const { return bCancelRequested.load(); }

	void SetProgress(EDialogueLayoutPhase NewPhase, int32 NewDone, int32 NewTotal)
	{
		Phase.store(static_cast<uint8>(NewPhase));
		Done.store(NewDone);
		Total.store(NewTotal);
	}
	EDialogueLayoutPhase GetPhase() const { return static_cast<EDialogueLayoutPhase>(Phase.load()); }
	int32 GetDone() const { return Done.load(); }
	int32 GetTotal() const { return Total.load(); }

	/** Worker finished (including cancelled / failed). */
	void MarkCompleted() { bCompleted.store(true); }
	bool IsCompleted() const { return bCompleted.load(); }

	void PublishCheckpoint(TSharedRef<const FDialogueLayoutCheckpoint> Checkpoint)
	{
		FScopeLock lock(&Mutex);
		Latest = Checkpoint;
	}
	TSharedPtr<const FDialogueLayoutCheckpoint> GetLatestCheckpoint() const
	{
		FScopeLock lock(&Mutex);
		return Latest;
	}

	void AddDiagnostic(const FString& Message)
	{
		FScopeLock lock(&Mutex);
		Diagnostics.Add(Message);
	}
	TArray<FString> GetDiagnostics() const
	{
		FScopeLock lock(&Mutex);
		return Diagnostics;
	}

private:
	std::atomic<bool> bCancelRequested{false};
	std::atomic<bool> bCompleted{false};
	std::atomic<uint8> Phase{static_cast<uint8>(EDialogueLayoutPhase::Idle)};
	std::atomic<int32> Done{0};
	std::atomic<int32> Total{0};

	mutable FCriticalSection Mutex;
	TSharedPtr<const FDialogueLayoutCheckpoint> Latest;
	TArray<FString> Diagnostics;
};
