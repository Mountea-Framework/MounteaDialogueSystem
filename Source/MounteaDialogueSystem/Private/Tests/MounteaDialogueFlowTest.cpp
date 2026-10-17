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
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Helpers/MounteaDialogueManagerStatics.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "TimerManager.h"

// Whole dialogues run on the server side of a standalone test world: manager -> world subsystem -> session ->
// nodes -> row timers. Rows last 0.1s and the world is ticked in 0.05s steps; a timer fires about two ticks after
// its duration (see FDialogueWorldFixture::Tick), so a 0.1s line is done after 4 ticks. Replication itself is not
// exercised (standalone).

namespace MounteaDialogueFlowTest
{
	using namespace MounteaDialogueTest;

	static bool HasTraversed(const TArray<FDialogueTraversePath>& Path, const UMounteaDialogueGraphNode* Node, int32& OutCount)
	{
		const FDialogueTraversePath* entry = Path.FindByPredicate([Node](const FDialogueTraversePath& Item)
		{
			return Item.NodeGuid == Node->GetNodeGUID() && Item.GraphGuid == Node->GetGraphGUID();
		});
		OutCount = entry ? entry->TraverseCount : 0;
		return entry != nullptr;
	}
}

// The basic contract: a start request makes the manager Active, publishes the first runtime node in the session
// payload and takes the world-wide dialogue lock.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowStartTest,
	"Mountea.DialogueSystem.World.Flow.StartActivatesManagerAndPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowStartTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	TestTrue(TEXT("Precondition: the NPC can start the graph"), IMounteaDialogueParticipantInterface::Execute_CanStartDialogue(fixture.NpcParticipant));

	fixture.StartDialogue();

	TestEqual(TEXT("Manager is Active"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestTrue(TEXT("The first runtime node is the Start node's child"), fixture.GetActiveNode() == lead);
	TestTrue(TEXT("The payload has a session"), fixture.GetPayload().SessionGUID.IsValid());
	TestEqual(TEXT("The payload points at the active node"), fixture.GetPayload().ActiveNodeGUID, lead->GetNodeGUID());
	TestEqual(TEXT("The payload points at the active graph"), fixture.GetPayload().ActiveGraphGUID, graph.Graph->GetGraphGUID());
	TestEqual(TEXT("The first row line is active"), fixture.GetPayload().ActiveDialogueRowDataIndex, 0);
	TestEqual(TEXT("Manager context carries the payload session"), fixture.GetContext()->SessionGUID, fixture.GetPayload().SessionGUID);
	TestTrue(TEXT("The lock is held"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
	TestTrue(TEXT("The lock is held by this manager"), FMounteaDialogueTestAccess::GetLockOwner(fixture.Subsystem) == fixture.Manager);
	TestTrue(TEXT("The session is listed as active"), fixture.Subsystem->GetActiveSessions().Contains(fixture.Session));
	TestEqual(TEXT("The NPC participant is Active"), IMounteaDialogueParticipantInterface::Execute_GetParticipantState(fixture.NpcParticipant), EDialogueParticipantState::EDPS_Active);
	TestEqual(TEXT("The player participant is Active"), IMounteaDialogueParticipantInterface::Execute_GetParticipantState(fixture.PlayerParticipant), EDialogueParticipantState::EDPS_Active);
	TestTrue(TEXT("The row timer is running"), fixture.GetWorld()->GetTimerManager().IsTimerActive(fixture.Manager->GetDialogueRowTimerHandle()));

	fixture.Teardown(*this);
	return true;
}

// Automatic lines advance on their own; a node without children ends the dialogue, which must give everything
// back: manager state, lock, participant states, and the traversed path must be saved on the participants.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowAutoAdvanceTest,
	"Mountea.DialogueSystem.World.Flow.AutomaticRowsAdvanceAndLastNodeCloses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowAutoAdvanceTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	const FGuid sessionGuid = fixture.GetPayload().SessionGUID;

	fixture.Tick(4);
	TestEqual(TEXT("After the first line's duration the second line is active"), fixture.GetPayload().ActiveDialogueRowDataIndex, 1);
	TestEqual(TEXT("Still Active while the second line plays"), fixture.GetState(), EDialogueManagerState::EDMS_Active);

	fixture.Tick(6);
	TestEqual(TEXT("A node without children closes the dialogue"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
	TestFalse(TEXT("The lock is released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
	TestEqual(TEXT("No session is listed as active"), fixture.Subsystem->GetActiveSessions().Num(), 0);
	TestNull(TEXT("The manager drops its context"), fixture.GetContext());
	TestEqual(TEXT("The NPC participant is back to its default state"), IMounteaDialogueParticipantInterface::Execute_GetParticipantState(fixture.NpcParticipant), EDialogueParticipantState::EDPS_Enabled);
	TestEqual(TEXT("The player participant is back to its default state"), IMounteaDialogueParticipantInterface::Execute_GetParticipantState(fixture.PlayerParticipant), EDialogueParticipantState::EDPS_Enabled);

	int32 count = 0;
	TestTrue(TEXT("The NPC remembers the traversed node"),
		HasTraversed(IMounteaDialogueParticipantInterface::Execute_GetTraversedPath(fixture.NpcParticipant), lead, count));
	TestEqual(TEXT("The node was traversed once"), count, 1);

	// A finished dialogue can be started again, with a fresh session.
	fixture.StartDialogue();
	TestEqual(TEXT("A second dialogue starts"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestNotEqual(TEXT("The second dialogue has its own session"), fixture.GetPayload().SessionGUID, sessionGuid);

	fixture.Teardown(*this);
	return true;
}

// A node whose children all need input publishes them as options, in edge order, and waits.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowOptionsTest,
	"Mountea.DialogueSystem.World.Flow.AnswersArePublishedAsOptionsAndSelectable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowOptionsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answerA = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("AnswerA"));
	UMounteaDialogueGraphNode_AnswerNode* answerB = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("AnswerB"));
	UMounteaDialogueGraphNode_LeadNode* reply = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Reply"));
	graph.Link(graph.Start(), lead);
	graph.Link(lead, answerA);
	graph.Link(lead, answerB);
	graph.Link(answerB, reply);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	fixture.Tick(6);

	TestEqual(TEXT("Still Active while waiting for a choice"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestTrue(TEXT("The Lead stays the active node while options are shown"), fixture.GetActiveNode() == lead);
	const TArray<FGuid>& options = fixture.GetPayload().AllowedChildNodeGUIDs;
	if (TestEqual(TEXT("Both answers are offered"), options.Num(), 2))
	{
		TestEqual(TEXT("Options follow edge order (first)"), options[0], answerA->GetNodeGUID());
		TestEqual(TEXT("Options follow edge order (second)"), options[1], answerB->GetNodeGUID());
	}
	TestFalse(TEXT("No row is shown while options are up"), UMounteaDialogueTraversalStatics::IsDialogueRowValid(fixture.GetPayload().ActiveDialogueRow));

	// Nothing happens on its own while the player decides.
	fixture.Tick(10);
	TestTrue(TEXT("Waiting is indefinite"), fixture.GetActiveNode() == lead);

	IMounteaDialogueManagerInterface::Execute_SelectNode(fixture.Manager, answerB->GetNodeGUID());
	TestTrue(TEXT("The chosen answer becomes active"), fixture.GetActiveNode() == answerB);
	TestEqual(TEXT("The payload follows the choice"), fixture.GetPayload().ActiveNodeGUID, answerB->GetNodeGUID());
	TestEqual(TEXT("The payload remembers where it came from"), fixture.GetPayload().PreviousNodeGUID, lead->GetNodeGUID());

	fixture.Tick(5);
	TestTrue(TEXT("An auto-starting child follows the answer without input"), fixture.GetActiveNode() == reply);

	fixture.Tick(6);
	TestEqual(TEXT("The branch ends and closes the dialogue"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);

	fixture.Teardown(*this);
	return true;
}

// SelectNode is client-callable: a GUID that is not one of the offered options must not move the dialogue
// anywhere. The dialogue fails and closes, and must not leave the lock behind.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowInvalidSelectionTest,
	"Mountea.DialogueSystem.World.Flow.InvalidSelectionIsRejectedAndCleansUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowInvalidSelectionTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;
	FScopedDialogueLogMute mute;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));
	UMounteaDialogueGraphNode_AnswerNode* unrelated = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Unrelated"));
	graph.Link(graph.Start(), lead);
	graph.Link(lead, answer);
	graph.Link(graph.Start(), unrelated);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	fixture.Tick(6);
	TestTrue(TEXT("Precondition: options are shown"), fixture.GetActiveNode() == lead);

	IMounteaDialogueManagerInterface::Execute_SelectNode(fixture.Manager, unrelated->GetNodeGUID());
	TestFalse(TEXT("A node that is not an offered option never becomes active"), fixture.GetActiveNode() == unrelated);
	TestEqual(TEXT("The lock matches the manager state (no dangling lock)"),
		FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem), fixture.GetState() == EDialogueManagerState::EDMS_Active);

	fixture.Teardown(*this);
	return true;
}

// Await Input lines hold until the player skips; Skip on an Automatic line just cuts it short.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowAwaitInputTest,
	"Mountea.DialogueSystem.World.Flow.AwaitInputRowWaitsForSkip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowAwaitInputTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_AwaitInput, ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	fixture.Tick(10);
	TestEqual(TEXT("An Await Input line does not advance on its own"), fixture.GetPayload().ActiveDialogueRowDataIndex, 0);

	IMounteaDialogueManagerInterface::Execute_SkipDialogueRow(fixture.Manager);
	TestEqual(TEXT("Skip advances to the next line"), fixture.GetPayload().ActiveDialogueRowDataIndex, 1);

	IMounteaDialogueManagerInterface::Execute_SkipDialogueRow(fixture.Manager);
	TestEqual(TEXT("Skip on an Automatic line advances immediately"), fixture.GetPayload().ActiveDialogueRowDataIndex, 2);

	IMounteaDialogueManagerInterface::Execute_SkipDialogueRow(fixture.Manager);
	TestEqual(TEXT("Skipping the last line finishes the node, which closes the dialogue"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);

	fixture.Teardown(*this);
	return true;
}

// A Stopping line plays, then ends the whole node; lines after it never play (regression area: EREM_Stopping used
// to hang server sessions, and later was skipped instead of shown).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowStoppingTest,
	"Mountea.DialogueSystem.World.Flow.StoppingRowFinishesTheNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowStoppingTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	// If Stopping were ignored, the second (Await Input) line would hold the dialogue open forever.
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Stopping, ERowExecutionMode::EREM_AwaitInput });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	fixture.Tick(4);
	TestEqual(TEXT("The Stopping line is shown"), fixture.GetPayload().ActiveDialogueRowDataIndex, 1);
	TestEqual(TEXT("and the dialogue is still running while it plays"), fixture.GetState(), EDialogueManagerState::EDMS_Active);

	fixture.Tick(6);
	TestNotEqual(TEXT("The line after the Stopping line never plays"), fixture.GetPayload().ActiveDialogueRowDataIndex, 2);
	TestEqual(TEXT("The Stopping line ends the node, and the leaf node closes the dialogue"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
	TestFalse(TEXT("The lock is released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));

	fixture.Teardown(*this);
	return true;
}

// A Stopping first line also ends the node once it has played.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowStoppingFirstTest,
	"Mountea.DialogueSystem.World.Flow.StoppingFirstRowFinishesTheNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowStoppingFirstTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Stopping, ERowExecutionMode::EREM_AwaitInput });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	TestEqual(TEXT("The Stopping first line is shown"), fixture.GetPayload().ActiveDialogueRowDataIndex, 0);

	fixture.Tick(6);
	TestEqual(TEXT("After it plays, the node ends and the dialogue closes"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
	TestNotEqual(TEXT("The Await Input line after it never plays"), fixture.GetPayload().ActiveDialogueRowDataIndex, 1);

	fixture.Teardown(*this);
	return true;
}

// Every session records the graph it runs, so managers (and clients) can resolve nodes even in graphs no
// participant owns.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowRootGraphTest,
	"Mountea.DialogueSystem.World.Flow.StartPayloadCarriesRootGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowRootGraphTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"), { ERowExecutionMode::EREM_AwaitInput });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	TestTrue(TEXT("The payload carries the started graph"), fixture.GetPayload().RootGraph == graph.Graph);
	TestTrue(TEXT("ResolveGraphByGuid finds a graph through the root alone"),
		UMounteaDialogueManagerStatics::ResolveGraphByGuid({}, graph.Graph->GetGraphGUID(), graph.Graph) == graph.Graph);
	TestNull(TEXT("and still returns null without it"),
		UMounteaDialogueManagerStatics::ResolveGraphByGuid({}, graph.Graph->GetGraphGUID()));

	fixture.Teardown(*this);
	return true;
}

// Closing mid-line must stop the row timer, otherwise it fires into a closed dialogue.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowCloseMidRowTest,
	"Mountea.DialogueSystem.World.Flow.CloseMidRowStopsTimersAndReleasesLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowCloseMidRowTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	TestTrue(TEXT("Precondition: a line is playing"), fixture.GetWorld()->GetTimerManager().IsTimerActive(fixture.Manager->GetDialogueRowTimerHandle()));

	IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(fixture.Manager);

	TestEqual(TEXT("Closed"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
	TestFalse(TEXT("The row timer is cleared"), fixture.GetWorld()->GetTimerManager().IsTimerActive(fixture.Manager->GetDialogueRowTimerHandle()));
	TestFalse(TEXT("The lock is released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
	TestEqual(TEXT("The NPC participant is released"), IMounteaDialogueParticipantInterface::Execute_GetParticipantState(fixture.NpcParticipant), EDialogueParticipantState::EDPS_Enabled);

	fixture.Tick(6);
	TestEqual(TEXT("Nothing restarts after the close"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);

	fixture.Teardown(*this);
	return true;
}

// The server validates every request against the active session; a request for another session, or from a
// manager that does not own the session, must be refused.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowSessionGuardTest,
	"Mountea.DialogueSystem.World.Flow.SessionRejectsStaleOrForeignRequests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowSessionGuardTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;
	FScopedDialogueLogMute mute;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));
	graph.Link(graph.Start(), lead);
	graph.Link(lead, answer);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	APawn* otherPawn = fixture.GetWorld()->SpawnActor<APawn>();
	UMounteaDialogueManager* otherManager = FDialogueWorldFixture::AddComponent<UMounteaDialogueManager>(otherPawn);

	fixture.StartDialogue();
	fixture.Tick(6);
	const FGuid sessionGuid = fixture.GetPayload().SessionGUID;

	// Refusals that are not about this manager's session leave the running dialogue alone.
	TestFalse(TEXT("A null manager is refused"), fixture.Session->HandleSkipDialogueRow(nullptr, sessionGuid));
	TestFalse(TEXT("A manager that does not own the session is refused"), fixture.Session->HandleSelectNode(otherManager, sessionGuid, answer->GetNodeGUID()));
	TestEqual(TEXT("The owning manager's dialogue keeps running"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestTrue(TEXT("Still waiting on the Lead's options"), fixture.GetActiveNode() == lead);

	// Requests from the owning manager with a wrong session GUID (late or duplicated RPCs) are refused, and must
	// not end the session that is running.
	TestFalse(TEXT("A GUID of another session is refused"), fixture.Session->HandleSelectNode(fixture.Manager, FGuid::NewGuid(), answer->GetNodeGUID()));
	TestFalse(TEXT("An invalid GUID is refused"), fixture.Session->HandleSelectNode(fixture.Manager, FGuid(), answer->GetNodeGUID()));
	TestFalse(TEXT("A stale skip is refused"), fixture.Session->HandleSkipDialogueRow(fixture.Manager, FGuid::NewGuid()));
	TestFalse(TEXT("A stale close is refused"), fixture.Session->HandleCloseDialogue(fixture.Manager, FGuid::NewGuid()));
	TestFalse(TEXT("None of the refused requests selected the answer"), fixture.GetActiveNode() == answer);
	TestEqual(TEXT("The running dialogue survives stale requests"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestTrue(TEXT("It is still on the Lead"), fixture.GetActiveNode() == lead);
	TestEqual(TEXT("The session is unchanged"), fixture.GetPayload().SessionGUID, sessionGuid);
	TestTrue(TEXT("The lock is still held by the manager"), FMounteaDialogueTestAccess::GetLockOwner(fixture.Subsystem) == fixture.Manager);

	IMounteaDialogueManagerInterface::Execute_SelectNode(fixture.Manager, answer->GetNodeGUID());
	TestTrue(TEXT("A valid request still works afterwards"), fixture.GetActiveNode() == answer);

	fixture.Teardown(*this);
	return true;
}

// Only one dialogue may run at a time; a second manager is refused and the first keeps the lock.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowSecondStartTest,
	"Mountea.DialogueSystem.World.Flow.SecondManagerCannotStartWhileActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowSecondStartTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;
	FScopedDialogueLogMute mute;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"), { ERowExecutionMode::EREM_AwaitInput });
	graph.Link(graph.Start(), lead);

	FTestGraph otherGraph;
	UMounteaDialogueGraphNode_LeadNode* otherLead = otherGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("OtherLead"));
	otherGraph.Link(otherGraph.Start(), otherLead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	APawn* otherPawn = fixture.GetWorld()->SpawnActor<APawn>();
	UMounteaDialogueManager* otherManager = FDialogueWorldFixture::AddComponent<UMounteaDialogueManager>(otherPawn);
	AActor* otherNpc = fixture.GetWorld()->SpawnActor<AActor>();
	UMounteaDialogueParticipant* otherNpcParticipant = FDialogueWorldFixture::AddComponent<UMounteaDialogueParticipant>(otherNpc);
	IMounteaDialogueParticipantInterface::Execute_SetDialogueGraph(otherNpcParticipant, otherGraph.Graph);

	fixture.StartDialogue();
	const FGuid sessionGuid = fixture.GetPayload().SessionGUID;

	fixture.StartDialogue(otherManager, otherNpc);

	TestEqual(TEXT("The second manager stays idle"), IMounteaDialogueManagerInterface::Execute_GetManagerState(otherManager), EDialogueManagerState::EDMS_Enabled);
	TestTrue(TEXT("The first manager keeps the lock"), FMounteaDialogueTestAccess::GetLockOwner(fixture.Subsystem) == fixture.Manager);
	TestEqual(TEXT("The first dialogue keeps running"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestEqual(TEXT("The session is unchanged"), fixture.GetPayload().SessionGUID, sessionGuid);
	TestTrue(TEXT("The first dialogue is still on its node"), fixture.GetActiveNode() == lead);

	fixture.Teardown(*this);
	return true;
}

// Every refused start must give the lock back, otherwise no dialogue can ever start again in that world.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowRefusedStartTest,
	"Mountea.DialogueSystem.World.Flow.RefusedStartReleasesLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowRefusedStartTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;
	FScopedDialogueLogMute mute;

	// NPC without a graph.
	{
		FDialogueWorldFixture fixture;
		if (fixture.Setup(*this, nullptr))
		{
			fixture.StartDialogue();
			TestEqual(TEXT("[No graph] Manager stays idle"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
			TestFalse(TEXT("[No graph] Lock released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
		}
		fixture.Teardown(*this);
	}

	// Graph whose Start node has no child.
	{
		FTestGraph emptyGraph;
		FDialogueWorldFixture fixture;
		if (fixture.Setup(*this, emptyGraph.Graph))
		{
			fixture.StartDialogue();
			TestEqual(TEXT("[Empty graph] Manager stays idle"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
			TestFalse(TEXT("[Empty graph] Lock released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
		}
		fixture.Teardown(*this);
	}

	// GameState without a session component.
	{
		FTestGraph graph;
		UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
		graph.Link(graph.Start(), lead);

		FDialogueWorldFixture fixture;
		if (fixture.Setup(*this, graph.Graph, false))
		{
			fixture.StartDialogue();
			TestEqual(TEXT("[No session] Manager stays idle"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
			TestFalse(TEXT("[No session] Lock released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
			TestEqual(TEXT("[No session] Nothing listed as active"), fixture.Subsystem->GetActiveSessions().Num(), 0);
		}
		fixture.Teardown(*this);
	}

	// Main participant actor without a participant component.
	{
		FTestGraph graph;
		UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
		graph.Link(graph.Start(), lead);

		FDialogueWorldFixture fixture;
		if (fixture.Setup(*this, graph.Graph))
		{
			AActor* stranger = fixture.GetWorld()->SpawnActor<AActor>();
			fixture.StartDialogue(nullptr, stranger);
			TestEqual(TEXT("[No participant] Manager stays idle"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);
			TestFalse(TEXT("[No participant] Lock released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
		}
		fixture.Teardown(*this);
	}

	return true;
}

// A Delay node holds traversal for its duration, then moves to its child without input.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowDelayTest,
	"Mountea.DialogueSystem.World.Flow.DelayNodeWaitsThenContinues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowDelayTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_Delay* delay = graph.AddNode<UMounteaDialogueGraphNode_Delay>();
	delay->SetDelayDuration(1);
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"), { ERowExecutionMode::EREM_AwaitInput });
	graph.Link(graph.Start(), delay);
	graph.Link(delay, lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	TestTrue(TEXT("The Delay node is active first"), fixture.GetActiveNode() == delay);

	fixture.Tick(10);
	TestTrue(TEXT("Halfway through the delay nothing moved"), fixture.GetActiveNode() == delay);

	fixture.Tick(16);
	TestTrue(TEXT("After the delay its child is active"), fixture.GetActiveNode() == lead);
	TestEqual(TEXT("Still Active"), fixture.GetState(), EDialogueManagerState::EDMS_Active);

	fixture.Teardown(*this);
	return true;
}

// Return To Node jumps back to an ancestor (Bugreport #5, end-to-end): the target runs again and its options
// come back.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowReturnToNodeTest,
	"Mountea.DialogueSystem.World.Flow.ReturnToNodeJumpsBackToAncestor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowReturnToNodeTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = graph.AddNode<UMounteaDialogueGraphNode_ReturnToNode>();
	returnNode->SelectedNode = lead;
	graph.Link(graph.Start(), lead);
	graph.Link(lead, answer);
	graph.Link(answer, returnNode);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	fixture.Tick(6);
	IMounteaDialogueManagerInterface::Execute_SelectNode(fixture.Manager, answer->GetNodeGUID());
	TestTrue(TEXT("Precondition: the answer is active"), fixture.GetActiveNode() == answer);

	// Answer line (0.1s) -> Return node (0.1s delay) -> Lead again.
	fixture.Tick(8);
	TestTrue(TEXT("The Return node brought the Lead back"), fixture.GetActiveNode() == lead);
	TestEqual(TEXT("The dialogue is still running"), fixture.GetState(), EDialogueManagerState::EDMS_Active);

	int32 count = 0;
	TestTrue(TEXT("The Lead is in the traversed path"), HasTraversed(fixture.GetContext()->TraversedPath, lead, count));
	TestEqual(TEXT("The Lead was traversed twice"), count, 2);

	fixture.Tick(6);
	const TArray<FGuid>& options = fixture.GetPayload().AllowedChildNodeGUIDs;
	TestTrue(TEXT("The Lead's options are offered again"), options.Num() == 1 && options[0] == answer->GetNodeGUID());

	fixture.Teardown(*this);
	return true;
}

// Open Child Graph transfers traversal into another graph that no participant owns; the server resolves its
// nodes through the Open node's target.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowOpenChildGraphTest,
	"Mountea.DialogueSystem.World.Flow.OpenChildGraphContinuesInChild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowOpenChildGraphTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;

	FTestGraph childGraph;
	UMounteaDialogueGraphNode_LeadNode* childLead = childGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("ChildLead"));
	childGraph.Link(childGraph.Start(), childLead);

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_OpenChildGraph* openNode = graph.AddNode<UMounteaDialogueGraphNode_OpenChildGraph>();
	openNode->TargetDialogue = childGraph.Graph;
	graph.Link(graph.Start(), lead);
	graph.Link(lead, openNode);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	fixture.Tick(4);

	TestTrue(TEXT("The child graph's first node is active"), fixture.GetActiveNode() == childLead);
	TestEqual(TEXT("The payload switched graphs"), fixture.GetPayload().ActiveGraphGUID, childGraph.Graph->GetGraphGUID());
	TestEqual(TEXT("The payload points at the child node"), fixture.GetPayload().ActiveNodeGUID, childLead->GetNodeGUID());

	fixture.Tick(6);
	TestEqual(TEXT("The child graph's leaf closes the dialogue"), fixture.GetState(), EDialogueManagerState::EDMS_Enabled);

	fixture.Teardown(*this);
	return true;
}

// The player leaving mid-dialogue (manager owner destroyed) must free the lock for everyone else.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFlowManagerDestroyedTest,
	"Mountea.DialogueSystem.World.Flow.DestroyedManagerReleasesLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFlowManagerDestroyedTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueFlowTest;
	FScopedDialogueLogMute mute;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic });
	graph.Link(graph.Start(), lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	TestTrue(TEXT("Precondition: the lock is held"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));

	fixture.Player->Destroy();

	TestFalse(TEXT("The lock is released"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
	TestFalse(TEXT("The manager is no longer registered"), fixture.Subsystem->GetRegisteredManagers().Contains(fixture.Manager));

	// A row timer that was still pending must not fire into the destroyed manager.
	fixture.Tick(6);

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

