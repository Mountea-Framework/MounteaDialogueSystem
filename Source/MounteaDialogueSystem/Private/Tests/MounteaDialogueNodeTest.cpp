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
#include "EdGraph/EdGraphPin.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

// Per-node-type rules: which nodes may connect, which start on their own, and each type's own validation.

namespace MounteaDialogueNodeTest
{
	// GetRowNames is a private UFUNCTION feeding the details panel's GetOptions; call it the way the panel does.
	static TArray<FString> GetOptions(UObject* Object)
	{
		struct FParams { TArray<FString> ReturnValue; } params;
		if (UFunction* function = Object->FindFunction(TEXT("GetRowNames")))
			Object->ProcessEvent(function, &params);
		return params.ReturnValue;
	}
}

// The editor asks the source node whether a link to Other is allowed; the answer comes from Other's allowed
// input classes and the source's child limit.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueNodeConnectionRulesTest,
	"Mountea.DialogueSystem.Node.ConnectionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueNodeConnectionRulesTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode* start = graph.Start();
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));
	UMounteaDialogueGraphNode_CompleteNode* complete = graph.AddSpeech<UMounteaDialogueGraphNode_CompleteNode>(TEXT("Complete"));
	UMounteaDialogueGraphNode_Delay* delay = graph.AddNode<UMounteaDialogueGraphNode_Delay>();

	FText error;
	TestTrue(TEXT("Lead accepts input from Start"), lead->CanCreateConnection(start, EGPD_Output, error));
	TestTrue(TEXT("Answer accepts input from Lead"), answer->CanCreateConnection(lead, EGPD_Output, error));
	TestTrue(TEXT("Complete accepts input from any speech node"), complete->CanCreateConnection(answer, EGPD_Output, error));
	TestTrue(TEXT("Speech nodes accept input from a Delay"), complete->CanCreateConnection(delay, EGPD_Output, error));
	TestFalse(TEXT("Start accepts no input at all"), start->CanCreateConnection(lead, EGPD_Output, error));
	TestFalse(TEXT("The refusal explains itself"), error.IsEmpty());
	TestTrue(TEXT("Delay accepts input from anything"), delay->CanCreateConnection(complete, EGPD_Output, error));

	FText nullError;
	TestFalse(TEXT("A null node is refused"), lead->CanCreateConnection(nullptr, EGPD_Output, nullError));
	TestFalse(TEXT("The null refusal explains itself"), nullError.IsEmpty());

	// Start may have one child; once it has one, nothing else may link from it.
	graph.Link(start, lead);
	FText limitError;
	TestFalse(TEXT("A full Start node refuses a second child"), answer->CanCreateConnection(start, EGPD_Output, limitError));
	TestTrue(TEXT("The limit is named"), limitError.ToString().Contains(TEXT("Cannot have more than 1")));
	return true;
}

// Auto-start decides whether a child runs on its own or is offered as an option; the traversal depends on it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueNodeAutoStartTest,
	"Mountea.DialogueSystem.Node.AutoStartDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueNodeAutoStartTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Lead starts on its own"), GetDefault<UMounteaDialogueGraphNode_LeadNode>()->DoesAutoStart());
	TestFalse(TEXT("Answer waits for the player"), GetDefault<UMounteaDialogueGraphNode_AnswerNode>()->DoesAutoStart());
	TestFalse(TEXT("Complete waits for the player"), GetDefault<UMounteaDialogueGraphNode_CompleteNode>()->DoesAutoStart());
	TestTrue(TEXT("Delay starts on its own"), GetDefault<UMounteaDialogueGraphNode_Delay>()->DoesAutoStart());
	TestTrue(TEXT("Return To Node starts on its own"), GetDefault<UMounteaDialogueGraphNode_ReturnToNode>()->DoesAutoStart());
	TestTrue(TEXT("Open Child Graph starts on its own"), GetDefault<UMounteaDialogueGraphNode_OpenChildGraph>()->DoesAutoStart());
	TestFalse(TEXT("Editable Dialogue Node waits for the player"), GetDefault<UMounteaDialogueGraphNode_DialogueNode>()->DoesAutoStart());

	TestEqual(TEXT("Start allows one child"), GetDefault<UMounteaDialogueGraphNode_StartNode>()->GetMaxChildNodes(), 1);
	TestEqual(TEXT("Delay allows one child"), GetDefault<UMounteaDialogueGraphNode_Delay>()->GetMaxChildNodes(), 1);
	TestEqual(TEXT("Open Child Graph allows no children"), GetDefault<UMounteaDialogueGraphNode_OpenChildGraph>()->GetMaxChildNodes(), 0);
	TestEqual(TEXT("Lead has no child limit"), GetDefault<UMounteaDialogueGraphNode_LeadNode>()->GetMaxChildNodes(), -1);
	TestFalse(TEXT("Return To Node does not inherit graph decorators"), GetDefault<UMounteaDialogueGraphNode_ReturnToNode>()->DoesInheritDecorators());
	return true;
}

// Speech nodes read their row from the DataTable; the base type refuses direct row overrides, the editable
// Dialogue Node accepts them.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueNodeSpeechDataTest,
	"Mountea.DialogueSystem.Node.SpeechData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueNodeSpeechDataTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;
	FScopedDialogueLogMute mute; // SetSpeechData on a DataTable node logs an error by design

	FTestGraph graph;
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"),
		{ ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_AwaitInput });
	graph.AddRow(TEXT("Other"), { ERowExecutionMode::EREM_Automatic });

	const FDialogueRow row = IMounteaDialogueSpeechDataInterface::Execute_GetSpeechData(lead);
	TestEqual(TEXT("The row comes from the table"), row.RowData.Num(), 2);
	TestEqual(TEXT("Row lines keep their execution mode"), row.RowData[1].RowExecutionBehaviour, ERowExecutionMode::EREM_AwaitInput);
	TestEqual(TEXT("Previews list each line's text"), lead->GetPreviews().Num(), 2);
	TestTrue(TEXT("The node validates at runtime"), lead->ValidateNodeRuntime());

	FDialogueRow replacement;
	replacement.RowTitle = FText::FromString(TEXT("Replacement"));
	TestFalse(TEXT("DataTable nodes refuse direct row overrides"), IMounteaDialogueSpeechDataInterface::Execute_SetSpeechData(lead, replacement));

	UMounteaDialogueGraphNode_DialogueNode* editable = graph.AddNode<UMounteaDialogueGraphNode_DialogueNode>();
	TestTrue(TEXT("The editable node accepts a row"), IMounteaDialogueSpeechDataInterface::Execute_SetSpeechData(editable, replacement));
	TestTrue(TEXT("The editable node returns it"),
		IMounteaDialogueSpeechDataInterface::Execute_GetSpeechData(editable).RowTitle.EqualTo(replacement.RowTitle));
	TestFalse(TEXT("Setting the same row again reports no change"), IMounteaDialogueSpeechDataInterface::Execute_SetSpeechData(editable, replacement));

	UMounteaDialogueGraphNode_LeadNode* noTable = graph.AddNode<UMounteaDialogueGraphNode_LeadNode>();
	TestFalse(TEXT("Without a table the node is not runnable"), noTable->ValidateNodeRuntime());
	return true;
}

// Delay must have a positive duration; SetDelayDuration clamps to at least one second.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueNodeDelayTest,
	"Mountea.DialogueSystem.Node.DelayDuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueNodeDelayTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	UMounteaDialogueGraphNode_Delay* delay = graph.AddNode<UMounteaDialogueGraphNode_Delay>();
	graph.Link(graph.Start(), delay);
	graph.Link(delay, graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead")));

	delay->SetDelayDuration(0);
	TestEqual(TEXT("Zero is clamped to one second"), delay->GetDelayDuration(), 1);
	delay->SetDelayDuration(-5);
	TestEqual(TEXT("Negative values are clamped to one second"), delay->GetDelayDuration(), 1);
	delay->SetDelayDuration(4);
	TestEqual(TEXT("Positive values are kept"), delay->GetDelayDuration(), 4);
	TestTrue(TEXT("The title shows the duration"), delay->GetNodeTitle().ToString().Contains(TEXT("4")));

	FDataValidationContext context;
	TestTrue(TEXT("A connected delay with a positive duration is valid"), static_cast<UMounteaDialogueGraphNode*>(delay)->ValidateNode(context, false));
	return true;
}

// Open Child Graph must point at a graph, and must not form a cycle back to its own graph.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueNodeOpenChildGraphTest,
	"Mountea.DialogueSystem.Node.OpenChildGraphReferences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueNodeOpenChildGraphTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph parent;
	FTestGraph child;
	UMounteaDialogueGraphNode_LeadNode* lead = parent.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));
	UMounteaDialogueGraphNode_OpenChildGraph* open = parent.AddNode<UMounteaDialogueGraphNode_OpenChildGraph>();
	parent.Link(parent.Start(), lead);
	parent.Link(lead, open);

	{
		FDataValidationContext context;
		TestFalse(TEXT("[No target] invalid"), open->ValidateNode(context, false));
		TestTrue(TEXT("[No target] says so"), JoinIssues(context).Contains(TEXT("Target Dialogue is not set")));
	}

	open->TargetDialogue = child.Graph;
	{
		FDataValidationContext context;
		TestTrue(TEXT("[Plain target] valid"), open->ValidateNode(context, false));
	}

	open->TargetDialogue = parent.Graph;
	{
		FDataValidationContext context;
		TestFalse(TEXT("[Self target] invalid"), open->ValidateNode(context, false));
		TestTrue(TEXT("[Self target] reports recursion"), JoinIssues(context).Contains(TEXT("recursive reference")));
	}

	// parent -> child -> parent
	open->TargetDialogue = child.Graph;
	UMounteaDialogueGraphNode_OpenChildGraph* back = child.AddNode<UMounteaDialogueGraphNode_OpenChildGraph>();
	back->TargetDialogue = parent.Graph;
	{
		FDataValidationContext context;
		TestFalse(TEXT("[Indirect cycle] invalid"), open->ValidateNode(context, false));
		TestTrue(TEXT("[Indirect cycle] reports recursion"), JoinIssues(context).Contains(TEXT("recursive reference")));
	}
	return true;
}

// Return To Node offers every node it can jump to, except node types it must not target.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueNodeReturnTargetsTest,
	"Mountea.DialogueSystem.Node.ReturnToNodeTargetList",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueNodeReturnTargetsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueTest;

	FTestGraph graph;
	graph.AddSpeech<UMounteaDialogueGraphNode_LeadNode>(TEXT("Lead"));                 // index 1 (Start is 0)
	graph.AddSpeech<UMounteaDialogueGraphNode_AnswerNode>(TEXT("Answer"));             // 2
	graph.AddSpeech<UMounteaDialogueGraphNode_CompleteNode>(TEXT("Complete"));         // 3: filtered
	graph.AddNode<UMounteaDialogueGraphNode_Delay>();                                  // 4: filtered
	UMounteaDialogueGraphNode_ReturnToNode* returnNode = graph.AddNode<UMounteaDialogueGraphNode_ReturnToNode>(); // 5: filtered

	const TArray<FString> targets = MounteaDialogueNodeTest::GetOptions(returnNode);
	TestTrue(TEXT("Only Lead and Answer are offered"), targets == TArray<FString>({ TEXT("1"), TEXT("2") }));

	UMounteaDialogueGraphNode_ReturnToNode* detached = NewObject<UMounteaDialogueGraphNode_ReturnToNode>(GetTransientPackage());
	TestEqual(TEXT("Without a graph nothing is offered"), MounteaDialogueNodeTest::GetOptions(detached).Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
