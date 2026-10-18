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

#include "Tests/MounteaDialogueEditorTestFixtures.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "GraphScheme/AssetGraphScheme_MounteaDialogueGraph.h"
#include "GraphScheme/FConnectionDrawingPolicy_MounteaDialogueGraph.h"
#include "Rendering/DrawElements.h"
#include "Layout/Layered/MounteaLayeredLayout.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

// Editor side of Auto Arrange: snapshot capture, the single validated commit, route preparation, and the rule that
// option order only changes on purpose (manual movement), never as a side effect of editing or arranging.

namespace MounteaLayoutEditorTest
{
	struct FDiamond
	{
		MounteaEditorTest::FEditorTestGraph Graph{TEXT("LayoutEditorGraph")};
		UMounteaDialogueGraphNode_LeadNode* Lead = nullptr;
		UMounteaDialogueGraphNode_AnswerNode* A = nullptr;
		UMounteaDialogueGraphNode_AnswerNode* B = nullptr;
		UMounteaDialogueGraphNode_AnswerNode* C = nullptr;

		// Start -> Lead -> {A, B, C}
		void Build(FAutomationTestBase& Test)
		{
			Lead = Graph.AddNode<UMounteaDialogueGraphNode_LeadNode>();
			A = Graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
			B = Graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
			C = Graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
			Graph.MakeEdGraph();

			const UEdGraphSchema* schema = Graph.EdGraph->GetSchema();
			Graph.EdNodeOf(A)->NodePosX = 0;
			Graph.EdNodeOf(B)->NodePosX = 400;
			Graph.EdNodeOf(C)->NodePosX = 800;
			Test.TestTrue(TEXT("Start -> Lead"), schema->TryCreateConnection(Graph.EdNodeOf(Graph.Start())->GetOutputPin(), Graph.EdNodeOf(Lead)->GetInputPin()));
			for (UMounteaDialogueGraphNode* answer : TArray<UMounteaDialogueGraphNode*>({ A, B, C }))
				Test.TestTrue(TEXT("Lead -> answer"), schema->TryCreateConnection(Graph.EdNodeOf(Lead)->GetOutputPin(), Graph.EdNodeOf(answer)->GetInputPin()));
			Graph.EdGraph->RebuildMounteaDialogueGraph();
		}

		TArray<int32> Orders() const
		{
			return { Lead->ExecutionOrder, A->ExecutionOrder, B->ExecutionOrder, C->ExecutionOrder };
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLayoutEditorCommitTest,
	"Mountea.DialogueSystemEditor.Layout.EditorCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLayoutEditorCommitTest::RunTest(const FString& Parameters)
{
	MounteaLayoutEditorTest::FDiamond dialogue;
	dialogue.Build(*this);
	UEdGraph_MounteaDialogueGraph* edGraph = dialogue.Graph.EdGraph;
	const TArray<int32> ordersBefore = dialogue.Orders();

	FDialogueLayoutSnapshot snapshot;
	FDialogueLayoutBinding binding;
	FString error;
	if (!TestTrue(TEXT("Snapshot is captured"), edGraph->BuildLayoutSnapshot(snapshot, binding, error)))
		return false;
	TestEqual(TEXT("Snapshot has every node"), snapshot.Nodes.Num(), 5);
	TestEqual(TEXT("Snapshot has every physical connection"), snapshot.Edges.Num(), 4);
	TestTrue(TEXT("Start is resolved"), snapshot.AnchorId != INDEX_NONE && snapshot.Nodes[snapshot.AnchorId].bIsStart);
	for (const FDialogueLayoutNode& node : snapshot.Nodes)
		TestTrue(TEXT("Snapshot sizes are positive"), node.Size.X > 0.0 && node.Size.Y > 0.0);

	FDialogueLayoutSnapshot again;
	FDialogueLayoutBinding againBinding;
	edGraph->BuildLayoutSnapshot(again, againBinding, error);
	TestEqual(TEXT("Capturing twice gives the same revision"), again.Revision, snapshot.Revision);

	FDialogueLayoutTaskState state;
	FMounteaLayeredLayout::Run(snapshot, state);
	const TSharedPtr<const FDialogueLayoutCheckpoint> checkpoint = state.GetLatestCheckpoint();
	if (!TestTrue(TEXT("Layout produced a checkpoint"), checkpoint.IsValid()))
		return false;

	// A mutation after the snapshot invalidates the result; nothing may be written.
	{
		const int32 savedOrder = dialogue.B->ExecutionOrder;
		dialogue.B->ExecutionOrder = 77;
		FString staleError;
		const FIntPoint startBefore(dialogue.Graph.EdNodeOf(dialogue.Graph.Start())->NodePosX, dialogue.Graph.EdNodeOf(dialogue.Graph.Start())->NodePosY);
		TestFalse(TEXT("A stale checkpoint is refused"), edGraph->ApplyLayoutCheckpoint(snapshot, binding, *checkpoint, staleError));
		TestFalse(TEXT("The refusal explains itself"), staleError.IsEmpty());
		TestEqual(TEXT("A refused checkpoint writes nothing"), FIntPoint(dialogue.Graph.EdNodeOf(dialogue.Graph.Start())->NodePosX, dialogue.Graph.EdNodeOf(dialogue.Graph.Start())->NodePosY), startBefore);
		dialogue.B->ExecutionOrder = savedOrder;
	}

	TestTrue(TEXT("The checkpoint applies"), edGraph->ApplyLayoutCheckpoint(snapshot, binding, *checkpoint, error));

	for (int32 i = 0; i < snapshot.Nodes.Num(); ++i)
	{
		const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(binding.Nodes[i].Get());
		TestEqual(TEXT("Editor X matches the checkpoint"), edNode->NodePosX, checkpoint->NodePositions[i].X);
		TestEqual(TEXT("Editor Y matches the checkpoint"), edNode->NodePosY, checkpoint->NodePositions[i].Y);
		TestEqual(TEXT("Runtime NodePosition matches the editor position"), edNode->DialogueGraphNode->NodePosition, FIntPoint(edNode->NodePosX, edNode->NodePosY));
	}

	const UEdNode_MounteaDialogueGraphNode* edStart = dialogue.Graph.EdNodeOf(dialogue.Graph.Start());
	TestEqual(TEXT("Start X is exactly 0"), edStart->NodePosX, 0);
	TestEqual(TEXT("Start Y is exactly 0"), edStart->NodePosY, 0);
	TestEqual(TEXT("Arranging never changes option order"), dialogue.Orders(), ordersBefore);

	// Connections: control positions and prepared routes.
	for (int32 i = 0; i < snapshot.Edges.Num(); ++i)
	{
		const UEdNode_MounteaDialogueGraphEdge* edge = Cast<UEdNode_MounteaDialogueGraphEdge>(binding.Edges[i].Get());
		TestEqual(TEXT("Control X"), edge->NodePosX, checkpoint->Routes[i].ControlPosition.X);
		TestEqual(TEXT("Control Y"), edge->NodePosY, checkpoint->Routes[i].ControlPosition.Y);
		TestNotNull(TEXT("Every connection has a prepared route"), edGraph->FindPreparedRoute(edge));
	}

	// Moving a node invalidates the routes that depended on it (and re-checks the others).
	UEdNode_MounteaDialogueGraphEdge* leadToA = Cast<UEdNode_MounteaDialogueGraphEdge>(binding.Edges[0].Get());
	UEdNode_MounteaDialogueGraphNode* movedNode = leadToA->GetEndNode();
	movedNode->NodePosX += 1234;
	++GFrameCounter;
	TestNull(TEXT("A connection to a moved node is no longer drawn from its stale route"), edGraph->FindPreparedRoute(leadToA));
	movedNode->NodePosX -= 1234;
	++GFrameCounter;
	TestNotNull(TEXT("Putting the node back restores the route"), edGraph->FindPreparedRoute(leadToA));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLayoutOrderingTest,
	"Mountea.DialogueSystemEditor.Layout.OrderingIsExplicit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLayoutOrderingTest::RunTest(const FString& Parameters)
{
	MounteaLayoutEditorTest::FDiamond dialogue;
	dialogue.Build(*this);
	UEdGraph_MounteaDialogueGraph* edGraph = dialogue.Graph.EdGraph;

	const TArray<int32> initial = dialogue.Orders();
	TestTrue(TEXT("Horizontal order was derived on creation"), initial[1] < initial[2] && initial[2] < initial[3]);

	// Ordinary modification, rebuilds and topology sync keep what exists.
	dialogue.Graph.EdNodeOf(dialogue.A)->NodePosX = 5000; // visually last now, but nobody asked to reorder
	edGraph->Modify(true);
	TestEqual(TEXT("Modify does not reorder"), dialogue.Orders(), initial);
	edGraph->RebuildMounteaDialogueGraph();
	TestEqual(TEXT("Rebuild does not reorder"), dialogue.Orders(), initial);
	edGraph->SyncTopology();
	TestEqual(TEXT("Undo synchronisation does not reorder"), dialogue.Orders(), initial);

	// Manual movement reorders, on purpose.
	TestTrue(TEXT("Explicit reordering reports a change"), edGraph->ReorderFromPositions());
	TestTrue(TEXT("Moved node now follows the others"), dialogue.A->ExecutionOrder > dialogue.C->ExecutionOrder);
	TestFalse(TEXT("A second pass has nothing left to do"), edGraph->ReorderFromPositions());

	// Missing orders are initialised after existing ones; existing ones stay.
	const TArray<int32> before = dialogue.Orders();
	dialogue.B->ExecutionOrder = INDEX_NONE;
	edGraph->EnsureExecutionOrders();
	TestTrue(TEXT("Missing order is initialised"), dialogue.B->ExecutionOrder != INDEX_NONE);
	TestEqual(TEXT("Existing orders are untouched"), dialogue.C->ExecutionOrder, before[3]);
	TestEqual(TEXT("Existing orders are untouched (A)"), dialogue.A->ExecutionOrder, before[1]);
	TestTrue(TEXT("New order sorts after the existing ones"), dialogue.B->ExecutionOrder > FMath::Max3(before[1], before[3], before[0]));

	// Cycles and joins must not hang or blow up the ordering traversal.
	dialogue.Graph.Link(dialogue.C, dialogue.Lead); // physical cycle in the runtime graph
	dialogue.Graph.Link(dialogue.B, dialogue.C);    // join
	edGraph->ReorderFromPositions();
	TestTrue(TEXT("Ordering survives cycles and joins"), true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaCulledWiresTest,
	"Mountea.DialogueSystemEditor.Layout.CulledWires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// Slate hands nodes outside the viewport a synthesized geometry: zero size, scale 1, correct top-left only.
// Routed wires must still be placed and zoomed correctly from that.
bool FMounteaCulledWiresTest::RunTest(const FString& Parameters)
{
	MounteaLayoutEditorTest::FDiamond dialogue;
	dialogue.Build(*this);
	UEdGraph_MounteaDialogueGraph* edGraph = dialogue.Graph.EdGraph;

	FDialogueLayoutSnapshot snapshot;
	FDialogueLayoutBinding binding;
	FString error;
	if (!TestTrue(TEXT("Snapshot"), edGraph->BuildLayoutSnapshot(snapshot, binding, error)))
		return false;
	FDialogueLayoutTaskState state;
	FMounteaLayeredLayout::Run(snapshot, state);
	const TSharedPtr<const FDialogueLayoutCheckpoint> checkpoint = state.GetLatestCheckpoint();
	if (!TestTrue(TEXT("Checkpoint"), checkpoint.IsValid()) || !TestTrue(TEXT("Applied"), edGraph->ApplyLayoutCheckpoint(snapshot, binding, *checkpoint, error)))
		return false;

	UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(binding.Edges[0].Get());
	UEdNode_MounteaDialogueGraphNode* startNode = edgeNode->GetStartNode();
	UEdNode_MounteaDialogueGraphNode* endNode = edgeNode->GetEndNode();
	const FPreparedDialogueRoute* route = edGraph->FindPreparedRoute(edgeNode);
	if (!TestNotNull(TEXT("The connection has a prepared route"), route))
		return false;
	const FPreparedDialogueRoute prepared = *route;

	const float zoom = 0.25f;
	const FSlateRect clip(-1.0e6f, -1.0e6f, 1.0e6f, 1.0e6f);
	FSlateWindowElementList elements(nullptr);
	FConnectionDrawingPolicy_MounteaDialogueGraph policy(0, 1, zoom, clip, elements, edGraph);

	const FVector2f origin(100.0f, 50.0f);
	const FGeometry culledStart = FGeometry::MakeRoot(FVector2f::ZeroVector, FSlateLayoutTransform(1.0f, origin));
	const FGeometry culledEnd = FGeometry::MakeRoot(FVector2f::ZeroVector, FSlateLayoutTransform(1.0f, FVector2f(900.0f, 400.0f)));
	const auto lineElements = [&elements]() -> const TArray<FSlateLineElement, FSlateStatTrackingMemoryAllocator<FDrawElementStatTracker>>&
	{
		return elements.GetUncachedDrawElements().Get<(uint8)EElementType::ET_Line>();
	};

	// Node -> control link: the whole routed wire.
	FConnectionParams mainParams;
	policy.DetermineWiringStyle(startNode->GetOutputPin(), edgeNode->GetInputPin(), mainParams);
	policy.DrawSplineWithArrow(culledStart, culledEnd, mainParams);
	if (!TestEqual(TEXT("The routed wire is one line"), lineElements().Num(), 1))
		return false;
	TestEqual(TEXT("No spline is created for culled nodes"), elements.GetUncachedDrawElements().Get<(uint8)EElementType::ET_Spline>().Num(), 0);

	const TArray<FVector2f>& points = lineElements()[0].GetPoints();
	const FVector2D sourcePosition(prepared.SourcePosition.X, prepared.SourcePosition.Y);
	const FVector2D expectedFirst = FVector2D(origin) + (prepared.Points[0] - sourcePosition) * zoom;
	const FVector2D expectedLast = FVector2D(origin) + (prepared.Points.Last() - sourcePosition) * zoom;
	TestTrue(TEXT("Wire starts at the zoomed source anchor, not at the node corner"), FVector2D(points[0]).Equals(expectedFirst, 0.05));
	TestTrue(TEXT("Wire ends at the zoomed target anchor"), FVector2D(points.Last()).Equals(expectedLast, 0.05));

	// Control -> node stub: covered by the route, must not draw a stray line from the corner.
	FConnectionParams stubParams;
	policy.DetermineWiringStyle(edgeNode->GetOutputPin(), endNode->GetInputPin(), stubParams);
	policy.DrawSplineWithArrow(culledStart, culledEnd, stubParams);
	TestEqual(TEXT("The stub link draws nothing while the route is valid"), lineElements().Num(), 1);

	// Without a route and without arranged widgets there is nothing trustworthy to draw: no wire from the corner.
	edGraph->ClearPreparedRoutes();
	policy.DrawSplineWithArrow(culledStart, culledEnd, mainParams);
	TestEqual(TEXT("A culled wire without a route is skipped instead of drawn from the corner"), lineElements().Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
