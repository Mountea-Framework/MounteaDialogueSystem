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

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Layout/Layered/MounteaLayeredLayout.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "Math/RandomStream.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

#include <limits>

// Pure solver tests: value snapshots in, validated checkpoints out. No UObjects, no Slate.

namespace MounteaLayoutTest
{
	using FEdgeList = TArray<TPair<int32, int32>>;

	// Mirrors the supplied UITestDialogue regression fixture (32 nodes, 45 connections, shared descendants).
	// The archive itself lives in Fixtures/UITestDialogue.mnteadlg.
	static const int32 UITestStartIndex = 0;
	static const int32 UITestNodeCount = 32;
	static const int32 UITestEdges[][2] = {
		{0, 18},
		{1, 11},
		{2, 11},
		{3, 29},
		{4, 1},
		{5, 11},
		{6, 5},
		{7, 3},
		{7, 9},
		{8, 3},
		{8, 4},
		{8, 6},
		{8, 26},
		{8, 31},
		{9, 11},
		{10, 1},
		{12, 23},
		{13, 3},
		{13, 14},
		{13, 15},
		{13, 22},
		{13, 27},
		{14, 30},
		{15, 8},
		{16, 28},
		{17, 11},
		{18, 16},
		{19, 1},
		{20, 13},
		{21, 3},
		{21, 10},
		{21, 12},
		{21, 17},
		{21, 25},
		{22, 21},
		{23, 2},
		{24, 23},
		{25, 5},
		{26, 11},
		{27, 7},
		{28, 20},
		{30, 3},
		{30, 19},
		{30, 24},
		{31, 23}
	};

	FDialogueLayoutSnapshot MakeSnapshot(int32 NodeCount, const FEdgeList& Edges, int32 StartIndex = 0)
	{
		FDialogueLayoutSnapshot snapshot;
		for (int32 i = 0; i < NodeCount; ++i)
		{
			FDialogueLayoutNode node;
			node.Id = i;
			node.Guid = FGuid(i + 1, 0, 0, 0);
			node.Size = FVector2D(200.0 + (i % 3) * 60.0, 120.0 + (i % 2) * 50.0); // varied dimensions
			node.ExecutionOrder = i == StartIndex ? 0 : i;
			node.bIsStart = i == StartIndex;
			snapshot.Nodes.Add(node);
		}
		for (const TPair<int32, int32>& edge : Edges)
			snapshot.Edges.Add({edge.Key, edge.Value});
		snapshot.AnchorId = NodeCount > 0 ? StartIndex : INDEX_NONE;
		snapshot.Revision = 7;
		return snapshot;
	}

	struct FRunResult
	{
		TSharedPtr<const FDialogueLayoutCheckpoint> Checkpoint;
		EDialogueLayoutPhase Phase = EDialogueLayoutPhase::Idle;
		TArray<FString> Diagnostics;
		bool bOk = false;
	};

	FRunResult RunLayout(const FDialogueLayoutSnapshot& Snapshot, bool bCancelFirst = false)
	{
		FDialogueLayoutTaskState state;
		if (bCancelFirst)
			state.RequestCancel();
		FRunResult result;
		result.bOk = FMounteaLayeredLayout::Run(Snapshot, state);
		result.Checkpoint = state.GetLatestCheckpoint();
		result.Phase = state.GetPhase();
		result.Diagnostics = state.GetDiagnostics();
		return result;
	}

	// -MounteaDumpLayout writes each arrangement to Saved/MounteaLayout for visual review (before/after captures).
	void DumpLayout(const FString& Name, const FDialogueLayoutSnapshot& Snapshot, const FDialogueLayoutCheckpoint& Checkpoint)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("MounteaDumpLayout")))
			return;

		FString text;
		for (int32 i = 0; i < Snapshot.Nodes.Num(); ++i)
			text += FString::Printf(TEXT("N %d %d %d %.1f %.1f") TEXT("\n"), i, Checkpoint.NodePositions[i].X, Checkpoint.NodePositions[i].Y, Snapshot.Nodes[i].Size.X, Snapshot.Nodes[i].Size.Y);
		for (int32 e = 0; e < Checkpoint.Routes.Num(); ++e)
		{
			const FDialogueLayoutRoute& route = Checkpoint.Routes[e];
			if (!route.IsValid())
				continue;
			text += FString::Printf(TEXT("R %d %d"), e, route.bFeedback ? 1 : 0);
			for (const FVector2D& p : route.Points)
				text += FString::Printf(TEXT(" %.1f,%.1f"), p.X, p.Y);
			text += FString::Printf(TEXT("\nC %d %d") TEXT("\n"), route.ControlPosition.X, route.ControlPosition.Y);
		}
		FFileHelper::SaveStringToFile(text, *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MounteaLayout"), Name + TEXT(".layout.txt")));
	}

	FEdgeList Chain(int32 Count)
	{
		FEdgeList edges;
		for (int32 i = 0; i + 1 < Count; ++i)
			edges.Add({i, i + 1});
		return edges;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLayeredShapesTest,
	"Mountea.DialogueSystemEditor.Layout.Shapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLayeredShapesTest::RunTest(const FString& Parameters)
{
	using namespace MounteaLayoutTest;

	struct FCase
	{
		FString Name;
		int32 NodeCount;
		FEdgeList Edges;
		int32 Start = 0;
	};
	TArray<FCase> cases;

	cases.Add({TEXT("Chain"), 6, Chain(6)});
	cases.Add({TEXT("Diamond"), 4, {{0, 1}, {0, 2}, {1, 3}, {2, 3}}});

	{
		FEdgeList edges; // repeated diamonds sharing descendants
		for (int32 k = 0; k < 8; ++k)
		{
			const int32 top = k * 3;
			edges.Append({{top, top + 1}, {top, top + 2}, {top + 1, top + 3}, {top + 2, top + 3}});
		}
		cases.Add({TEXT("RepeatedDiamonds"), 25, edges});
	}
	{
		FEdgeList fanOut, fanIn;
		for (int32 i = 1; i <= 14; ++i)
		{
			fanOut.Add({0, i});
			fanIn.Add({0, i});
			fanIn.Add({i, 15});
		}
		cases.Add({TEXT("HighFanOut"), 15, fanOut});
		cases.Add({TEXT("HighFanInAndOut"), 16, fanIn});
	}
	{
		FEdgeList edges = Chain(7); // long connections skipping several layers
		edges.Add({0, 6});
		edges.Add({1, 6});
		cases.Add({TEXT("LongConnections"), 7, edges});
	}
	{
		FEdgeList edges = Chain(4);
		edges.Append({{5, 6}, {6, 7}, {8, 9}});
		cases.Add({TEXT("DisconnectedComponentsAndIsolated"), 12, edges});
	}
	cases.Add({TEXT("StartOnly"), 1, {}});
	cases.Add({TEXT("SelfLoop"), 3, {{0, 1}, {1, 1}, {1, 2}}});
	cases.Add({TEXT("ManySelfLoops"), 3, {{0, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 2}, {2, 2}}});
	cases.Add({TEXT("TwoCycle"), 3, {{0, 1}, {1, 2}, {2, 1}}});
	cases.Add({TEXT("BackToStart"), 4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}}});
	cases.Add({TEXT("RootlessCycleComponent"), 6, {{0, 1}, {3, 4}, {4, 5}, {5, 3}}});
	cases.Add({TEXT("ParallelConnections"), 3, {{0, 1}, {1, 2}, {1, 2}, {1, 2}}});
	cases.Add({TEXT("StartNotFirst"), 4, {{2, 0}, {0, 1}, {0, 3}}, 2});

	{
		FEdgeList edges; // supplied dialogue
		for (const int32 (&edge)[2] : UITestEdges)
			edges.Add({edge[0], edge[1]});
		cases.Add({TEXT("UITestDialogue"), UITestNodeCount, edges, UITestStartIndex});
	}
	{
		FRandomStream random(1337); // dense random graph with cycles
		FEdgeList edges;
		for (int32 i = 0; i < 120; ++i)
			edges.Add({random.RandRange(0, 79), random.RandRange(0, 79)});
		cases.Add({TEXT("RandomWithCycles"), 80, edges});
	}

	for (const FCase& testCase : cases)
	{
		const FDialogueLayoutSnapshot snapshot = MakeSnapshot(testCase.NodeCount, testCase.Edges, testCase.Start);
		const FRunResult first = RunLayout(snapshot);
		TestTrue(*(testCase.Name + TEXT(": run succeeds")), first.bOk);
		TestEqual(*(testCase.Name + TEXT(": finishes")), first.Phase, EDialogueLayoutPhase::Finished);
		if (!first.Checkpoint.IsValid())
		{
			AddError(testCase.Name + TEXT(": no checkpoint published. ") + FString::Join(first.Diagnostics, TEXT(" | ")));
			continue;
		}

		TArray<FString> problems;
		TestTrue(*(testCase.Name + TEXT(": checkpoint validates")), FMounteaLayeredLayout::Validate(snapshot, *first.Checkpoint, problems));
		if (problems.Num() > 0)
			AddError(testCase.Name + TEXT(": ") + FString::Join(problems, TEXT(" | ")));

		DumpLayout(testCase.Name, snapshot, *first.Checkpoint);
		TestEqual(*(testCase.Name + TEXT(": Start at origin")), first.Checkpoint->NodePositions[testCase.Start], FIntPoint::ZeroValue);
		TestEqual(*(testCase.Name + TEXT(": revision carried")), first.Checkpoint->InputRevision, 7u);

		// Positions are not an input, so a repeated run must reproduce the same arrangement.
		const FRunResult second = RunLayout(snapshot);
		TestTrue(*(testCase.Name + TEXT(": deterministic, no drift")), second.Checkpoint.IsValid() && second.Checkpoint->NodePositions == first.Checkpoint->NodePositions);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLayeredFailureModesTest,
	"Mountea.DialogueSystemEditor.Layout.FailureModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLayeredFailureModesTest::RunTest(const FString& Parameters)
{
	using namespace MounteaLayoutTest;

	{
		const FRunResult result = RunLayout(MakeSnapshot(0, {}));
		TestTrue(TEXT("Empty graph is a successful no-op"), result.bOk);
		TestEqual(TEXT("Empty graph finishes"), result.Phase, EDialogueLayoutPhase::Finished);
		TestFalse(TEXT("Empty graph publishes nothing"), result.Checkpoint.IsValid());
	}
	{
		FDialogueLayoutSnapshot snapshot = MakeSnapshot(3, Chain(3));
		snapshot.AnchorId = INDEX_NONE;
		const FRunResult result = RunLayout(snapshot);
		TestFalse(TEXT("Missing Start fails"), result.bOk);
		TestEqual(TEXT("Missing Start phase"), result.Phase, EDialogueLayoutPhase::Failed);
		TestTrue(TEXT("Missing Start has a diagnostic"), result.Diagnostics.Num() > 0);
		TestFalse(TEXT("Missing Start publishes nothing"), result.Checkpoint.IsValid());
	}
	{
		FDialogueLayoutSnapshot snapshot = MakeSnapshot(3, Chain(3));
		snapshot.Edges.Add({0, 99});
		snapshot.Edges.Add({-4, 1});
		const FRunResult result = RunLayout(snapshot);
		TestTrue(TEXT("Invalid endpoints are skipped"), result.bOk && result.Checkpoint.IsValid());
		TestTrue(TEXT("Invalid endpoints are reported"), result.Diagnostics.Num() > 0);
		if (result.Checkpoint.IsValid())
		{
			TestFalse(TEXT("Invalid connection has no route"), result.Checkpoint->Routes[2].IsValid());
			TestTrue(TEXT("Valid connections are routed"), result.Checkpoint->Routes[0].IsValid());
		}
	}
	{
		FDialogueLayoutSnapshot snapshot = MakeSnapshot(3, Chain(3));
		snapshot.Nodes[1].Id = 5;
		TestFalse(TEXT("Duplicate / sparse ids are rejected"), RunLayout(snapshot).bOk);
	}
	{
		FDialogueLayoutSnapshot snapshot = MakeSnapshot(4, Chain(4));
		for (FDialogueLayoutNode& node : snapshot.Nodes)
			node.Size = FVector2D::ZeroVector; // unmeasured
		snapshot.Nodes[2].Size = FVector2D(std::numeric_limits<double>::quiet_NaN(), -3.0);
		const FRunResult result = RunLayout(snapshot);
		TestTrue(TEXT("Unmeasured sizes use the documented fallback"), result.bOk && result.Checkpoint.IsValid());
	}
	{
		FDialogueLayoutSnapshot snapshot = MakeSnapshot(4, Chain(4));
		for (FDialogueLayoutNode& node : snapshot.Nodes)
			node.Size = FVector2D(90000.0, 70000.0); // extreme dimensions
		const FRunResult result = RunLayout(snapshot);
		TestTrue(TEXT("Extreme dimensions stay finite and valid"), result.bOk && result.Checkpoint.IsValid());
	}
	{
		FDialogueLayoutSnapshot snapshot = MakeSnapshot(6, {{0, 5}, {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}});
		snapshot.Limits.MaxRoutingVertices = 7; // the long connection needs more routing vertices than this
		const FRunResult result = RunLayout(snapshot);
		TestFalse(TEXT("Routing-vertex budget exhaustion fails"), result.bOk);
		TestFalse(TEXT("Nothing is published when the baseline cannot be built"), result.Checkpoint.IsValid());
		TestTrue(TEXT("Budget exhaustion is reported"), result.Diagnostics.Num() > 0);
	}
	{
		FDialogueLayoutSnapshot snapshot = MakeSnapshot(6, Chain(6));
		snapshot.Limits.MaxTotalRoutePoints = 3;
		const FRunResult result = RunLayout(snapshot);
		TestFalse(TEXT("Route-point budget exhaustion fails"), result.bOk);
		TestFalse(TEXT("No checkpoint after route-point exhaustion"), result.Checkpoint.IsValid());
	}
	{
		const FRunResult result = RunLayout(MakeSnapshot(5, Chain(5)), /*bCancelFirst*/ true);
		TestTrue(TEXT("Cancelling before the first checkpoint is not a failure"), result.bOk);
		TestEqual(TEXT("Cancel phase"), result.Phase, EDialogueLayoutPhase::Cancelled);
		TestFalse(TEXT("Cancel before the first checkpoint leaves nothing to apply"), result.Checkpoint.IsValid());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLayeredOrderTest,
	"Mountea.DialogueSystemEditor.Layout.OptionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLayeredOrderTest::RunTest(const FString& Parameters)
{
	using namespace MounteaLayoutTest;

	FDialogueLayoutSnapshot snapshot = MakeSnapshot(5, {{0, 1}, {0, 2}, {0, 3}, {0, 4}});
	snapshot.Nodes[1].ExecutionOrder = 3;
	snapshot.Nodes[2].ExecutionOrder = 1;
	snapshot.Nodes[3].ExecutionOrder = 4;
	snapshot.Nodes[4].ExecutionOrder = 2;

	const FRunResult result = RunLayout(snapshot);
	if (!TestTrue(TEXT("Layout produced a checkpoint"), result.Checkpoint.IsValid()))
		return false;

	const TArray<FIntPoint>& p = result.Checkpoint->NodePositions;
	TestTrue(TEXT("Siblings are laid out left-to-right in option order"), p[2].X < p[4].X && p[4].X < p[1].X && p[1].X < p[3].X);
	TestTrue(TEXT("Children sit below the Start node"), p[1].Y > p[0].Y && p[1].Y == p[2].Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLayeredStressTest,
	"Mountea.DialogueSystemEditor.Layout.Stress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLayeredStressTest::RunTest(const FString& Parameters)
{
	using namespace MounteaLayoutTest;

	FRandomStream random(42);
	FEdgeList edges;
	const int32 nodeCount = 400;
	for (int32 i = 1; i < nodeCount; ++i)
		edges.Add({random.RandRange(0, i - 1), i}); // spanning tree
	for (int32 i = 0; i < 500; ++i)
		edges.Add({random.RandRange(0, nodeCount - 1), random.RandRange(0, nodeCount - 1)}); // joins, back edges, loops

	const FDialogueLayoutSnapshot snapshot = MakeSnapshot(nodeCount, edges);
	const double started = FPlatformTime::Seconds();
	const FRunResult result = RunLayout(snapshot);
	const double elapsed = FPlatformTime::Seconds() - started;

	TestTrue(TEXT("Large graph lays out"), result.bOk && result.Checkpoint.IsValid());
	TestTrue(*FString::Printf(TEXT("Large graph lays out in reasonable time (%.2fs)"), elapsed), elapsed < 20.0);
	if (result.Checkpoint.IsValid())
	{
		TArray<FString> problems;
		TestTrue(TEXT("Large graph validates"), FMounteaLayeredLayout::Validate(snapshot, *result.Checkpoint, problems));
		if (problems.Num() > 0)
			AddError(FString::Join(problems, TEXT(" | ")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLayeredCancelTest,
	"Mountea.DialogueSystemEditor.Layout.Cancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLayeredCancelTest::RunTest(const FString& Parameters)
{
	using namespace MounteaLayoutTest;

	FRandomStream random(99);
	FEdgeList edges;
	const int32 nodeCount = 1500;
	for (int32 i = 1; i < nodeCount; ++i)
		edges.Add({random.RandRange(FMath::Max(0, i - 40), i - 1), i});
	for (int32 i = 0; i < 1500; ++i)
		edges.Add({random.RandRange(0, nodeCount - 1), random.RandRange(0, nodeCount - 1)});

	const TSharedRef<const FDialogueLayoutSnapshot> snapshot = MakeShared<const FDialogueLayoutSnapshot>(MakeSnapshot(nodeCount, edges));
	const TSharedRef<FDialogueLayoutTaskState> state = MakeShared<FDialogueLayoutTaskState>();
	TFuture<void> worker = Async(EAsyncExecution::ThreadPool, [snapshot, state]() { FMounteaLayeredLayout::Run(*snapshot, *state); });

	// Wait for the first complete checkpoint, then cancel.
	const double waitStart = FPlatformTime::Seconds();
	while (!state->GetLatestCheckpoint().IsValid() && !state->IsCompleted() && FPlatformTime::Seconds() - waitStart < 60.0)
		FPlatformProcess::Sleep(0.005f);

	const double cancelAt = FPlatformTime::Seconds();
	state->RequestCancel();
	while (!state->IsCompleted() && FPlatformTime::Seconds() - cancelAt < 30.0)
		FPlatformProcess::Sleep(0.001f);
	const double stopLatency = FPlatformTime::Seconds() - cancelAt;
	worker.Wait();

	TestTrue(TEXT("The worker acknowledged the cancellation"), state->IsCompleted());
	TestTrue(*FString::Printf(TEXT("Cancellation is prompt (%.0f ms)"), stopLatency * 1000.0), stopLatency < 2.0);

	const TSharedPtr<const FDialogueLayoutCheckpoint> checkpoint = state->GetLatestCheckpoint();
	if (TestTrue(TEXT("A validated checkpoint survives cancellation"), checkpoint.IsValid()))
	{
		TArray<FString> problems;
		TestTrue(TEXT("The surviving checkpoint is complete and valid"), FMounteaLayeredLayout::Validate(*snapshot, *checkpoint, problems));
		TestEqual(TEXT("Start is at the origin in the cancelled checkpoint"), checkpoint->NodePositions[0], FIntPoint::ZeroValue);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
