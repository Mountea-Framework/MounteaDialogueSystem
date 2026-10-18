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
#include "Data/MounteaDialogueContextPayload.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Helpers/MounteaDialogueContextStatics.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Serialization/BufferArchive.h"

// The dialogue context (server-side state of a running dialogue) and the replicated payload it is built from.

// Validity, node switching and the previous-node memory used by the UI's "back" logic.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueContextStateTest,
	"Mountea.DialogueSystem.Context.ActiveNodeAndValidity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueContextStateTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));
	UMounteaDialogueParticipant* participant = NewObject<UMounteaDialogueParticipant>(GetTransientPackage());
	TScriptInterface<IMounteaDialogueParticipantInterface> participantScript;
	participantScript.SetObject(participant);
	participantScript.SetInterface(participant);

	UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(GetTransientPackage());
	TestFalse(TEXT("An empty context is invalid"), UMounteaDialogueContextStatics::IsContextValid(context));
	TestFalse(TEXT("A null context is invalid"), UMounteaDialogueContextStatics::IsContextValid(nullptr));

	context->SetDialogueContext(lead, { answer });
	TestFalse(TEXT("A context without participants is invalid"), context->IsValid());

	TestTrue(TEXT("Adding a participant succeeds"), context->AddDialogueParticipant(participantScript));
	TestFalse(TEXT("Adding the same participant twice is refused"), context->AddDialogueParticipant(participantScript));
	TestTrue(TEXT("Active node plus a participant is valid"), context->IsValid());
	TestEqual(TEXT("Allowed children are stored"), context->AllowedChildNodes.Num(), 1);

	context->SetDialogueContext(answer, {});
	TestEqual(TEXT("Switching nodes remembers the previous one"), context->PreviousActiveNode, lead->GetNodeGUID());
	TestEqual(TEXT("Allowed children are replaced"), context->AllowedChildNodes.Num(), 0);

	TestTrue(TEXT("Removing a participant succeeds"), context->RemoveDialogueParticipant(participantScript));
	TestFalse(TEXT("Removing it again is refused"), context->RemoveDialogueParticipant(participantScript));
	context->AddDialogueParticipant(participantScript);
	context->ClearDialogueParticipants();
	TestEqual(TEXT("Clear removes every participant"), context->DialogueParticipants.Num(), 0);

	context->UpdateActiveDialogueRowDataIndex(3);
	TestEqual(TEXT("Row line index is stored"), context->GetActiveDialogueRowDataIndex(), 3);
	TestFalse(TEXT("ToString does not crash and says something"), context->ToString().IsEmpty());
	return true;
}

// The traversed path counts visits per (node, graph) pair; Only First Time and saved paths depend on it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueContextTraversedPathTest,
	"Mountea.DialogueSystem.Context.TraversedPathCountsVisits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueContextTraversedPathTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));

	UMounteaDialogueContext* context = NewObject<UMounteaDialogueContext>(GetTransientPackage());
	context->AddTraversedNode(lead);
	context->AddTraversedNode(lead);
	context->AddTraversedNode(answer);
	context->AddTraversedNode(nullptr);

	TestEqual(TEXT("One entry per node"), context->TraversedPath.Num(), 2);
	const FDialogueTraversePath* leadEntry = context->TraversedPath.FindByKey(FDialogueTraversePath(lead->GetNodeGUID(), graph.Graph->GetGraphGUID()));
	TestTrue(TEXT("Repeat visits are counted"), leadEntry && leadEntry->TraverseCount == 2);
	TestTrue(TEXT("HasNodeBeenTraversed sees the visit"), UMounteaDialogueTraversalStatics::HasNodeBeenTraversed(answer, context->TraversedPath));

	UMounteaDialogueGraphNode_LeadNode* detached = NewObject<UMounteaDialogueGraphNode_LeadNode>(GetTransientPackage());
	context->AddTraversedNode(detached);
	TestEqual(TEXT("A node without a graph is not recorded"), context->TraversedPath.Num(), 2);
	return true;
}

// The payload is what clients receive; Reset must bring it back to "no dialogue".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueContextPayloadTest,
	"Mountea.DialogueSystem.Context.PayloadValidityAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueContextPayloadTest::RunTest(const FString& Parameters)
{
	FMounteaDialogueContextPayload payload;
	TestFalse(TEXT("A default payload is invalid"), payload.IsValid());

	payload.SessionGUID = FGuid::NewGuid();
	payload.ActiveNodeGUID = FGuid::NewGuid();
	payload.ActiveGraphGUID = FGuid::NewGuid();
	payload.RootGraph = NewObject<UMounteaDialogueGraph>(GetTransientPackage());
	payload.AllowedChildNodeGUIDs.Add(FGuid::NewGuid());
	payload.ActiveDialogueRowDataIndex = 2;
	payload.ContextVersion = 7;
	TestTrue(TEXT("A payload with a session and node is valid"), payload.IsValid());

	payload.Reset();
	TestFalse(TEXT("Reset clears the session"), payload.SessionGUID.IsValid());
	TestFalse(TEXT("Reset clears the node"), payload.ActiveNodeGUID.IsValid());
	TestEqual(TEXT("Reset clears the options"), payload.AllowedChildNodeGUIDs.Num(), 0);
	TestNull(TEXT("Reset clears the root graph"), payload.RootGraph.Get());
	TestEqual(TEXT("Reset clears the line index"), payload.ActiveDialogueRowDataIndex, 0);
	TestFalse(TEXT("A reset payload is invalid"), payload.IsValid());

	FBufferArchive archive;
	bool bSuccess = true;
	TestFalse(TEXT("NetSerialize without a package map refuses"), payload.NetSerialize(archive, nullptr, bSuccess));
	TestFalse(TEXT("and reports failure"), bSuccess);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
