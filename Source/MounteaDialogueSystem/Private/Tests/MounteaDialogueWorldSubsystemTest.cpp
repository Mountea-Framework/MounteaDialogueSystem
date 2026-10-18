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
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

// The world subsystem owns manager registration, participant resolution and the single "one dialogue at a time"
// lock. These tests drive it directly; MounteaDialogueFlowTest covers the same pieces through full dialogues.

namespace MounteaDialogueWorldSubsystemTest
{
	using namespace MounteaDialogueTest;

	static FTestGraph MakeStartableGraph()
	{
		FTestGraph graph;
		UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"), { ERowExecutionMode::EREM_AwaitInput });
		graph.Link(graph.Start(), lead);
		return graph;
	}

	static bool Contains(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& Participants, UObject* Participant)
	{
		return Participants.ContainsByPredicate([Participant](const TScriptInterface<IMounteaDialogueParticipantInterface>& Item)
		{
			return Item.GetObject() == Participant;
		});
	}
}

// Managers register themselves on BeginPlay and leave on EndPlay; the subsystem is how the session finds them.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemRegistrationTest,
	"Mountea.DialogueSystem.World.Subsystem.ManagersRegisterOnBeginPlayAndLeaveOnEndPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemRegistrationTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWorldSubsystemTest;

	FTestGraph graph = MakeStartableGraph();
	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	TestTrue(TEXT("BeginPlay registered the manager"), fixture.Subsystem->GetRegisteredManagers().Contains(fixture.Manager));
	TestTrue(TEXT("The subsystem finds the GameState session"), fixture.Subsystem->GetGameStateSession() == fixture.Session);

	const int32 before = fixture.Subsystem->GetRegisteredManagers().Num();
	fixture.Subsystem->RegisterManager(fixture.Manager);
	fixture.Subsystem->RegisterManager(nullptr);
	TestEqual(TEXT("Registering twice or registering null changes nothing"), fixture.Subsystem->GetRegisteredManagers().Num(), before);

	fixture.Player->Destroy();
	TestFalse(TEXT("EndPlay unregistered the manager"), fixture.Subsystem->GetRegisteredManagers().Contains(fixture.Manager));

	fixture.Subsystem->UnregisterManager(nullptr);

	fixture.Teardown(*this);
	return true;
}

// The lock admits one manager; others are refused until it is released, and only its owner (or the matching
// session) can release it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemLockTest,
	"Mountea.DialogueSystem.World.Subsystem.LockAdmitsOneManager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemLockTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWorldSubsystemTest;

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	APawn* otherPawn = fixture.GetWorld()->SpawnActor<APawn>();
	UMounteaDialogueManager* otherManager = FDialogueWorldFixture::AddComponent<UMounteaDialogueManager>(otherPawn);
	UMounteaDialogueWorldSubsystem* subsystem = fixture.Subsystem;

	TestFalse(TEXT("A null manager never gets the lock"), FMounteaDialogueTestAccess::TryAcquireDialogueLock(subsystem, nullptr));
	TestTrue(TEXT("The first manager gets the lock"), FMounteaDialogueTestAccess::TryAcquireDialogueLock(subsystem, fixture.Manager));
	TestFalse(TEXT("A second manager is refused"), FMounteaDialogueTestAccess::TryAcquireDialogueLock(subsystem, otherManager));
	TestFalse(TEXT("The lock is not re-entrant"), FMounteaDialogueTestAccess::TryAcquireDialogueLock(subsystem, fixture.Manager));

	subsystem->ReleaseDialogueLock(otherManager);
	TestTrue(TEXT("Another manager cannot release the lock"), FMounteaDialogueTestAccess::GetLockOwner(subsystem) == fixture.Manager);

	subsystem->ReleaseDialogueLock(fixture.Manager);
	TestFalse(TEXT("The owner releases the lock"), FMounteaDialogueTestAccess::IsLockActive(subsystem));
	TestTrue(TEXT("After release the other manager gets it"), FMounteaDialogueTestAccess::TryAcquireDialogueLock(subsystem, otherManager));

	subsystem->ReleaseDialogueLock(otherManager);
	subsystem->ReleaseDialogueLock(otherManager);
	TestFalse(TEXT("Releasing an already free lock is harmless"), FMounteaDialogueTestAccess::IsLockActive(subsystem));

	fixture.Teardown(*this);
	return true;
}

// A lock held by a manager that no longer exists must not block the world forever.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemStaleLockTest,
	"Mountea.DialogueSystem.World.Subsystem.LockOfDeadManagerIsReclaimed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemStaleLockTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWorldSubsystemTest;

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	// Not registered with the subsystem, so nothing releases its lock on the way out.
	AActor* ghostOwner = fixture.GetWorld()->SpawnActor<AActor>();
	UMounteaDialogueManager* ghost = NewObject<UMounteaDialogueManager>(ghostOwner);

	TestTrue(TEXT("Precondition: the ghost takes the lock"), FMounteaDialogueTestAccess::TryAcquireDialogueLock(fixture.Subsystem, ghost));
	ghost->MarkAsGarbage();

	TestTrue(TEXT("A live manager reclaims the lock of a dead one"), FMounteaDialogueTestAccess::TryAcquireDialogueLock(fixture.Subsystem, fixture.Manager));
	TestTrue(TEXT("The live manager now owns it"), FMounteaDialogueTestAccess::GetLockOwner(fixture.Subsystem) == fixture.Manager);

	fixture.Teardown(*this);
	return true;
}

// During a session the lock is tied to its session GUID: a release for another session is ignored.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemSessionLockTest,
	"Mountea.DialogueSystem.World.Subsystem.ReleaseForAnotherSessionIsIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemSessionLockTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWorldSubsystemTest;

	FTestGraph graph = MakeStartableGraph();
	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	const FGuid sessionGuid = fixture.GetPayload().SessionGUID;

	fixture.Subsystem->ReleaseDialogueLock(fixture.Manager, FGuid::NewGuid());
	TestTrue(TEXT("A release for another session keeps the lock"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
	TestTrue(TEXT("The session stays listed"), fixture.Subsystem->GetActiveSessions().Contains(fixture.Session));

	fixture.Subsystem->ReleaseDialogueLock(fixture.Manager, sessionGuid);
	TestFalse(TEXT("A release for the active session frees it"), FMounteaDialogueTestAccess::IsLockActive(fixture.Subsystem));
	TestFalse(TEXT("The session is no longer listed"), fixture.Subsystem->GetActiveSessions().Contains(fixture.Session));

	fixture.Teardown(*this);
	return true;
}

// Participant resolution: the main participant must exist and be able to start; the player comes from the
// manager's owner; extra participants are added only when they can take part.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemResolveParticipantsTest,
	"Mountea.DialogueSystem.World.Subsystem.ResolveParticipants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemResolveParticipantsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWorldSubsystemTest;

	FTestGraph graph = MakeStartableGraph();
	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	UWorld* world = fixture.GetWorld();
	AActor* extra = world->SpawnActor<AActor>();
	UMounteaDialogueParticipant* extraParticipant = FDialogueWorldFixture::AddComponent<UMounteaDialogueParticipant>(extra);
	AActor* disabled = world->SpawnActor<AActor>();
	UMounteaDialogueParticipant* disabledParticipant = FDialogueWorldFixture::AddComponent<UMounteaDialogueParticipant>(disabled);
	IMounteaDialogueParticipantInterface::Execute_SetParticipantState(disabledParticipant, EDialogueParticipantState::EDPS_Disabled);
	AActor* plain = world->SpawnActor<AActor>();

	{
		FDialogueStartRequest request;
		request.MainParticipantActorRef = fixture.Npc;
		request.OtherParticipantActorRefs = { extra, disabled, plain, nullptr };

		TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> participants;
		TArray<FText> errors;
		TestTrue(TEXT("A valid request resolves"), FMounteaDialogueTestAccess::ResolveParticipants(fixture.Subsystem, fixture.Manager, request, participants, errors));
		TestTrue(TEXT("The main participant is included"), Contains(participants, fixture.NpcParticipant));
		TestTrue(TEXT("The player participant comes from the manager's pawn"), Contains(participants, fixture.PlayerParticipant));
		TestTrue(TEXT("An enabled extra participant is included"), Contains(participants, extraParticipant));
		TestFalse(TEXT("A disabled extra participant is left out"), Contains(participants, disabledParticipant));
		TestEqual(TEXT("Actors without a participant (and null) are skipped"), participants.Num(), 3);
		TestEqual(TEXT("No errors"), errors.Num(), 0);
	}

	{
		FDialogueStartRequest request;
		request.MainParticipantActor = TSoftObjectPtr<AActor>(fixture.Npc);

		TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> participants;
		TArray<FText> errors;
		TestTrue(TEXT("A soft reference to the main participant resolves too"),
			FMounteaDialogueTestAccess::ResolveParticipants(fixture.Subsystem, fixture.Manager, request, participants, errors));
	}

	const auto ExpectFailure = [this, &fixture](const TCHAR* What, const FDialogueStartRequest& Request, UMounteaDialogueManager* Manager)
	{
		TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> participants;
		TArray<FText> errors;
		TestFalse(What, FMounteaDialogueTestAccess::ResolveParticipants(fixture.Subsystem, Manager, Request, participants, errors));
		TestEqual(FString::Printf(TEXT("%s (one error explains why)"), What), errors.Num(), 1);
	};

	FDialogueStartRequest noMain;
	ExpectFailure(TEXT("Without a main participant it fails"), noMain, fixture.Manager);

	FDialogueStartRequest plainMain;
	plainMain.MainParticipantActorRef = plain;
	ExpectFailure(TEXT("A main actor without a participant component fails"), plainMain, fixture.Manager);

	FDialogueStartRequest graphless;
	graphless.MainParticipantActorRef = extra;
	ExpectFailure(TEXT("A main participant that cannot start (no graph) fails"), graphless, fixture.Manager);

	FDialogueStartRequest valid;
	valid.MainParticipantActorRef = fixture.Npc;
	ExpectFailure(TEXT("A null manager fails"), valid, nullptr);

	fixture.Teardown(*this);
	return true;
}

// A start request may override the graph; the dialogue must run that graph, not the NPC's own one.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemGraphOverrideTest,
	"Mountea.DialogueSystem.World.Subsystem.StartRequestGraphOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemGraphOverrideTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWorldSubsystemTest;

	FTestGraph npcGraph = MakeStartableGraph();
	FTestGraph overrideGraph;
	UMounteaDialogueGraphNode_LeadNode* overrideLead = overrideGraph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("OverrideLead"), { ERowExecutionMode::EREM_AwaitInput });
	overrideGraph.Link(overrideGraph.Start(), overrideLead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, npcGraph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	FDialogueStartRequest request;
	request.MainParticipantActorRef = fixture.Npc;
	request.DialogueGraph = overrideGraph.Graph;
	fixture.Subsystem->HandleStartRequest(fixture.Manager, request);

	TestEqual(TEXT("The dialogue started"), fixture.GetState(), EDialogueManagerState::EDMS_Active);
	TestEqual(TEXT("The payload runs the override graph"), fixture.GetPayload().ActiveGraphGUID, overrideGraph.Graph->GetGraphGUID());
	TestTrue(TEXT("The payload carries the override graph for resolution"), fixture.GetPayload().RootGraph == overrideGraph.Graph);
	TestEqual(TEXT("The payload points at the override graph's first node"), fixture.GetPayload().ActiveNodeGUID, overrideLead->GetNodeGUID());
	TestTrue(TEXT("The manager's context resolved the override graph's node"), fixture.GetActiveNode() == overrideLead);

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
