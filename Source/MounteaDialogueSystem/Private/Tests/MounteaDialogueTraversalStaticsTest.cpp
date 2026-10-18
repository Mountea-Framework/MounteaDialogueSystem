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
#include "GameplayTagContainer.h"
#include "Helpers/MounteaDialogueContextStatics.h"
#include "Helpers/MounteaDialogueHUDStatics.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

// The pure traversal helpers the session uses to pick the next node and speaker.

// Children are offered in ExecutionOrder (edge order), and children whose edge conditions fail are dropped.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueTraversalChildrenTest,
	"Mountea.DialogueSystem.Traversal.AllowedChildrenAndOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueTraversalChildrenTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;
	using UStatics = UMounteaDialogueTraversalStatics;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* first = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("First"));
	UMounteaDialogueGraphNode_AnswerNode* blocked = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Blocked"));
	UMounteaDialogueGraphNode_AnswerNode* last = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Last"));
	graph.Link(graph.Start(), lead);
	graph.Link(lead, first);
	UMounteaDialogueGraphEdge* blockedEdge = graph.Link(lead, blocked);
	graph.Link(lead, last);

	FMounteaDialogueCondition never;
	never.ConditionClass = NewObject<UMounteaDialogueConditionBase>(blockedEdge);
	never.bNegate = true;
	blockedEdge->EdgeConditions.Rules.Add(never);

	UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(GetTransientPackage());
	TArray<UMounteaDialogueGraphNode*> allowed = UStatics::GetAllowedChildNodesFiltered(lead, context);
	TestTrue(TEXT("The blocked child is dropped, the rest keep edge order"), allowed == TArray<UMounteaDialogueGraphNode*>({ first, last }));
	TestEqual(TEXT("A null parent has no children"), UStatics::GetAllowedChildNodesFiltered(nullptr, context).Num(), 0);

	TArray<UMounteaDialogueGraphNode*> shuffled = { last, nullptr, first, blocked };
	UStatics::SortNodes(shuffled);
	TestTrue(TEXT("SortNodes orders by ExecutionOrder and drops nulls"), shuffled == TArray<UMounteaDialogueGraphNode*>({ first, blocked, last }));

	TestTrue(TEXT("The first child of Start is the Lead"), UStatics::GetFirstChildNode(graph.Start()) == lead);
	TestNull(TEXT("A leaf has no first child"), UStatics::GetFirstChildNode(last));
	TestNull(TEXT("A null node has no first child"), UStatics::GetFirstChildNode(nullptr));

	TArray<UMounteaDialogueGraphNode*> mixed = { last, graph.AddNode<UMounteaDialogueGraphNode_Delay>(), first };
	TestTrue(TEXT("HUD filtering keeps only speech nodes, in order"),
		UMounteaDialogueHUDStatics::FilterDialogueFriendlyNodes(mixed) == TArray<UMounteaDialogueGraphNode*>({ first, last }));

	TestTrue(TEXT("Context lookups find nodes by GUID"),
		UMounteaDialogueContextStatics::FindNodesByGUID(graph.Graph, { last->GetNodeGUID(), FGuid::NewGuid(), first->GetNodeGUID() })
		== TArray<UMounteaDialogueGraphNode*>({ last, first }));
	TestNull(TEXT("Unknown GUIDs resolve to null"), UMounteaDialogueContextStatics::FindNodeByGUID(graph.Graph, FGuid::NewGuid()));
	return true;
}

// The speaker for a node: matched by tag when the row or node carries tags, else the current speaker, else the
// first participant.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueTraversalActiveParticipantTest,
	"Mountea.DialogueSystem.Traversal.ResolveActiveParticipant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueTraversalActiveParticipantTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;
	using UStatics = UMounteaDialogueTraversalStatics;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	const FGameplayTag npcTag = FGameplayTag::RequestGameplayTag(TEXT("Mountea_Dialogue.Graph.Type.Dialogue"));
	const FGameplayTag playerTag = FGameplayTag::RequestGameplayTag(TEXT("Mountea_Dialogue.Graph.Type.Monologue"));
	fixture.NpcParticipant->ParticipantTag = npcTag;
	fixture.PlayerParticipant->ParticipantTag = playerTag;

	TScriptInterface<IMounteaDialogueParticipantInterface> npc;
	npc.SetObject(fixture.NpcParticipant);
	npc.SetInterface(fixture.NpcParticipant);
	TScriptInterface<IMounteaDialogueParticipantInterface> player;
	player.SetObject(fixture.PlayerParticipant);
	player.SetInterface(fixture.PlayerParticipant);

	UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(GetTransientPackage());
	TestNull(TEXT("No active node: nobody"), UStatics::ResolveActiveParticipant(context).GetObject());

	context->DialogueParticipants = { npc, player };
	context->SetDialogueContext(lead, {});
	TestTrue(TEXT("No tags, no speaker yet: the first participant"), UStatics::ResolveActiveParticipant(context).GetObject() == fixture.NpcParticipant);

	context->SetActiveDialogueParticipant(player);
	TestTrue(TEXT("No tags: the current speaker stays"), UStatics::ResolveActiveParticipant(context).GetObject() == fixture.PlayerParticipant);

	lead->NodeGameplayTags.AddTag(npcTag);
	TestTrue(TEXT("Node tags pick the matching participant"), UStatics::ResolveActiveParticipant(context).GetObject() == fixture.NpcParticipant);

	lead->NodeGameplayTags.Reset();
	FDialogueRow row = UStatics::GetSpeechData(lead);
	row.CompatibleTags.AddTag(npcTag);
	context->UpdateActiveDialogueRow(row);
	TestTrue(TEXT("Row tags pick the matching participant"), UStatics::ResolveActiveParticipant(context).GetObject() == fixture.NpcParticipant);

	TestTrue(TEXT("Lookup by tag"), UStatics::FindParticipantByTag(context, playerTag).GetObject() == fixture.PlayerParticipant);
	TestNull(TEXT("Lookup by an empty tag finds nobody"), UStatics::FindParticipantByTag(context, FGameplayTag()).GetObject());

	TestTrue(TEXT("UpdateMatchingDialogueParticipant reports a change"), UStatics::UpdateMatchingDialogueParticipant(context, npc));
	TestFalse(TEXT("and no change when it is already active"), UStatics::UpdateMatchingDialogueParticipant(context, npc));

	fixture.Teardown(*this);
	return true;
}

// Text helpers used by the UI.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueTraversalTextHelpersTest,
	"Mountea.DialogueSystem.HUDStatics.ReplaceRegexInText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueTraversalTextHelpersTest::RunTest(const FString& Parameters)
{
	const auto Replace = [](const TCHAR* Regex, const TCHAR* With, const TCHAR* Source)
	{
		return UMounteaDialogueHUDStatics::ReplaceRegexInText(Regex, FText::FromString(With), FText::FromString(Source)).ToString();
	};

	TestEqual(TEXT("Every match is replaced"), Replace(TEXT("\\{name\\}"), TEXT("Ada"), TEXT("Hi {name}, bye {name}.")), FString(TEXT("Hi Ada, bye Ada.")));
	TestEqual(TEXT("No match leaves the text"), Replace(TEXT("\\{name\\}"), TEXT("Ada"), TEXT("Hello.")), FString(TEXT("Hello.")));
	TestEqual(TEXT("Empty source stays empty"), Replace(TEXT("x"), TEXT("y"), TEXT("")), FString());
	TestEqual(TEXT("A null container has no option index"), UMounteaDialogueHUDStatics::GetSafeOptionIndex(nullptr, EUINavigation::Down), INDEX_NONE);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
