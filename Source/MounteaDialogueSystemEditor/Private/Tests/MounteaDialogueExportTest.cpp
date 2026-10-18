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
#include "Conditions/MounteaDialogueConditionBase.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"

// Export of graphs built in memory (no assets), read back through the archive reader. The golden-file import ->
// export path is covered by MounteaDialogueArchiveEndToEndTest; this covers graphs authored in Unreal.

namespace MounteaExportTest
{
	const MounteaArchive::FNode* FindNodeOf(const MounteaArchive::FDialogueArchive& Archive, const UMounteaDialogueGraphNode* Node)
	{
		const FString id = MounteaArchive::GuidToId(Node->GetNodeGUID());
		return Archive.Nodes.FindByPredicate([&id](const MounteaArchive::FNode& Record) { return Record.Id == id; });
	}
}

// Every node type, row timing, edge order, edge conditions, Delay duration and Return target survive export.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaExportGraphStructureTest,
	"Mountea.DialogueSystemEditor.Export.GraphStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaExportGraphStructureTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;
	using namespace MounteaArchive;

	FEditorTestGraph graph(TEXT("ExportGraph"));
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddNode<UMounteaDialogueGraphNode_LeadNode>(graph.AddRow(TEXT("Lead"), {
		Line(TEXT("First line"), ERowExecutionMode::EREM_Automatic, ERowDurationMode::ERDM_Duration, 2.f),
		Line(TEXT("Second line"), ERowExecutionMode::EREM_AwaitInput, ERowDurationMode::EDRM_Add, 0.f, 1.5f),
		Line(TEXT("Third line"), ERowExecutionMode::EREM_Stopping, ERowDurationMode::ERDM_AutoCalculate) }));
	UMounteaDialogueGraphNode_AnswerNode* answerA = graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>(graph.AddRow(TEXT("A"), { Line(TEXT("Yes")) }, TEXT("Player")));
	UMounteaDialogueGraphNode_AnswerNode* answerB = graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>(graph.AddRow(TEXT("B"), { Line(TEXT("No")) }, TEXT("Player")));
	UMounteaDialogueGraphNode_Delay* delay = graph.AddNode<UMounteaDialogueGraphNode_Delay>();
	delay->SetDelayDuration(2);
	UMounteaDialogueGraphNode_CompleteNode* complete = graph.AddNode<UMounteaDialogueGraphNode_CompleteNode>(graph.AddRow(TEXT("Bye"), { Line(TEXT("Bye")) }));
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = graph.AddNode<UMounteaDialogueGraphNode_ReturnToNode>();
	returnNode->SelectedNode = lead;

	graph.Link(graph.Start(), lead);
	graph.Link(lead, answerA);
	UMounteaDialogueGraphEdge* conditional = graph.Link(lead, answerB);
	graph.Link(answerA, delay);
	graph.Link(delay, complete);
	graph.Link(answerB, returnNode);

	FMounteaDialogueCondition rule;
	rule.ConditionClass = NewObject<UMounteaDialogueConditionBase>(conditional);
	rule.bNegate = true;
	conditional->EdgeConditions.Rules.Add(rule);
	conditional->EdgeConditions.Mode = EConditionEvaluationMode::Any;

	FScopedTempDir temp(TEXT("Export"));
	FDialogueArchive archive;
	if (!ExportAndRead(*this, graph.Graph, temp.File(TEXT("Graph.mnteadlg")), archive))
		return false;

	TestTrue(TEXT("A single graph exports as a dialogue archive"), archive.Kind == EKind::Dialogue);
	TestEqual(TEXT("The root dialogue is the graph"), archive.RootDialogueId, GuidToId(graph.Graph->GetGraphGUID()));
	TestEqual(TEXT("One dialogue"), archive.Dialogues.Num(), 1);
	TestEqual(TEXT("Start plus six nodes"), archive.Nodes.Num(), 7);
	TestEqual(TEXT("Six edges"), archive.Edges.Num(), 6);

	const FString leadId = GuidToId(lead->GetNodeGUID());
	TArray<FString> leadTargets;
	for (const FArchiveEdge& edge : archive.Edges)
	{
		if (edge.SourceId == leadId)
			leadTargets.Add(edge.TargetId);
	}
	TestTrue(TEXT("Sibling order is the edge order"), leadTargets == TArray<FString>({ GuidToId(answerA->GetNodeGUID()), GuidToId(answerB->GetNodeGUID()) }));

	const FArchiveEdge* conditionalEdge = archive.Edges.FindByPredicate([&](const FArchiveEdge& Edge) { return Edge.TargetId == GuidToId(answerB->GetNodeGUID()); });
	if (TestNotNull(TEXT("The conditional edge is exported"), conditionalEdge))
	{
		TestEqual(TEXT("Its mode is kept"), conditionalEdge->ConditionMode, FString(TEXT("any")));
		TestTrue(TEXT("Its negated rule is kept"), conditionalEdge->Rules.Num() == 1 && conditionalEdge->Rules[0].bNegate);
	}

	if (const FNode* leadRecord = MounteaExportTest::FindNodeOf(archive, lead); TestNotNull(TEXT("Lead exported"), leadRecord))
	{
		TestEqual(TEXT("Lead type"), leadRecord->Type, FString(Contract::NodeLead));
		TestEqual(TEXT("Lead participant"), leadRecord->ParticipantName, FString(TEXT("NPC")));
		if (TestEqual(TEXT("Three rows"), leadRecord->Rows.Num(), 3))
		{
			TestTrue(TEXT("Row 1 timing"), leadRecord->Rows[0].ExecutionBehaviour == ERowExecutionMode::EREM_Automatic && FMath::IsNearlyEqual(leadRecord->Rows[0].Duration, 2.f));
			TestTrue(TEXT("Row 2 timing"), leadRecord->Rows[1].ExecutionBehaviour == ERowExecutionMode::EREM_AwaitInput
				&& leadRecord->Rows[1].DurationMode == ERowDurationMode::EDRM_Add && FMath::IsNearlyEqual(leadRecord->Rows[1].DurationOverride, 1.5f));
			TestTrue(TEXT("Row 3 timing"), leadRecord->Rows[2].ExecutionBehaviour == ERowExecutionMode::EREM_Stopping
				&& leadRecord->Rows[2].DurationMode == ERowDurationMode::ERDM_AutoCalculate);
			const FLocalizedString* text = archive.FindString(leadRecord->Rows[1].TextKey);
			TestTrue(TEXT("Row text is exported in the default locale"),
				text && text->Values.FindRef(archive.Project.Localization.DefaultLocale) == TEXT("Second line"));
		}
	}

	if (const FNode* delayRecord = MounteaExportTest::FindNodeOf(archive, delay); TestNotNull(TEXT("Delay exported"), delayRecord))
		TestTrue(TEXT("Delay keeps its duration"), delayRecord->bHasDuration && FMath::IsNearlyEqual(delayRecord->Duration, 2.f));

	if (const FNode* returnRecord = MounteaExportTest::FindNodeOf(archive, returnNode); TestNotNull(TEXT("Return exported"), returnRecord))
		TestEqual(TEXT("Return keeps its target"), returnRecord->TargetNode, leadId);

	TestNotNull(TEXT("Participants are collected (NPC)"), archive.Participants.FindByPredicate([](const FParticipant& P) { return P.Name == TEXT("NPC"); }));
	TestNotNull(TEXT("Participants are collected (Player)"), archive.Participants.FindByPredicate([](const FParticipant& P) { return P.Name == TEXT("Player"); }));
	TestEqual(TEXT("The base condition is exported as one definition"), archive.Conditions.Num(), 1);
	return true;
}

// Open Child Graph pulls its target into the archive, otherwise the node would point nowhere.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaExportChildGraphTest,
	"Mountea.DialogueSystemEditor.Export.ChildGraphsAreIncluded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaExportChildGraphTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;
	using namespace MounteaArchive;

	FEditorTestGraph child(TEXT("ChildGraph"));
	child.Link(child.Start(), child.AddNode<UMounteaDialogueGraphNode_LeadNode>(child.AddRow(TEXT("ChildLead"), { Line(TEXT("From the child")) })));

	FEditorTestGraph parent(TEXT("ParentGraph"));
	UMounteaDialogueGraphNode_LeadNode* lead = parent.AddNode<UMounteaDialogueGraphNode_LeadNode>(parent.AddRow(TEXT("Lead"), { Line(TEXT("Hello")) }));
	UMounteaDialogueGraphNode_OpenChildGraph* open = parent.AddNode<UMounteaDialogueGraphNode_OpenChildGraph>();
	open->TargetDialogue = child.Graph;
	parent.Link(parent.Start(), lead);
	parent.Link(lead, open);

	FScopedTempDir temp(TEXT("ExportChild"));
	FDialogueArchive archive;
	if (!ExportAndRead(*this, parent.Graph, temp.File(TEXT("Parent.mnteadlg")), archive))
		return false;

	TestEqual(TEXT("Parent and child are both exported"), archive.Dialogues.Num(), 2);
	TestEqual(TEXT("The parent stays the root"), archive.RootDialogueId, GuidToId(parent.Graph->GetGraphGUID()));
	if (const FNode* openRecord = MounteaExportTest::FindNodeOf(archive, open); TestNotNull(TEXT("Open node exported"), openRecord))
		TestEqual(TEXT("It targets the child dialogue"), openRecord->TargetDialogue, GuidToId(child.Graph->GetGraphGUID()));
	return true;
}

// Several graphs export as one project archive without a root.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaExportProjectTest,
	"Mountea.DialogueSystemEditor.Export.Project",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaExportProjectTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;
	using namespace MounteaArchive;

	FEditorTestGraph first(TEXT("First"));
	first.Link(first.Start(), first.AddNode<UMounteaDialogueGraphNode_LeadNode>(first.AddRow(TEXT("One"), { Line(TEXT("One")) })));
	FEditorTestGraph second(TEXT("Second"));
	second.Link(second.Start(), second.AddNode<UMounteaDialogueGraphNode_LeadNode>(second.AddRow(TEXT("Two"), { Line(TEXT("Two")) })));

	FScopedTempDir temp(TEXT("ExportProject"));
	const FString path = temp.File(TEXT("Both.mnteadlgproj"));
	if (!TestTrue(TEXT("Project export succeeds"), UMounteaDialogueSystemImportExportHelpers::ExportProject({ first.Graph, nullptr, second.Graph }, path)))
		return false;

	FDialogueArchive archive;
	FString message;
	if (!TestTrue(TEXT("The project reads back"), UMounteaDialogueSystemImportExportHelpers::ReadArchiveFile(path, archive, message)))
		return false;

	TestTrue(TEXT("Kind is project"), archive.Kind == EKind::Project);
	TestEqual(TEXT("Both dialogues, nulls skipped"), archive.Dialogues.Num(), 2);
	TestEqual(TEXT("The project is named after the file"), archive.Project.Name, FString(TEXT("Both")));
	TestTrue(TEXT("Dialogue slugs are unique"), archive.Dialogues[0].Slug != archive.Dialogues[1].Slug);
	return true;
}

// Things that cannot be exported are refused with a reason instead of producing a broken archive.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaExportRefusalsTest,
	"Mountea.DialogueSystemEditor.Export.Refusals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaExportRefusalsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;

	AddExpectedError(TEXT("Invalid Graph object provided for export"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("UNSUPPORTED_NODE_TYPE"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("NOTHING_TO_EXPORT"), EAutomationExpectedErrorFlags::Contains, 1);

	FScopedTempDir temp(TEXT("ExportRefusals"));
	TestFalse(TEXT("A null graph is refused"), UMounteaDialogueSystemImportExportHelpers::ExportDialogueGraph(nullptr, temp.File(TEXT("Null.mnteadlg"))));

	// The editable Dialogue Node has no Dialoguer counterpart.
	FEditorTestGraph graph(TEXT("Unsupported"));
	graph.Link(graph.Start(), graph.AddNode<UMounteaDialogueGraphNode_DialogueNode>());
	const FString path = temp.File(TEXT("Unsupported.mnteadlg"));
	TestFalse(TEXT("A graph with an unsupported node is refused"), UMounteaDialogueSystemImportExportHelpers::ExportDialogueGraph(graph.Graph, path));
	TestFalse(TEXT("No file is written"), FPaths::FileExists(path));

	TestFalse(TEXT("An empty project is refused"), UMounteaDialogueSystemImportExportHelpers::ExportProject({}, temp.File(TEXT("Empty.mnteadlgproj"))));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
