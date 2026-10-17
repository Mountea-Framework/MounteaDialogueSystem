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
#include "Components/AudioComponent.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

// The participant component: audio lookup, graph ownership, state, traversed-path memory and session role
// overrides.

namespace MounteaDialogueParticipantTest
{
	using namespace MounteaDialogueTest;

	static UAudioComponent* AddAudio(AActor* Owner, const TCHAR* Name, const FName Tag = NAME_None)
	{
		UAudioComponent* audio = NewObject<UAudioComponent>(Owner, Name);
		if (!Tag.IsNone())
			audio->ComponentTags.Add(Tag);
		Owner->AddInstanceComponent(audio);
		audio->RegisterComponent();
		return audio;
	}

	static TScriptInterface<IMounteaDialogueParticipantInterface> ToScript(UMounteaDialogueParticipant* Participant)
	{
		TScriptInterface<IMounteaDialogueParticipantInterface> script;
		script.SetObject(Participant);
		script.SetInterface(Participant);
		return script;
	}
}

// Voice lines need an audio component; designers pick it by name or by tag.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueParticipantAudioLookupTest,
	"Mountea.DialogueSystem.World.Participant.AudioComponentLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueParticipantAudioLookupTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueParticipantTest;

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	AActor* speaker = fixture.GetWorld()->SpawnActor<AActor>();
	UAudioComponent* first = AddAudio(speaker, TEXT("FirstVoice"));
	UAudioComponent* tagged = AddAudio(speaker, TEXT("SecondVoice"), TEXT("Voice"));
	UMounteaDialogueParticipant* participant = FDialogueWorldFixture::AddComponent<UMounteaDialogueParticipant>(speaker);

	TestTrue(TEXT("Lookup by component name"), participant->FindAudioComponentByName(TEXT("SecondVoice")) == tagged);
	TestTrue(TEXT("Lookup by component tag"), participant->FindAudioComponentByTag(TEXT("Voice")) == tagged);
	TestNull(TEXT("Unknown name finds nothing"), participant->FindAudioComponentByName(TEXT("Nope")));
	TestNull(TEXT("Unknown tag finds nothing"), participant->FindAudioComponentByTag(TEXT("Nope")));
	TestTrue(TEXT("BeginPlay falls back to the first audio component"), IMounteaDialogueParticipantInterface::Execute_GetAudioComponent(participant) == first);

	fixture.Teardown(*this);
	return true;
}

// A participant can start only when it is enabled and its graph is startable; a broken graph must not start.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueParticipantCanStartTest,
	"Mountea.DialogueSystem.World.Participant.CanStartDependsOnStateAndGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueParticipantCanStartTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueParticipantTest;

	FTestGraph good;
	good.Link(good.Start(), good.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead")));

	FTestGraph broken;
	broken.Link(broken.Start(), broken.AddNode<UMounteaDialogueGraphNode_LeadNode>()); // no DataTable/row

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	UMounteaDialogueParticipant* npc = fixture.NpcParticipant;
	TestFalse(TEXT("No graph: cannot start"), IMounteaDialogueParticipantInterface::Execute_CanStartDialogue(npc));
	TestTrue(TEXT("No graph: can still take part"), IMounteaDialogueParticipantInterface::Execute_CanParticipateInDialogue(npc));

	IMounteaDialogueParticipantInterface::Execute_SetDialogueGraph(npc, broken.Graph);
	TestFalse(TEXT("A graph with an invalid node cannot start"), IMounteaDialogueParticipantInterface::Execute_CanStartDialogue(npc));

	IMounteaDialogueParticipantInterface::Execute_SetDialogueGraph(npc, good.Graph);
	TestTrue(TEXT("A valid graph can start"), IMounteaDialogueParticipantInterface::Execute_CanStartDialogue(npc));

	IMounteaDialogueParticipantInterface::Execute_SetParticipantState(npc, EDialogueParticipantState::EDPS_Disabled);
	TestFalse(TEXT("Disabled: cannot start"), IMounteaDialogueParticipantInterface::Execute_CanStartDialogue(npc));
	TestFalse(TEXT("Disabled: cannot take part"), IMounteaDialogueParticipantInterface::Execute_CanParticipateInDialogue(npc));

	fixture.Teardown(*this);
	return true;
}

// The graph must not be swapped out under a running dialogue.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueParticipantGraphLockedWhileActiveTest,
	"Mountea.DialogueSystem.World.Participant.GraphCannotChangeWhileActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueParticipantGraphLockedWhileActiveTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueParticipantTest;

	FTestGraph graph;
	graph.Link(graph.Start(), graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"), { ERowExecutionMode::EREM_AwaitInput }));
	FTestGraph other;
	other.Link(other.Start(), other.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Other")));

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	fixture.StartDialogue();
	TestEqual(TEXT("Precondition: the NPC is Active"), IMounteaDialogueParticipantInterface::Execute_GetParticipantState(fixture.NpcParticipant), EDialogueParticipantState::EDPS_Active);

	IMounteaDialogueParticipantInterface::Execute_SetDialogueGraph(fixture.NpcParticipant, other.Graph);
	TestTrue(TEXT("The graph is unchanged while Active"), IMounteaDialogueParticipantInterface::Execute_GetDialogueGraph(fixture.NpcParticipant) == graph.Graph);

	IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(fixture.Manager);
	IMounteaDialogueParticipantInterface::Execute_SetDialogueGraph(fixture.NpcParticipant, other.Graph);
	TestTrue(TEXT("After the dialogue the graph can change"), IMounteaDialogueParticipantInterface::Execute_GetDialogueGraph(fixture.NpcParticipant) == other.Graph);

	fixture.Teardown(*this);
	return true;
}

// Traversed paths from several sessions accumulate per node; this is what "only first time" conditions read.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueParticipantTraversedPathTest,
	"Mountea.DialogueSystem.World.Participant.TraversedPathAccumulates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueParticipantTraversedPathTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueParticipantTest;

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	const FGuid graphGuid = FGuid::NewGuid();
	const FGuid nodeA = FGuid::NewGuid();
	const FGuid nodeB = FGuid::NewGuid();

	TArray<FDialogueTraversePath> first = { FDialogueTraversePath(nodeA, graphGuid, 1) };
	TArray<FDialogueTraversePath> second = { FDialogueTraversePath(nodeA, graphGuid, 2), FDialogueTraversePath(nodeB, graphGuid, 1) };
	IMounteaDialogueParticipantInterface::Execute_SaveTraversedPath(fixture.NpcParticipant, first);
	IMounteaDialogueParticipantInterface::Execute_SaveTraversedPath(fixture.NpcParticipant, second);

	const TArray<FDialogueTraversePath> saved = IMounteaDialogueParticipantInterface::Execute_GetTraversedPath(fixture.NpcParticipant);
	TestEqual(TEXT("One entry per node"), saved.Num(), 2);
	const FDialogueTraversePath* savedA = saved.FindByKey(FDialogueTraversePath(nodeA, graphGuid));
	const FDialogueTraversePath* savedB = saved.FindByKey(FDialogueTraversePath(nodeB, graphGuid));
	TestTrue(TEXT("Counts add up across saves"), savedA && savedA->TraverseCount == 3);
	TestTrue(TEXT("New nodes are added"), savedB && savedB->TraverseCount == 1);

	const FGuid otherGraph = FGuid::NewGuid();
	TArray<FDialogueTraversePath> sameNodeOtherGraph = { FDialogueTraversePath(nodeA, otherGraph, 1) };
	IMounteaDialogueParticipantInterface::Execute_SaveTraversedPath(fixture.NpcParticipant, sameNodeOtherGraph);
	TestEqual(TEXT("The same node GUID in another graph is a separate entry"),
		IMounteaDialogueParticipantInterface::Execute_GetTraversedPath(fixture.NpcParticipant).Num(), 3);

	fixture.Teardown(*this);
	return true;
}

// Role overrides on the session decide who plays a role (OverrideParticipants decorator); typed lookups must
// prefer them.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueParticipantRoleOverrideTest,
	"Mountea.DialogueSystem.World.Participant.SessionRoleOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueParticipantRoleOverrideTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueParticipantTest;

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	const TScriptInterface<IMounteaDialogueParticipantInterface> player = ToScript(fixture.PlayerParticipant);
	const TScriptInterface<IMounteaDialogueParticipantInterface> npc = ToScript(fixture.NpcParticipant);
	UMounteaDialogueSession* session = fixture.Session;

	TestNull(TEXT("No override by default"), session->GetRoleOverride(EDialogueParticipantType::NPC).GetObject());

	session->SetRoleOverride(EDialogueParticipantType::NPC, player);
	TestTrue(TEXT("The override is stored"), session->GetRoleOverride(EDialogueParticipantType::NPC).GetObject() == fixture.PlayerParticipant);
	TestTrue(TEXT("Typed lookup prefers the override"),
		UMounteaDialogueParticipantStatics::GetParticipantByType({ npc, player }, EDialogueParticipantType::NPC, fixture.Manager).GetObject() == fixture.PlayerParticipant);
	TestNull(TEXT("Other roles are unaffected"), session->GetRoleOverride(EDialogueParticipantType::Environment).GetObject());

	session->SetRoleOverride(EDialogueParticipantType::None, npc);
	TestNull(TEXT("The None role cannot be overridden"), session->GetRoleOverride(EDialogueParticipantType::None).GetObject());

	session->SetRoleOverride(EDialogueParticipantType::NPC, nullptr);
	TestNull(TEXT("Setting null removes the override"), session->GetRoleOverride(EDialogueParticipantType::NPC).GetObject());

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
