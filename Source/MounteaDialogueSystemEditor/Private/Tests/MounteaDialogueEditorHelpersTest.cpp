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

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/MounteaDialogueEditorTestFixtures.h"
#include "Helpers/MounteaDialogueHtmlHelpers.h"
#include "Helpers/MounteaDialogueMarkdownHelpers.h"
#include "Helpers/MounteaDialogueNodeSpacingUtils.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaEditorHtmlFileUrlTest,
  "Mountea.DialogueSystemEditor.Helpers.HtmlFileUrl",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaEditorHtmlFileUrlTest::RunTest(const FString& Parameters)
{
  TestEqual(TEXT("Windows drive paths retain their document name"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("C:\\My Project\\Help\\page.html")),
    FString(TEXT("file:///C:/My%20Project/Help/page.html")));
  TestEqual(TEXT("Mac paths have three slashes before the root directory"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("/Users/Alice/My Project/Help/page.html")),
    FString(TEXT("file:///Users/Alice/My%20Project/Help/page.html")));
  TestEqual(TEXT("Linux paths preserve filename case and unreserved characters"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("/home/alice/Help/Page-1_~.html")),
    FString(TEXT("file:///home/alice/Help/Page-1_~.html")));
  TestEqual(TEXT("Network paths use the server as their authority"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("\\\\server\\My Share\\Help\\page.html")),
    FString(TEXT("file://server/My%20Share/Help/page.html")));
  TestEqual(TEXT("URL delimiters and literal percent sequences remain part of filenames"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("/Help/100%20 #1?.html")),
    FString(TEXT("file:///Help/100%2520%20%231%3F.html")));
  TestEqual(TEXT("A colon within a Unix filename is encoded"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("/Help/chapter:1.html")),
    FString(TEXT("file:///Help/chapter%3A1.html")));
  TestEqual(TEXT("Unicode paths are encoded as UTF-8 including supplementary characters"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("/Users/Řeč/💬/page.html")),
    FString(TEXT("file:///Users/%C5%98e%C4%8D/%F0%9F%92%AC/page.html")));
  TestEqual(TEXT("Relative path segments resolve to the same document"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(TEXT("/Help/chapters/../page.html")),
    FString(TEXT("file:///Help/page.html")));

  const FString relativePath = TEXT("Mountea Help/page.html");
  const FString absolutePath = FPaths::ConvertRelativePathToFull(relativePath);
  TestEqual(TEXT("Relative and absolute source paths resolve to the same URL"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(relativePath),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(absolutePath));

  const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
  if(!TestTrue(TEXT("The dialogue plugin is available"), plugin.IsValid()))
    return false;

  const FString generatedPath = FPaths::Combine(plugin->GetBaseDir(), TEXT("Resources/Help/generated.html"));
  TestEqual(TEXT("Generated HTML resolves assets from the plugin help directory"),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(FString()),
    FMounteaDialogueHtmlHelpers::BuildBaseUrl(generatedPath));
  return true;
}

// Small editor helpers: Markdown -> HTML for the help/changelog panels, and node spacing on the canvas.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaEditorMarkdownTest,
	"Mountea.DialogueSystemEditor.Helpers.MarkdownToHtml",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaEditorMarkdownTest::RunTest(const FString& Parameters)
{
	FString html;
	TestFalse(TEXT("Empty markdown converts to nothing"), FMounteaDialogueMarkdownHelpers::ConvertMarkdownToHtml(FString(), html));
	TestTrue(TEXT("and leaves no output"), html.IsEmpty());

	TestTrue(TEXT("A heading converts"), FMounteaDialogueMarkdownHelpers::ConvertMarkdownToHtml(TEXT("# Title"), html));
	TestTrue(TEXT("as <h1>"), html.Contains(TEXT("<h1>Title</h1>")));

	FMounteaDialogueMarkdownHelpers::ConvertMarkdownToHtml(TEXT("Some **bold** and ~~gone~~ text."), html);
	TestTrue(TEXT("Bold"), html.Contains(TEXT("<strong>bold</strong>")));
	TestTrue(TEXT("Strikethrough (enabled flag)"), html.Contains(TEXT("<del>gone</del>")));

	FMounteaDialogueMarkdownHelpers::ConvertMarkdownToHtml(TEXT("| A | B |\n|---|---|\n| 1 | 2 |\n"), html);
	TestTrue(TEXT("Tables (enabled flag)"), html.Contains(TEXT("<table>")));

	FMounteaDialogueMarkdownHelpers::ConvertMarkdownToHtml(TEXT("Řeč – ünïcode"), html);
	TestTrue(TEXT("Non-ASCII text survives the UTF-8 round trip"), html.Contains(TEXT("Řeč – ünïcode")));
	return true;
}

// A child must sit at least the minimum gap below the lowest of its parents (feedback parents do not count).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaEditorSpacingTest,
	"Mountea.DialogueSystemEditor.Helpers.NodeSpacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaEditorSpacingTest::RunTest(const FString& Parameters)
{
	using namespace MounteaEditorTest;
	namespace Spacing = MounteaDialogueNodeSpacingUtils;

	FEditorTestGraph graph(TEXT("SpacingGraph"));
	UMounteaDialogueGraphNode_LeadNode* lead = graph.AddNode<UMounteaDialogueGraphNode_LeadNode>();
	UMounteaDialogueGraphNode_AnswerNode* answer = graph.AddNode<UMounteaDialogueGraphNode_AnswerNode>();
	graph.Link(graph.Start(), lead);
	graph.Link(lead, answer);
	graph.MakeEdGraph();

	UEdNode_MounteaDialogueGraphNode* edStart = graph.EdNodeOf(graph.Start());
	UEdNode_MounteaDialogueGraphNode* edLead = graph.EdNodeOf(lead);
	UEdNode_MounteaDialogueGraphNode* edAnswer = graph.EdNodeOf(answer);
	edStart->NodePosY = 0;
	edLead->NodePosY = 10;    // overlaps its parent
	edAnswer->NodePosY = 5000; // already far below

	float required = 0.f;
	TestFalse(TEXT("A node without parents has no requirement"), Spacing::ComputeRequiredChildTopY(edStart, required));
	TestTrue(TEXT("A child has a requirement"), Spacing::ComputeRequiredChildTopY(edLead, required));
	TestEqual(TEXT("Required top = parent bottom + gap"), required, Spacing::GetNodeHeight(edStart) + Spacing::GetMinParentChildGap());
	TestTrue(TEXT("The gap is positive"), Spacing::GetMinParentChildGap() > 0.f);
	TestTrue(TEXT("Nodes without widgets have a fallback height"), Spacing::GetNodeHeight(nullptr) > 0.f);

	TestTrue(TEXT("A child already far enough below needs nothing more"), Spacing::ComputeRequiredChildTopY(edAnswer, required) && required < 5000.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
