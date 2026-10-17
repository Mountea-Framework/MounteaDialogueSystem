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
#include "Decorators/MounteaDialogueDecorator_OverrideDialogue.h"
#include "Decorators/MounteaDialogueDecorator_OverrideParticipants.h"
#include "Decorators/MounteaDialogueDecorator_SaveNodeAsStart.h"
#include "Decorators/MounteaDialogueDecorator_SelectRandomDialogueRow.h"
#include "Decorators/MounteaDialogueDecorator_SendCommand.h"
#include "Decorators/MounteaIDialogueDecorator_SwapParticipants.h"
#include "GameplayTagContainer.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

// Each built-in decorator: where it may be placed (validation) and what it does to the running dialogue
// (execution). Execution runs against a real manager whose context is set by hand.

namespace MounteaDialogueDecoratorTest
{
	using namespace MounteaDialogueTest;

	static TArray<FText> Validate(UMounteaDialogueDecoratorBase* Decorator, bool& bOutValid)
	{
		TArray<FText> messages;
		bOutValid = Decorator->ValidateDecorator(messages);
		return messages;
	}

	static bool AnyContains(const TArray<FText>& Messages, const TCHAR* Text)
	{
		return Messages.ContainsByPredicate([Text](const FText& Message) { return Message.ToString().Contains(Text); });
	}

	template<typename TParticipant>
	static TScriptInterface<IMounteaDialogueParticipantInterface> ToScript(TParticipant* Participant)
	{
		TScriptInterface<IMounteaDialogueParticipantInterface> script;
		script.SetObject(Participant);
		script.SetInterface(Participant);
		return script;
	}

	static TScriptInterface<IMounteaDialogueManagerInterface> ToScript(UMounteaDialogueManager* Manager)
	{
		TScriptInterface<IMounteaDialogueManagerInterface> script;
		script.SetObject(Manager);
		script.SetInterface(Manager);
		return script;
	}

	// A manager whose context points at Node with NPC (active) and player participants.
	static UMounteaDialogueContext* GiveManagerContext(FDialogueWorldFixture& Fixture, UMounteaDialogueGraphNode* Node)
	{
		UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(Fixture.Manager);
		context->DialogueParticipants = { ToScript(Fixture.NpcParticipant), ToScript(Fixture.PlayerParticipant) };
		context->SetActiveDialogueParticipant(ToScript(Fixture.NpcParticipant));
		context->SetDialogueContext(Node, {});
		context->UpdateActiveDialogueRow(UMounteaDialogueTraversalStatics::GetSpeechData(Node));
		Fixture.Manager->SetDialogueContext(context);
		return context;
	}
}

// The decorator lifecycle every subclass relies on: Initialize gives it a world and manager, Cleanup takes them
// away again.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueDecoratorLifecycleTest,
	"Mountea.DialogueSystem.Decorator.Base.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueDecoratorLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueDecoratorTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueDecorator_SwapParticipants* decorator = AddDecorator<UMounteaDialogueDecorator_SwapParticipants>(lead, lead->NodeDecorators);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	TestTrue(TEXT("The owning node is its outer"), decorator->GetOwningNode() == lead);
	TestTrue(TEXT("The owning graph is the node's outer"), decorator->GetOwningGraph() == graph.Graph);
	TestFalse(TEXT("Not usable before initialization"), decorator->EvaluateDecorator());

	decorator->InitializeDecorator(fixture.GetWorld(), ToScript(fixture.NpcParticipant), ToScript(fixture.Manager));
	TestTrue(TEXT("Usable after initialization with a world"), decorator->EvaluateDecorator());
	TestTrue(TEXT("Node-level evaluation passes"), lead->EvaluateDecorators());

	decorator->CleanupDecorator();
	TestFalse(TEXT("Not usable after cleanup"), decorator->EvaluateDecorator());
	TestFalse(TEXT("Node-level evaluation fails with an unusable decorator"), lead->EvaluateDecorators());

	TestFalse(TEXT("Not stackable by default"), decorator->IsDecoratorStackable());
	TestTrue(TEXT("Allowed on graphs by default"), decorator->IsDecoratorAllowedForGraph());
	TestTrue(TEXT("Send Command is stackable"), GetDefault<UMounteaDialogueDecorator_SendCommand>()->IsDecoratorStackable());
	TestFalse(TEXT("Override Dialogue is node-only"), GetDefault<UMounteaDialogueDecorator_OverrideDialogue>()->IsDecoratorAllowedForGraph());

	fixture.Teardown(*this);
	return true;
}

// Override Dialogue: needs a table and row, and must not sit on Start or Return nodes.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueDecoratorOverrideDialogueTest,
	"Mountea.DialogueSystem.Decorator.OverrideDialogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueDecoratorOverrideDialogueTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueDecoratorTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	const FName overrideRow = graph.AddRow(TEXT("Override"), { ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic });
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = graph.AddNode<UMounteaDialogueGraphNode_ReturnToNode>();

	bool bValid = true;
	{
		UMounteaDialogueDecorator_OverrideDialogue* onGraph = NewObject<UMounteaDialogueDecorator_OverrideDialogue>(graph.Graph);
		const TArray<FText> messages = Validate(onGraph, bValid);
		TestFalse(TEXT("[Graph] invalid"), bValid);
		TestTrue(TEXT("[Graph] not allowed on graphs"), AnyContains(messages, TEXT("not allowed in Graph Decorators")));
		TestTrue(TEXT("[Graph] missing table"), AnyContains(messages, TEXT("has no Data Table")));
		TestTrue(TEXT("[Graph] missing row"), AnyContains(messages, TEXT("Invalid Row Name")));
	}
	{
		UMounteaDialogueDecorator_OverrideDialogue* onStart = NewObject<UMounteaDialogueDecorator_OverrideDialogue>(graph.Start());
		SetProperty(*this, onStart, TEXT("DataTable"), TObjectPtr<UDataTable>(graph.Table));
		SetProperty(*this, onStart, TEXT("RowName"), overrideRow);
		const TArray<FText> messages = Validate(onStart, bValid);
		TestFalse(TEXT("[Start] invalid"), bValid);
		TestTrue(TEXT("[Start] not allowed on Start"), AnyContains(messages, TEXT("not allowed for Start Nodes")));
	}
	{
		UMounteaDialogueDecorator_OverrideDialogue* onReturn = NewObject<UMounteaDialogueDecorator_OverrideDialogue>(returnNode);
		SetProperty(*this, onReturn, TEXT("DataTable"), TObjectPtr<UDataTable>(graph.Table));
		SetProperty(*this, onReturn, TEXT("RowName"), overrideRow);
		const TArray<FText> messages = Validate(onReturn, bValid);
		TestFalse(TEXT("[Return] invalid"), bValid);
		TestTrue(TEXT("[Return] the message names Return nodes"), AnyContains(messages, TEXT("not allowed for Return Nodes")));
	}

	UMounteaDialogueDecorator_OverrideDialogue* decorator = NewObject<UMounteaDialogueDecorator_OverrideDialogue>(lead);
	SetProperty(*this, decorator, TEXT("DataTable"), TObjectPtr<UDataTable>(graph.Table));
	SetProperty(*this, decorator, TEXT("RowName"), overrideRow);
	Validate(decorator, bValid);
	TestTrue(TEXT("[Lead] valid"), bValid);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	UMounteaDialogueContext* context = GiveManagerContext(fixture, lead);
	decorator->InitializeDecorator(fixture.GetWorld(), ToScript(fixture.NpcParticipant), ToScript(fixture.Manager));
	decorator->ExecuteDecorator();
	TestEqual(TEXT("The active row is replaced by the override row"), context->ActiveDialogueRow.RowData.Num(), 3);
	TestEqual(TEXT("The active table handle points at the override row"), context->ActiveDialogueTableHandle.RowName, overrideRow);

	fixture.Teardown(*this);
	return true;
}

// Select Random Dialogue Row picks a line index within the configured range, clamped to the row.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueDecoratorSelectRandomRowTest,
	"Mountea.DialogueSystem.Decorator.SelectRandomDialogueRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueDecoratorSelectRandomRowTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueDecoratorTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_Automatic });

	bool bValid = true;
	Validate(NewObject<UMounteaDialogueDecorator_SelectRandomDialogueRow>(graph.Graph), bValid);
	TestFalse(TEXT("Not allowed as a graph decorator"), bValid);

	UMounteaDialogueDecorator_SelectRandomDialogueRow* decorator = NewObject<UMounteaDialogueDecorator_SelectRandomDialogueRow>(lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	UMounteaDialogueContext* context = GiveManagerContext(fixture, lead);
	decorator->InitializeDecorator(fixture.GetWorld(), ToScript(fixture.NpcParticipant), ToScript(fixture.Manager));

	SetProperty(*this, decorator, TEXT("RandomRange"), FIntPoint(1, 1));
	decorator->ExecuteDecorator();
	TestEqual(TEXT("A one-value range picks that line"), context->ActiveDialogueRowDataIndex, 1);

	SetProperty(*this, decorator, TEXT("RandomRange"), FIntPoint(2, 2));
	decorator->ExecuteDecorator();
	TestEqual(TEXT("The last line can be picked"), context->ActiveDialogueRowDataIndex, 2);

	SetProperty(*this, decorator, TEXT("RandomRange"), FIntPoint(10, 5));
	for (int32 i = 0; i < 20; ++i)
	{
		decorator->ExecuteDecorator();
		if (!TestTrue(TEXT("A reversed, out-of-range range is clamped to the row"), context->ActiveDialogueRowDataIndex >= 0 && context->ActiveDialogueRowDataIndex <= 2))
			break;
	}

	fixture.Teardown(*this);
	return true;
}

// Save Node As Start makes the graph owner remember this node as its next starting point.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueDecoratorSaveNodeAsStartTest,
	"Mountea.DialogueSystem.Decorator.SaveNodeAsStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueDecoratorSaveNodeAsStartTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueDecoratorTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_CompleteNode* complete = graph.AddSpeech<UMounteaDialogueGraphNode_CompleteNode>(TEXT("Complete"));
	graph.Link(graph.Start(), lead);
	graph.Link(lead, complete);

	bool bValid = true;
	Validate(NewObject<UMounteaDialogueDecorator_SaveNodeAsStart>(graph.Graph), bValid);
	TestFalse(TEXT("Not allowed as a graph decorator"), bValid);
	const TArray<FText> completeMessages = Validate(NewObject<UMounteaDialogueDecorator_SaveNodeAsStart>(complete), bValid);
	TestFalse(TEXT("Not allowed on Complete nodes"), bValid);
	TestTrue(TEXT("Says why"), AnyContains(completeMessages, TEXT("Complete Dialogue Nodes")));

	UMounteaDialogueDecorator_SaveNodeAsStart* decorator = NewObject<UMounteaDialogueDecorator_SaveNodeAsStart>(lead);
	Validate(decorator, bValid);
	TestTrue(TEXT("Valid on a Lead"), bValid);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, graph.Graph))
	{
		fixture.Teardown(*this);
		return false;
	}

	GiveManagerContext(fixture, lead);
	decorator->InitializeDecorator(fixture.GetWorld(), ToScript(fixture.NpcParticipant), ToScript(fixture.Manager));
	decorator->ExecuteDecorator();
	TestTrue(TEXT("The graph owner saved the node"), IMounteaDialogueParticipantInterface::Execute_GetSavedStartingNode(fixture.NpcParticipant) == lead);

	fixture.Teardown(*this);
	return true;
}

// Swap Participants hands the active speaker role to the participant with the configured tag.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueDecoratorSwapParticipantsTest,
	"Mountea.DialogueSystem.Decorator.SwapParticipants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueDecoratorSwapParticipantsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueDecoratorTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueDecorator_SwapParticipants* decorator = NewObject<UMounteaDialogueDecorator_SwapParticipants>(lead);

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	// Any two registered tags will do; these ship with the plugin.
	const FGameplayTag npcTag = FGameplayTag::RequestGameplayTag(TEXT("Mountea_Dialogue.Graph.Type.Dialogue"));
	const FGameplayTag playerTag = FGameplayTag::RequestGameplayTag(TEXT("Mountea_Dialogue.Graph.Type.Monologue"));
	fixture.NpcParticipant->ParticipantTag = npcTag;
	fixture.PlayerParticipant->ParticipantTag = playerTag;

	UMounteaDialogueContext* context = GiveManagerContext(fixture, lead);
	decorator->InitializeDecorator(fixture.GetWorld(), ToScript(fixture.NpcParticipant), ToScript(fixture.Manager));

	SetProperty(*this, decorator, TEXT("NewParticipantTag"), playerTag);
	decorator->ExecuteDecorator();
	TestTrue(TEXT("The participant with the tag becomes active"), context->ActiveDialogueParticipant.GetObject() == fixture.PlayerParticipant);

	SetProperty(*this, decorator, TEXT("NewParticipantTag"), FGameplayTag::RequestGameplayTag(TEXT("Mountea_Dialogue.Graph.Type")));
	decorator->ExecuteDecorator();
	TestTrue(TEXT("An unmatched tag changes nothing"), context->ActiveDialogueParticipant.GetObject() == fixture.PlayerParticipant);

	fixture.Teardown(*this);
	return true;
}

// Send Command needs a command; it forwards it to the owning participant.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueDecoratorSendCommandTest,
	"Mountea.DialogueSystem.Decorator.SendCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueDecoratorSendCommandTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueDecoratorTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueDecorator_SendCommand* decorator = NewObject<UMounteaDialogueDecorator_SendCommand>(lead);

	bool bValid = true;
	const TArray<FText> messages = Validate(decorator, bValid);
	TestFalse(TEXT("An empty command is invalid"), bValid);
	TestTrue(TEXT("Says why"), AnyContains(messages, TEXT("StringCommand is empty")));

	SetProperty(*this, decorator, TEXT("Command"), FString(TEXT("Wave")));
	Validate(decorator, bValid);
	TestTrue(TEXT("A command makes it valid"), bValid);
	return true;
}

// Override Participants: every enabled override needs an actor with a participant; one valid override must not
// hide another invalid one.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueDecoratorOverrideParticipantsTest,
	"Mountea.DialogueSystem.Decorator.OverrideParticipants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueDecoratorOverrideParticipantsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueDecoratorTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));

	FDialogueWorldFixture fixture;
	if (!fixture.Setup(*this, nullptr))
	{
		fixture.Teardown(*this);
		return false;
	}

	AActor* stranger = fixture.GetWorld()->SpawnActor<AActor>();
	UMounteaDialogueParticipant* strangerParticipant = FDialogueWorldFixture::AddComponent<UMounteaDialogueParticipant>(stranger);

	bool bValid = true;
	{
		UMounteaDialogueDecorator_OverrideParticipants* decorator = NewObject<UMounteaDialogueDecorator_OverrideParticipants>(lead);
		SetProperty(*this, decorator, TEXT("bOverridePlayerParticipant"), true);
		const TArray<FText> messages = Validate(decorator, bValid);
		TestFalse(TEXT("An enabled override without an actor is invalid"), bValid);
		TestTrue(TEXT("Says why"), AnyContains(messages, TEXT("Actor reference is Empty")));

		SetProperty(*this, decorator, TEXT("bOverrideDialogueParticipant"), true);
		SetProperty(*this, decorator, TEXT("NewDialogueParticipant"), TSoftObjectPtr<AActor>(stranger));
		Validate(decorator, bValid);
		TestFalse(TEXT("A later valid override does not hide an earlier invalid one"), bValid);
	}

	UMounteaDialogueDecorator_OverrideParticipants* decorator = NewObject<UMounteaDialogueDecorator_OverrideParticipants>(lead);
	SetProperty(*this, decorator, TEXT("bOverrideDialogueParticipant"), true);
	SetProperty(*this, decorator, TEXT("NewDialogueParticipant"), TSoftObjectPtr<AActor>(stranger));
	SetProperty(*this, decorator, TEXT("bOverrideActiveParticipant"), true);
	SetProperty(*this, decorator, TEXT("NewActiveParticipant"), TSoftObjectPtr<AActor>(stranger));
	Validate(decorator, bValid);
	TestTrue(TEXT("Valid overrides validate"), bValid);

	UMounteaDialogueContext* context = GiveManagerContext(fixture, lead);
	decorator->InitializeDecorator(fixture.GetWorld(), ToScript(fixture.NpcParticipant), ToScript(fixture.Manager));
	decorator->ExecuteDecorator();

	TestTrue(TEXT("The override joins the dialogue"), context->DialogueParticipants.Contains(ToScript(strangerParticipant)));
	TestTrue(TEXT("The override becomes the active speaker"), context->ActiveDialogueParticipant.GetObject() == strangerParticipant);
	TestTrue(TEXT("The session records the NPC role override"), fixture.Session->GetRoleOverride(EDialogueParticipantType::NPC).GetObject() == strangerParticipant);

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
