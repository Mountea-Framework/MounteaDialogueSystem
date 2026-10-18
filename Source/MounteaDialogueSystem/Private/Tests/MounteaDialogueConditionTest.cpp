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
#include "Conditions/MounteaDialogueCondition_OnlyFirstTime.h"
#include "Helpers/MounteaDialogueConditionsStatics.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

// Edge conditions gate which children are offered. The base condition is always true, so "true" and "false"
// rules are built from it with and without bNegate.

namespace MounteaDialogueConditionTest
{
	static FMounteaDialogueCondition Rule(UObject* Outer, const bool bResult)
	{
		FMounteaDialogueCondition rule;
		rule.ConditionClass = NewObject<UMounteaDialogueConditionBase>(Outer);
		rule.bNegate = !bResult;
		return rule;
	}

	static UMounteaDialogueGraphEdge* Edge(const EConditionEvaluationMode Mode, const TArray<FMounteaDialogueCondition>& Rules)
	{
		UMounteaDialogueGraphEdge* edge = NewObject<UMounteaDialogueGraphEdge>(GetTransientPackage());
		edge->EdgeConditions.Mode = Mode;
		edge->EdgeConditions.Rules = Rules;
		return edge;
	}
}

// All / Any semantics, negation, and the "no rules means open" default.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueConditionEdgeRulesTest,
	"Mountea.DialogueSystem.Condition.EdgeRulesAllAnyNegate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueConditionEdgeRulesTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueConditionTest;
	using EMode = EConditionEvaluationMode;

	UObject* outer = GetTransientPackage();
	UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(outer);
	const auto Eval = [context](const UMounteaDialogueGraphEdge* InEdge)
	{
		return UMounteaDialogueConditionsStatics::EvaluateEdgeConditions(InEdge, context);
	};

	TestTrue(TEXT("No edge: open"), Eval(nullptr));
	TestTrue(TEXT("No rules: open"), Eval(Edge(EMode::All, {})));
	TestTrue(TEXT("No rules (Any): open"), Eval(Edge(EMode::Any, {})));

	TestTrue(TEXT("All of [true, true]"), Eval(Edge(EMode::All, { Rule(outer, true), Rule(outer, true) })));
	TestFalse(TEXT("All of [true, false]"), Eval(Edge(EMode::All, { Rule(outer, true), Rule(outer, false) })));
	TestTrue(TEXT("Any of [false, true]"), Eval(Edge(EMode::Any, { Rule(outer, false), Rule(outer, true) })));
	TestFalse(TEXT("Any of [false, false]"), Eval(Edge(EMode::Any, { Rule(outer, false), Rule(outer, false) })));

	FMounteaDialogueCondition emptyRule;
	TestTrue(TEXT("All ignores rules without a condition"), Eval(Edge(EMode::All, { emptyRule, Rule(outer, true) })));
	TestTrue(TEXT("Any ignores rules without a condition"), Eval(Edge(EMode::Any, { emptyRule, Rule(outer, true) })));

	TestFalse(TEXT("A null condition evaluates false"), UMounteaDialogueConditionsStatics::EvaluateCondition(nullptr, context));
	TestTrue(TEXT("The base condition evaluates true"), UMounteaDialogueConditionsStatics::EvaluateCondition(NewObject<UMounteaDialogueConditionBase>(outer), context));
	TestEqual(TEXT("A null condition is named 'none'"), UMounteaDialogueConditionsStatics::GetConditionName(nullptr), FString(TEXT("none")));
	return true;
}

// Only First Time closes an edge once its target node is in the traversed path, either of the running session
// or remembered by the graph owner from earlier sessions. Negated, it opens the "repeat visit" branch instead.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueConditionOnlyFirstTimeTest,
	"Mountea.DialogueSystem.Condition.OnlyFirstTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueConditionOnlyFirstTimeTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));
	UMounteaDialogueGraphEdge* edge = graph.Link(lead, answer);

	FMounteaDialogueCondition rule;
	rule.ConditionClass = NewObject<UMounteaDialogueCondition_OnlyFirstTime>(edge);
	edge->EdgeConditions.Rules.Add(rule);

	UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(GetTransientPackage());
	const auto Open = [edge, context]() { return UMounteaDialogueConditionsStatics::EvaluateEdgeConditions(edge, context); };

	TestTrue(TEXT("Open before the target was visited"), Open());

	context->AddTraversedNode(lead);
	TestTrue(TEXT("Visiting a different node does not close it"), Open());

	context->AddTraversedNode(answer);
	TestFalse(TEXT("Closed once the target is in the session path"), Open());

	edge->EdgeConditions.Rules[0].bNegate = true;
	TestTrue(TEXT("Negated: the repeat branch opens"), Open());

	TestTrue(TEXT("Without a context the condition stays open"), rule.ConditionClass->EvaluateCondition(nullptr));

	// A condition not owned by an edge has no target and stays open.
	TestTrue(TEXT("Unowned condition stays open"), NewObject<UMounteaDialogueCondition_OnlyFirstTime>(GetTransientPackage())->EvaluateCondition(context));
	return true;
}

// Remembered visits from earlier sessions live on the graph owner (the NPC participant).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueConditionOnlyFirstTimeRememberedTest,
	"Mountea.DialogueSystem.Condition.OnlyFirstTimeUsesGraphOwnerMemory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueConditionOnlyFirstTimeRememberedTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));
	graph.Link(graph.Start(), lead);
	UMounteaDialogueGraphEdge* edge = graph.Link(lead, answer);
	FMounteaDialogueCondition rule;
	rule.ConditionClass = NewObject<UMounteaDialogueCondition_OnlyFirstTime>(edge);
	edge->EdgeConditions.Rules.Add(rule);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(GetTransientPackage());
	TScriptInterface<IMounteaDialogueParticipantInterface> npc;
	npc.SetObject(fixture.NpcParticipant);
	npc.SetInterface(fixture.NpcParticipant);
	context->DialogueParticipants.Add(npc);

	TestTrue(TEXT("Open while the NPC has no memory of it"), UMounteaDialogueConditionsStatics::EvaluateEdgeConditions(edge, context));

	TArray<FDialogueTraversePath> remembered = { FDialogueTraversePath(answer->GetNodeGUID(), graph.Graph->GetGraphGUID(), 1) };
	IMounteaDialogueParticipantInterface::Execute_SaveTraversedPath(fixture.NpcParticipant, remembered);
	TestFalse(TEXT("Closed once the NPC remembers the visit"), UMounteaDialogueConditionsStatics::EvaluateEdgeConditions(edge, context));

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
