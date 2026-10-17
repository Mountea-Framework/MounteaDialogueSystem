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

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR // editor-only: needs an editor world, kept out of game builds

#include "Tests/MounteaDialogueTestFixtures.h"
#include "Decorators/MounteaDialogueDecorator_SendCommand.h"
#include "Decorators/MounteaIDialogueDecorator_SwapParticipants.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

// The dialogue graph asset: lookups, start readiness and the editor validation shown on save. Graph-type
// validation depends on the project's Dialogue Configuration and is covered by MounteaDialogueSettingsTest.

// Every new graph gets exactly one Start node, and node lookups cover both the Start node and the rest.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGraphStructureTest,
	"Mountea.DialogueSystem.Graph.Structure.NewGraphAndLookups",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGraphStructureTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode* start = graph.Start();
	if (!TestNotNull(TEXT("A new graph creates its Start node"), start))
		return false;

	TestTrue(TEXT("The Start node is a Start node"), start->IsA<UMounteaDialogueGraphNode_StartNode>());
	TestTrue(TEXT("The Start node is listed in all nodes"), graph.Graph->GetAllNodes().Contains(start));
	TestTrue(TEXT("The Start node is a root"), graph.Graph->GetRootNodes().Contains(start));
	TestTrue(TEXT("The Start node knows its graph"), start->GetGraph() == graph.Graph);
	TestTrue(TEXT("A new graph has a GUID"), graph.Graph->GetGraphGUID().IsValid());

	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	TestTrue(TEXT("FindNodeByGuid finds the Start node"), graph.Graph->FindNodeByGuid(start->GetNodeGUID()) == start);
	TestTrue(TEXT("FindNodeByGuid finds other nodes"), graph.Graph->FindNodeByGuid(lead->GetNodeGUID()) == lead);
	TestNull(TEXT("FindNodeByGuid returns null for unknown GUIDs"), graph.Graph->FindNodeByGuid(FGuid::NewGuid()));

	const FGuid newGuid = FGuid::NewGuid();
	graph.Graph->SetGraphGUID(newGuid);
	TestEqual(TEXT("Nodes report their graph's GUID"), lead->GetGraphGUID(), newGuid);

	// A second CreateGraph must not add a second Start node (it runs on every PostInitProperties).
	graph.Graph->CreateGraph();
	TestTrue(TEXT("CreateGraph keeps the existing Start node"), graph.Start() == start);
	return true;
}

// ClearGraph is what reimport uses before rebuilding; links on the Start node must not survive it (otherwise
// every reimport adds the first edge again).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGraphClearTest,
	"Mountea.DialogueSystem.Graph.Structure.ClearGraphDropsAllLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGraphClearTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	graph.Link(graph.Start(), lead);

	// Reimport may drop the Start node from AllNodes; its links must still be cleared.
	graph.Graph->AllNodes.Remove(graph.Start());
	graph.Graph->ClearGraph();

	TestEqual(TEXT("No nodes are listed"), graph.Graph->GetAllNodes().Num(), 0);
	TestEqual(TEXT("No root nodes are listed"), graph.Graph->GetRootNodes().Num(), 0);
	TestEqual(TEXT("The Start node has no children"), graph.Start()->ChildrenNodes.Num(), 0);
	TestEqual(TEXT("The Start node has no edges"), graph.Start()->Edges.Num(), 0);
	TestEqual(TEXT("Former children have no parents"), lead->ParentNodes.Num(), 0);
	return true;
}

// CanStartDialogueGraph is the runtime gate a participant checks before starting.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGraphCanStartTest,
	"Mountea.DialogueSystem.Graph.Structure.CanStartRequiresValidNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGraphCanStartTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph valid;
	valid.Link(valid.Start(), valid.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead")));
	TestTrue(TEXT("A graph whose speech nodes have rows can start"), valid.Graph->CanStartDialogueGraph());

	FTestGraph noRow;
	noRow.Link(noRow.Start(), noRow.AddNode<UMounteaDialogueGraphNode_LeadNode>());
	TestFalse(TEXT("A speech node without a DataTable blocks the start"), noRow.Graph->CanStartDialogueGraph());

	FTestGraph missingRow;
	UMounteaDialogueGraphNode_LeadNode* lead = missingRow.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	lead->SetRowName(TEXT("DoesNotExist"));
	missingRow.Link(missingRow.Start(), lead);
	TestFalse(TEXT("A row name missing from the table blocks the start"), missingRow.Graph->CanStartDialogueGraph());

	FTestGraph emptyRow;
	UMounteaDialogueGraphNode_LeadNode* emptyLead = emptyRow.AddNode<UMounteaDialogueGraphNode_LeadNode>(emptyRow.AddRow(TEXT("Empty"), {}));
	emptyRow.Link(emptyRow.Start(), emptyLead);
	TestFalse(TEXT("A row without lines blocks the start"), emptyRow.Graph->CanStartDialogueGraph());

	FTestGraph cleared;
	cleared.Graph->ClearGraph();
	TestFalse(TEXT("A graph without nodes cannot start"), cleared.Graph->CanStartDialogueGraph());
	return true;
}

// The Start node may (in this version) have exactly one child.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGraphValidateStartTest,
	"Mountea.DialogueSystem.Graph.Validation.StartNodeNeedsExactlyOneChild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGraphValidateStartTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	{
		FTestGraph graph;
		FDataValidationContext context;
		TestFalse(TEXT("[No child] invalid"), graph.Graph->ValidateAllNodes(context, false));
		TestTrue(TEXT("[No child] says so"), JoinIssues(context).Contains(TEXT("Does not have any Children Nodes")));
	}
	{
		FTestGraph graph;
		graph.Link(graph.Start(), graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("A")));
		graph.Link(graph.Start(), graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("B")));
		FDataValidationContext context;
		TestFalse(TEXT("[Two children] invalid"), graph.Graph->ValidateAllNodes(context, false));
		TestTrue(TEXT("[Two children] says so"), JoinIssues(context).Contains(TEXT("more than 1 Child Node")));
	}
	{
		FTestGraph graph;
		graph.Link(graph.Start(), graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("A")));
		FDataValidationContext context;
		TestTrue(TEXT("[One valid child] valid"), graph.Graph->ValidateAllNodes(context, false));
		TestEqual(TEXT("[One valid child] no messages"), context.GetIssues().Num(), 0);
	}
	{
		FTestGraph graph;
		graph.Graph->StartNode = nullptr;
		FDataValidationContext context;
		TestFalse(TEXT("[No Start node] invalid"), graph.Graph->ValidateStartNode(context, false));
		TestTrue(TEXT("[No Start node] says so"), JoinIssues(context).Contains(TEXT("Has no Start Node")));
	}
	return true;
}

// Disconnected nodes and speech nodes without data are reported with the node title, in both plain and rich text.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGraphValidateNodesTest,
	"Mountea.DialogueSystem.Graph.Validation.NodeProblemsAreReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGraphValidateNodesTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	graph.Link(graph.Start(), graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead")));
	UMounteaDialogueGraphNode_AnswerNode* orphan = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Orphan"));
	UMounteaDialogueGraphNode_LeadNode* noData = graph.AddNode<UMounteaDialogueGraphNode_LeadNode>();
	graph.Link(orphan, noData);
	orphan->SetNodeTitle(FText::FromString(TEXT("OrphanTitle")));

	FDataValidationContext plain;
	TestFalse(TEXT("Invalid"), graph.Graph->ValidateAllNodes(plain, false));
	const FString plainText = JoinIssues(plain);
	TestTrue(TEXT("A node without parents is reported as needing inputs"), plainText.Contains(TEXT("OrphanTitle: This Node requires Inputs")));
	TestTrue(TEXT("A speech node without a DataTable is reported"), plainText.Contains(TEXT("Does not contain any Dialogue Data Table")));
	TestTrue(TEXT("A speech node without a row is reported"), plainText.Contains(TEXT("Does not contain valid Dialogue Row")));
	TestFalse(TEXT("Plain text has no rich-text markup"), plainText.Contains(TEXT("<RichTextBlock.Bold>")));

	FDataValidationContext rich;
	graph.Graph->ValidateAllNodes(rich, true);
	TestTrue(TEXT("Rich text highlights the node title"), JoinIssues(rich).Contains(TEXT("<RichTextBlock.Bold>OrphanTitle</>")));

	UMounteaDialogueGraphNode_LeadNode* isolated = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Isolated"));
	isolated->SetNodeTitle(FText::FromString(TEXT("IsolatedTitle")));
	FDataValidationContext isolatedContext;
	isolated->ValidateNode(isolatedContext, false);
	TestTrue(TEXT("A node with no links at all is reported"), JoinIssues(isolatedContext).Contains(TEXT("IsolatedTitle: This Node has no Connections")));
	return true;
}

// Graph decorators: empty slots and duplicates of non-stackable decorators are errors; stackable ones may repeat.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGraphValidateDecoratorsTest,
	"Mountea.DialogueSystem.Graph.Validation.GraphDecorators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGraphValidateDecoratorsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	TArray<FMounteaDialogueDecorator> decorators;

	decorators.Add(FMounteaDialogueDecorator());
	{
		FDataValidationContext context;
		TestFalse(TEXT("An empty decorator slot is invalid"), graph.Graph->ValidateDecorators(context, false, decorators, TEXT("Graph Decorators")));
		TestTrue(TEXT("It names the slot"), JoinIssues(context).Contains(TEXT("invalid Graph Decorators Decorator at Index: 0")));
	}

	decorators.Reset();
	UMounteaDialogueDecorator_SendCommand* commandA = AddDecorator<UMounteaDialogueDecorator_SendCommand>(graph.Graph, decorators);
	UMounteaDialogueDecorator_SendCommand* commandB = AddDecorator<UMounteaDialogueDecorator_SendCommand>(graph.Graph, decorators);
	SetProperty(*this, commandA, TEXT("Command"), FString(TEXT("A")));
	SetProperty(*this, commandB, TEXT("Command"), FString(TEXT("B")));
	{
		FDataValidationContext context;
		TestTrue(TEXT("Stackable decorators may repeat"), graph.Graph->ValidateDecorators(context, false, decorators, TEXT("Graph Decorators")));
	}

	AddDecorator<UMounteaDialogueDecorator_SwapParticipants>(graph.Graph, decorators);
	AddDecorator<UMounteaDialogueDecorator_SwapParticipants>(graph.Graph, decorators);
	{
		FDataValidationContext context;
		TestFalse(TEXT("A non-stackable decorator may not repeat"), graph.Graph->ValidateDecorators(context, false, decorators, TEXT("Graph Decorators")));
		TestTrue(TEXT("It reports the count"), JoinIssues(context).Contains(TEXT("2x times")));
	}

	TMap<UMounteaDialogueDecoratorBase*, int32> duplicates;
	TArray<UMounteaDialogueDecoratorBase*> used;
	for (const FMounteaDialogueDecorator& entry : decorators)
		used.Add(entry.DecoratorType);
	graph.Graph->FindDuplicatedDecorators(used, duplicates);
	TestEqual(TEXT("Both duplicate instances are listed, the stackable ones are not"), duplicates.Num(), 2);
	return true;
}

// Node decorators are validated per node; the plain-text duplicate message must name the node once.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGraphValidateNodeDecoratorsTest,
	"Mountea.DialogueSystem.Graph.Validation.NodeDecorators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGraphValidateNodeDecoratorsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	graph.Link(graph.Start(), lead);
	lead->SetNodeTitle(FText::FromString(TEXT("LeadTitle")));

	AddDecorator<UMounteaDialogueDecorator_SwapParticipants>(lead, lead->NodeDecorators);
	AddDecorator<UMounteaDialogueDecorator_SwapParticipants>(lead, lead->NodeDecorators);
	lead->NodeDecorators.Add(FMounteaDialogueDecorator());

	FDataValidationContext context;
	TestFalse(TEXT("Invalid"), lead->ValidateNode(context, false));
	const FString text = JoinIssues(context);
	TestTrue(TEXT("The empty slot is reported with its index"), text.Contains(TEXT("INVALID Node Decorator at Index: 2")));
	TestTrue(TEXT("The duplicate is reported"), text.Contains(TEXT("2x times")));
	TestFalse(TEXT("The duplicate message names the node once, not twice"), text.Contains(TEXT("LeadTitleLeadTitle")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
