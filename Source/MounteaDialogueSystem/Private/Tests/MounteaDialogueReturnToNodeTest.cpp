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
#include "Components/MounteaDialogueManager.h"
#include "Data/MounteaDialogueContext.h"
#include "GameFramework/Actor.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "TimerManager.h"
#include "UObject/Package.h"

// Tests in this file cover Return To Node jumping back to a node that is not one of its children, such as an
// ancestor Lead (Bugreport #5). A Return node has no children, so the generic "select one of the active node's
// children" lookup in HandleSelectNode always failed for it and ended the dialogue.
//
// Nodes and the dialogue context are plain objects; the full session/replication path is too heavy for a unit
// test, so the end-to-end jump is verified in PIE.

struct FMounteaDialogueReturnTestAccess
{
	static void OnDelayDurationExpired(UMounteaDialogueGraphNode_ReturnToNode* Node, const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
	{
		Node->OnDelayDurationExpired(Manager);
	}

	static FTimerHandle& TimerHandle(UMounteaDialogueGraphNode_ReturnToNode* Node)
	{
		return Node->TimerHandle_Delay;
	}
};

namespace MounteaDialogueReturnToNodeTest
{
	template<typename TNode>
	static TNode* NewNode()
	{
		TNode* node = NewObject<TNode>(GetTransientPackage());
		node->SetNodeGUID(FGuid::NewGuid());
		return node;
	}

	static UMounteaDialogueContext* NewContext()
	{
		return NewObject<UMounteaDialogueContext>(GetTransientPackage());
	}
}

// The existing behaviour must not change: a current child is still selectable.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFindSelectableChildTest,
	"Mountea.DialogueSystem.ReturnToNode.FindSelectableNode.ChildStillResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFindSelectableChildTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueReturnToNodeTest;

	UMounteaDialogueGraphNode_LeadNode* lead = NewNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_AnswerNode* answerA = NewNode<UMounteaDialogueGraphNode_AnswerNode>();
	UMounteaDialogueGraphNode_AnswerNode* answerB = NewNode<UMounteaDialogueGraphNode_AnswerNode>();

	UMounteaDialogueContext* context = NewContext();
	context->SetDialogueContext(lead, { answerA, answerB });

	TestTrue(TEXT("A current child resolves to itself"),
		UMounteaDialogueTraversalStatics::FindSelectableNode(context, answerB->GetNodeGUID()) == answerB);
	TestNull(TEXT("A GUID that is nowhere in the context resolves to null"),
		UMounteaDialogueTraversalStatics::FindSelectableNode(context, FGuid::NewGuid()));
	TestNull(TEXT("A null context is tolerated"),
		UMounteaDialogueTraversalStatics::FindSelectableNode(nullptr, answerA->GetNodeGUID()));
	return true;
}

// Direct regression for Bugreport #5: Start -> Lead -> Answer A -> Return To Node (target = Lead). The Return
// node is active, has no children, and its target is an ancestor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFindSelectableReturnTargetTest,
	"Mountea.DialogueSystem.ReturnToNode.FindSelectableNode.ReturnTargetResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFindSelectableReturnTargetTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueReturnToNodeTest;

	UMounteaDialogueGraphNode_LeadNode* lead = NewNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = NewNode<UMounteaDialogueGraphNode_ReturnToNode>();
	returnNode->SelectedNode = lead;

	UMounteaDialogueContext* context = NewContext();
	context->SetDialogueContext(returnNode, {});

	TestTrue(TEXT("The Return node's own target resolves even though it is not one of its children"),
		UMounteaDialogueTraversalStatics::FindSelectableNode(context, lead->GetNodeGUID()) == lead);
	return true;
}

// The exception must stay narrow: SelectNode is client-callable, so it must not become "jump anywhere".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueFindSelectableRejectsOthersTest,
	"Mountea.DialogueSystem.ReturnToNode.FindSelectableNode.RejectsAnythingElse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueFindSelectableRejectsOthersTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueReturnToNodeTest;

	UMounteaDialogueGraphNode_LeadNode* lead = NewNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_LeadNode* otherLead = NewNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = NewNode<UMounteaDialogueGraphNode_ReturnToNode>();
	returnNode->SelectedNode = lead;

	UMounteaDialogueContext* returnContext = NewContext();
	returnContext->SetDialogueContext(returnNode, {});
	TestNull(TEXT("With a Return node active, a node that is not its target is rejected"),
		UMounteaDialogueTraversalStatics::FindSelectableNode(returnContext, otherLead->GetNodeGUID()));

	returnNode->SelectedNode = nullptr;
	TestNull(TEXT("A Return node with no target selects nothing"),
		UMounteaDialogueTraversalStatics::FindSelectableNode(returnContext, lead->GetNodeGUID()));

	UMounteaDialogueGraphNode_AnswerNode* answer = NewNode<UMounteaDialogueGraphNode_AnswerNode>();
	UMounteaDialogueContext* answerContext = NewContext();
	answerContext->SetDialogueContext(answer, {});
	TestNull(TEXT("With a non-Return node active, an ancestor is not selectable (the exception is Return-only)"),
		UMounteaDialogueTraversalStatics::FindSelectableNode(answerContext, lead->GetNodeGUID()));
	return true;
}

// A Return node's delay timer can fire after the node is no longer active (already jumped, dialogue closed and
// restarted, or SelectNode sent early by a client). It must then do nothing instead of failing the dialogue.
// Uses Auto Complete Selected Node because that path changes the context directly, which makes it observable.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueReturnStaleTimerTest,
	"Mountea.DialogueSystem.ReturnToNode.ReturnNode.StaleTimerDoesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueReturnStaleTimerTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueReturnToNodeTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	// The control step below lets an active Return node call NodeProcessed on the real manager; the test world
	// has no GameState session, so the manager reports that. Expected here and unrelated to what is under test.
	AddExpectedError(TEXT("Missing Dialogue Session on GameState"), EAutomationExpectedErrorFlags::Contains, 0);

	AActor* owner = fixture.GetWorld()->SpawnActor<AActor>();
	UMounteaDialogueManager* manager = NewObject<UMounteaDialogueManager>(owner);
	owner->AddInstanceComponent(manager);
	manager->RegisterComponent();

	TScriptInterface<IMounteaDialogueManagerInterface> managerScript;
	managerScript.SetObject(manager);
	managerScript.SetInterface(manager);

	UMounteaDialogueGraphNode_LeadNode* target = NewNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_LeadNode* currentlyActive = NewNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = NewNode<UMounteaDialogueGraphNode_ReturnToNode>();
	returnNode->SelectedNode = target;
	returnNode->bAutoCompleteSelectedNode = true;

	UMounteaDialogueContext* context = NewContext();
	manager->SetDialogueContext(context);

	// Stale: some other node is active now.
	context->SetDialogueContext(currentlyActive, {});
	FMounteaDialogueReturnTestAccess::OnDelayDurationExpired(returnNode, managerScript);
	TestTrue(TEXT("A Return node that is no longer active must not touch the context"), context->ActiveNode == currentlyActive);

	// Control: when the Return node IS active, the existing behaviour is untouched.
	context->SetDialogueContext(returnNode, {});
	FMounteaDialogueReturnTestAccess::OnDelayDurationExpired(returnNode, managerScript);
	TestTrue(TEXT("An active Return node with auto-complete on still switches to its target"), context->ActiveNode == target);

	fixture.Teardown(*this);
	return true;
}

// Cleanup must cancel a pending jump, otherwise the timer outlives the node's activity.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueReturnCleanupClearsTimerTest,
	"Mountea.DialogueSystem.ReturnToNode.ReturnNode.CleanupClearsPendingJump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueReturnCleanupClearsTimerTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueReturnToNodeTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	UWorld* world = fixture.GetWorld();
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = NewNode<UMounteaDialogueGraphNode_ReturnToNode>();
	returnNode->SetNewWorld(world);

	FTimerHandle& handle = FMounteaDialogueReturnTestAccess::TimerHandle(returnNode);
	world->GetTimerManager().SetTimer(handle, FTimerDelegate::CreateLambda([]() {}), 60.f, false);
	TestTrue(TEXT("Precondition: the jump timer is pending"), world->GetTimerManager().IsTimerActive(handle));

	returnNode->CleanupNode();

	TestFalse(TEXT("CleanupNode must cancel the pending jump"), world->GetTimerManager().IsTimerActive(handle));

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
