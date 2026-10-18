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
#include "Decorators/MounteaIDialogueDecorator_SwapParticipants.h"
#include "GameplayTagContainer.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"

// Graph decorators belong to the graph that is running: a start-request override or an Open Child Graph target
// initializes and runs its own graph decorators, and the NPC's own graph decorators stay out of it.
// The probe is a Swap Participants graph decorator: when it runs, the player becomes the active speaker.

namespace MounteaDialogueGraphDecoratorFlowTest
{
	using namespace MounteaDialogueTest;

	static FGameplayTag NpcTag() { return FGameplayTag::RequestGameplayTag(TEXT("Mountea_Dialogue.Graph.Type.Dialogue")); }
	static FGameplayTag PlayerTag() { return FGameplayTag::RequestGameplayTag(TEXT("Mountea_Dialogue.Graph.Type.Monologue")); }

	// Adds a graph decorator that makes the player the active speaker.
	static UMounteaDialogueDecorator_SwapParticipants* AddSwapToPlayer(FAutomationTestBase& Test, UMounteaDialogueGraph* Graph)
	{
		TArray<FMounteaDialogueDecorator>* decorators = PropertyRef<TArray<FMounteaDialogueDecorator>>(Graph, TEXT("GraphDecorators"));
		if (!decorators)
		{
			Test.AddError(TEXT("GraphDecorators property not found"));
			return nullptr;
		}
		UMounteaDialogueDecorator_SwapParticipants* swap = AddDecorator<UMounteaDialogueDecorator_SwapParticipants>(Graph, *decorators);
		SetProperty(Test, swap, TEXT("NewParticipantTag"), PlayerTag());
		return swap;
	}

	static void TagParticipants(FDialogueWorldFixture& Fixture)
	{
		Fixture.NpcParticipant->ParticipantTag = NpcTag();
		Fixture.PlayerParticipant->ParticipantTag = PlayerTag();
	}

	static UObject* ActiveSpeaker(const FDialogueWorldFixture& Fixture)
	{
		return Fixture.GetPayload().ActiveDialogueParticipant.GetObject();
	}

	static void StartWithOverride(FDialogueWorldFixture& Fixture, UMounteaDialogueGraph* Override)
	{
		FDialogueStartRequest request;
		request.MainParticipantActorRef = Fixture.Npc;
		request.DialogueGraph = Override;
		Fixture.Subsystem->HandleStartRequest(Fixture.Manager, request);
	}
}

// With an override, the override graph's graph decorators are initialized and run.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueOverrideGraphDecoratorsRunTest,
	"Mountea.DialogueSystem.World.GraphDecorators.OverrideGraphDecoratorsRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueOverrideGraphDecoratorsRunTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueGraphDecoratorFlowTest;

	FTestGraph npcGraph;
	npcGraph.Link(npcGraph.Start(), npcGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("NpcLead"), { ERowExecutionMode::EREM_AwaitInput }));
	FTestGraph overrideGraph;
	overrideGraph.Link(overrideGraph.Start(), overrideGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("OverrideLead"), { ERowExecutionMode::EREM_AwaitInput }));
	UMounteaDialogueDecorator_SwapParticipants* swap = AddSwapToPlayer(*this, overrideGraph.Graph);

	FDialogueWorldFixture fixture;
	if (!swap || !fixture.Setup(*this, npcGraph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}
	TagParticipants(fixture);

	StartWithOverride(fixture, overrideGraph.Graph);
	TestEqual(TEXT("The dialogue started"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestTrue(TEXT("The override graph's decorator was initialized"), swap->EvaluateDecorator());
	TestTrue(TEXT("The override graph's decorator ran (player is now speaking)"), ActiveSpeaker(fixture) == fixture.PlayerParticipant);

	IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(fixture.Manager);
	TestFalse(TEXT("Closing cleans the override graph's decorator up"), swap->EvaluateDecorator());

	fixture.Teardown(*this);
	return true;
}

// With an override, the NPC's own graph decorators do not run.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueOwnerGraphDecoratorsSkippedTest,
	"Mountea.DialogueSystem.World.GraphDecorators.OwnerGraphDecoratorsDoNotRunForOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueOwnerGraphDecoratorsSkippedTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueGraphDecoratorFlowTest;

	FTestGraph npcGraph;
	npcGraph.Link(npcGraph.Start(), npcGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("NpcLead"), { ERowExecutionMode::EREM_AwaitInput }));
	UMounteaDialogueDecorator_SwapParticipants* npcSwap = AddSwapToPlayer(*this, npcGraph.Graph);
	FTestGraph overrideGraph;
	overrideGraph.Link(overrideGraph.Start(), overrideGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("OverrideLead"), { ERowExecutionMode::EREM_AwaitInput }));

	FDialogueWorldFixture fixture;
	if (!npcSwap || !fixture.Setup(*this, npcGraph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}
	TagParticipants(fixture);

	StartWithOverride(fixture, overrideGraph.Graph);
	TestEqual(TEXT("The dialogue started"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestTrue(TEXT("The NPC graph's decorator did not run (NPC is still speaking)"), ActiveSpeaker(fixture) == fixture.NpcParticipant);

	fixture.Teardown(*this);
	return true;
}

// Without an override nothing changes: the NPC graph's decorators run as before.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueOwnGraphDecoratorsRunTest,
	"Mountea.DialogueSystem.World.GraphDecorators.OwnGraphDecoratorsRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueOwnGraphDecoratorsRunTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueGraphDecoratorFlowTest;

	FTestGraph npcGraph;
	npcGraph.Link(npcGraph.Start(), npcGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("NpcLead"), { ERowExecutionMode::EREM_AwaitInput }));
	UMounteaDialogueDecorator_SwapParticipants* swap = AddSwapToPlayer(*this, npcGraph.Graph);

	FDialogueWorldFixture fixture;
	if (!swap || !fixture.Setup(*this, npcGraph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}
	TagParticipants(fixture);

	fixture.StartDialogue();
	TestTrue(TEXT("The NPC graph's decorator ran"), ActiveSpeaker(fixture) == fixture.PlayerParticipant);

	// A second dialogue re-initializes the decorator after the first one cleaned it up.
	IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(fixture.Manager);
	fixture.StartDialogue();
	TestTrue(TEXT("It is usable again in the next dialogue"), swap->EvaluateDecorator());
	TestTrue(TEXT("and runs again"), ActiveSpeaker(fixture) == fixture.PlayerParticipant);

	fixture.Teardown(*this);
	return true;
}

// Nodes in a child graph inherit the child graph's decorators, not the root graph's.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueChildGraphDecoratorsTest,
	"Mountea.DialogueSystem.World.GraphDecorators.ChildGraphNodesInheritChildGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueChildGraphDecoratorsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueGraphDecoratorFlowTest;

	FTestGraph childGraph;
	UMounteaDialogueGraphNode_LeadNode* childLead = childGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("ChildLead"), { ERowExecutionMode::EREM_AwaitInput });
	childGraph.Link(childGraph.Start(), childLead);
	UMounteaDialogueDecorator_SwapParticipants* childSwap = AddSwapToPlayer(*this, childGraph.Graph);

	FTestGraph rootGraph;
	UMounteaDialogueGraphNode_LeadNode* rootLead = rootGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("RootLead"));
	UMounteaDialogueGraphNode_OpenChildGraph* open = rootGraph.AddNode<UMounteaDialogueGraphNode_OpenChildGraph>();
	open->TargetDialogue = childGraph.Graph;
	rootGraph.Link(rootGraph.Start(), rootLead);
	rootGraph.Link(rootLead, open);

	FDialogueWorldFixture fixture;
	if (!childSwap || !fixture.Setup(*this, rootGraph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}
	TagParticipants(fixture);

	fixture.StartDialogue();
	TestTrue(TEXT("In the root graph the child's decorator does not run"), ActiveSpeaker(fixture) == fixture.NpcParticipant);

	fixture.Tick(4);
	TestTrue(TEXT("Precondition: the child graph is running"), fixture.GetActiveNode() == childLead);
	TestTrue(TEXT("The child graph's decorator ran for its node"), ActiveSpeaker(fixture) == fixture.PlayerParticipant);

	IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(fixture.Manager);
	TestFalse(TEXT("Closing cleans the child graph's decorator up"), childSwap->EvaluateDecorator());

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
