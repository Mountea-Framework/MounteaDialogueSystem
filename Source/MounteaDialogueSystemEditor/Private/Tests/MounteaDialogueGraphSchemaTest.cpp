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
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

// The graph editor: what the schema lets a user connect, and how the editor graph (EdGraph) is turned back into
// the runtime graph. Nodes are built without the asset editor UI, the way FAssetEditor_MounteaDialogueGraph does.

namespace MounteaSchemaTest
{
	static const UAssetGraphScheme_MounteaDialogueGraph* Schema(const UEdGraph* Graph)
	{
		return CastChecked<UAssetGraphScheme_MounteaDialogueGraph>(Graph->GetSchema());
	}

	static int32 CountEdgeNodes(const UEdGraph* Graph)
	{
		int32 count = 0;
		for (const UEdGraphNode* node : Graph->Nodes)
			count += node && node->IsA<UEdNode_MounteaDialogueGraphEdge>() ? 1 : 0;
		return count;
	}
}

// Pin-level rules: no self links, no input-to-input, no cycles, and node class rules apply whichever pin the
// user drags from.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaSchemaCanConnectTest,
	"Mountea.DialogueSystemEditor.Schema.CanCreateConnection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaSchemaCanConnectTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;

	FEditorTestGraph graph(TEXT("SchemaGraph"));
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
	UMounteaDialogueGraphNode_DialogueNode* editable = graph.AddNode<UMounteaDialogueGraphNode_DialogueNode>();
	graph.MakeEdGraph();
	const UAssetGraphScheme_MounteaDialogueGraph* schema = MounteaSchemaTest::Schema(graph.EdGraph);

	UEdNode_MounteaDialogueGraphNode* edStart = graph.EdNodeOf(graph.Start());
	UEdNode_MounteaDialogueGraphNode* edLead = graph.EdNodeOf(lead);
	UEdNode_MounteaDialogueGraphNode* edAnswer = graph.EdNodeOf(answer);
	UEdNode_MounteaDialogueGraphNode* edEditable = graph.EdNodeOf(editable);
	if (!TestTrue(TEXT("Precondition: every node has an EdNode"), edStart && edLead && edAnswer && edEditable))
		return false;

	const auto Allowed = [schema](UEdGraphPin* A, UEdGraphPin* B) { return schema->CanCreateConnection(A, B).Response != CONNECT_RESPONSE_DISALLOW; };

	TestTrue(TEXT("Start -> Lead is allowed"), Allowed(edStart->GetOutputPin(), edLead->GetInputPin()));
	TestTrue(TEXT("The same link dragged from the input side is allowed"), Allowed(edLead->GetInputPin(), edStart->GetOutputPin()));
	TestFalse(TEXT("A node cannot link to itself"), Allowed(edLead->GetOutputPin(), edLead->GetInputPin()));
	TestFalse(TEXT("Input to input is refused"), Allowed(edLead->GetInputPin(), edAnswer->GetInputPin()));
	TestFalse(TEXT("Output to output is refused"), Allowed(edLead->GetOutputPin(), edAnswer->GetOutputPin()));

	// Answer does not accept an editable Dialogue Node as input; the answer must not depend on drag direction.
	TestFalse(TEXT("Class rule, dragged from the output"), Allowed(edEditable->GetOutputPin(), edAnswer->GetInputPin()));
	TestFalse(TEXT("Class rule, dragged from the input"), Allowed(edAnswer->GetInputPin(), edEditable->GetOutputPin()));

	// Lead -> Answer exists; Answer -> Lead would close a loop.
	TestTrue(TEXT("Lead -> Answer is created"), schema->TryCreateConnection(edLead->GetOutputPin(), edAnswer->GetInputPin()));
	TestFalse(TEXT("A cycle is refused"), Allowed(edAnswer->GetOutputPin(), edLead->GetInputPin()));
	return true;
}

// Connections go through an edge node (which carries conditions); a duplicate link is refused, and rebuilding
// the runtime graph mirrors the editor links.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaSchemaTryConnectTest,
	"Mountea.DialogueSystemEditor.Schema.ConnectionsCreateEdgesAndRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaSchemaTryConnectTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;

	FEditorTestGraph graph(TEXT("RebuildGraph"));
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_AnswerNode* answerA = graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
	UMounteaDialogueGraphNode_AnswerNode* answerB = graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
	graph.MakeEdGraph();
	const UAssetGraphScheme_MounteaDialogueGraph* schema = MounteaSchemaTest::Schema(graph.EdGraph);

	UEdNode_MounteaDialogueGraphNode* edStart = graph.EdNodeOf(graph.Start());
	UEdNode_MounteaDialogueGraphNode* edLead = graph.EdNodeOf(lead);
	UEdNode_MounteaDialogueGraphNode* edA = graph.EdNodeOf(answerA);
	UEdNode_MounteaDialogueGraphNode* edB = graph.EdNodeOf(answerB);
	edA->NodePosX = 0;
	edB->NodePosX = 400;

	TestTrue(TEXT("Start -> Lead"), schema->TryCreateConnection(edStart->GetOutputPin(), edLead->GetInputPin()));
	TestTrue(TEXT("Lead -> A"), schema->TryCreateConnection(edLead->GetOutputPin(), edA->GetInputPin()));
	TestTrue(TEXT("Lead -> B (dragged from the input)"), schema->TryCreateConnection(edB->GetInputPin(), edLead->GetOutputPin()));
	TestFalse(TEXT("A second Lead -> A is refused"), schema->TryCreateConnection(edLead->GetOutputPin(), edA->GetInputPin()));
	TestEqual(TEXT("One edge node per link"), MounteaSchemaTest::CountEdgeNodes(graph.EdGraph), 3);

	graph.EdGraph->RebuildMounteaDialogueGraph();

	TestTrue(TEXT("Runtime Start -> Lead"), graph.Start()->ChildrenNodes == TArray<UMounteaDialogueGraphNode*>({ lead }));
	TestEqual(TEXT("Runtime Lead has both answers"), lead->ChildrenNodes.Num(), 2);
	TestTrue(TEXT("Runtime parents are set"), answerA->ParentNodes.Contains(lead) && answerB->ParentNodes.Contains(lead));
	TestNotNull(TEXT("Runtime edges exist for each link"), lead->Edges.FindRef(answerA));
	TestTrue(TEXT("Execution order follows horizontal position"), answerA->ExecutionOrder < answerB->ExecutionOrder);
	TestTrue(TEXT("The Start node is the root"), graph.Graph->GetRootNodes().Contains(graph.Start()));
	TestTrue(TEXT("Every node is listed"), graph.Graph->GetAllNodes().Num() == 4);

	// Breaking the Lead's links removes them from the runtime graph on the next rebuild.
	schema->BreakNodeLinks(*edLead);
	graph.EdGraph->RebuildMounteaDialogueGraph();
	TestEqual(TEXT("After breaking, the Lead has no children"), lead->ChildrenNodes.Num(), 0);
	TestEqual(TEXT("After breaking, the Lead has no parents"), lead->ParentNodes.Num(), 0);
	return true;
}

// Old assets may hold direct pin links or duplicate edge nodes; normalisation turns them into one edge node per link.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaSchemaNormalizeTest,
	"Mountea.DialogueSystemEditor.Schema.NormalizeEdgeNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaSchemaNormalizeTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;

	FEditorTestGraph graph(TEXT("NormalizeGraph"));
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
	graph.MakeEdGraph();
	UEdNode_MounteaDialogueGraphNode* edLead = graph.EdNodeOf(lead);
	UEdNode_MounteaDialogueGraphNode* edAnswer = graph.EdNodeOf(answer);

	// A legacy direct pin link, no edge node.
	edLead->GetOutputPin()->MakeLinkTo(edAnswer->GetInputPin());

	int32 migrated = 0;
	int32 removed = 0;
	TestTrue(TEXT("Migration modifies the graph"), graph.EdGraph->NormalizeEdgeNodes(true, migrated, removed));
	TestEqual(TEXT("The direct link is migrated"), migrated, 1);
	TestFalse(TEXT("No direct link remains"), edLead->GetOutputPin()->LinkedTo.Contains(edAnswer->GetInputPin()));
	TestEqual(TEXT("It became an edge node"), MounteaSchemaTest::CountEdgeNodes(graph.EdGraph), 1);

	// A second edge node for the same pair is a duplicate.
	UEdNode_MounteaDialogueGraphEdge* duplicate = graph.EdGraph->CreateEdgeNode(edLead, edAnswer);
	graph.EdGraph->AddNode(duplicate, false, false);
	TestEqual(TEXT("Precondition: two edge nodes"), MounteaSchemaTest::CountEdgeNodes(graph.EdGraph), 2);

	TestTrue(TEXT("Deduplication modifies the graph"), graph.EdGraph->NormalizeEdgeNodes(false, migrated, removed));
	TestEqual(TEXT("One duplicate is removed"), removed, 1);
	TestEqual(TEXT("One edge node is left"), MounteaSchemaTest::CountEdgeNodes(graph.EdGraph), 1);

	TestFalse(TEXT("A clean graph is left unchanged"), graph.EdGraph->NormalizeEdgeNodes(true, migrated, removed));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
