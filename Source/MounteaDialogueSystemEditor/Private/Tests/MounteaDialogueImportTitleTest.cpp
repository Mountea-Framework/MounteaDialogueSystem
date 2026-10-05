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

#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"
#include "Helpers/MounteaDialogueEngineCompat.h"
#include "Dom/JsonObject.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Tests in this file cover how the Dialoguer importer turns a node's two titles into the Nodes string
// table and into DialogueRow.RowTitle (Bugreport #3): the choice button must read the node's Selection
// Title, falling back to its display name, then to its label - the same chain Dialoguer's preview uses
// (selectionTitle || displayName || label). Default locale only.
//
// No assets are created: the tests drive the title helpers with JSON literals and an in-memory
// UStringTable, so no IAssetTools or disk access is involved.

struct FMounteaDialogueImportTestAccess
{
	static FString ResolveNodeDisplayName(const TSharedPtr<FJsonObject>& NodeData, const TMap<FString, FString>& Lookup)
	{
		return UMounteaDialogueSystemImportExportHelpers::ResolveNodeDisplayName(NodeData, Lookup);
	}

	static FString ResolveNodeSelectionTitle(const TSharedPtr<FJsonObject>& NodeData, const TMap<FString, FString>& Lookup)
	{
		return UMounteaDialogueSystemImportExportHelpers::ResolveNodeSelectionTitle(NodeData, Lookup);
	}

	static FString GetSelectionTitleEntryKey(const FString& NodeId)
	{
		return UMounteaDialogueSystemImportExportHelpers::GetSelectionTitleEntryKey(NodeId);
	}

	static void PopulateNodesStringTable(UStringTable* Table, const TArray<TSharedPtr<FJsonValue>>& Nodes, const TMap<FString, FString>& Lookup)
	{
		UMounteaDialogueSystemImportExportHelpers::PopulateNodesStringTable(Table, Nodes, Lookup);
	}

	static FString GetRowTitleEntryKey(const UStringTable* Table, const FString& NodeId)
	{
		return UMounteaDialogueSystemImportExportHelpers::GetRowTitleEntryKey(Table, NodeId);
	}

	static FText MakeRowTitle(const UStringTable* Table, const FString& NodeId)
	{
		return UMounteaDialogueSystemImportExportHelpers::MakeRowTitle(Table, NodeId);
	}

	static void BuildStringTableLookup(const TMap<FString, FString>& ExtractedFiles, FString& OutDefaultLocale, TMap<FString, FString>& OutLookup, TSharedPtr<FJsonObject>& OutEntries)
	{
		UMounteaDialogueSystemImportExportHelpers::BuildStringTableLookup(ExtractedFiles, OutDefaultLocale, OutLookup, OutEntries);
	}
};

namespace MounteaDialogueImportTitleTest
{
	static TSharedPtr<FJsonObject> ParseObject(const FString& Json)
	{
		TSharedPtr<FJsonObject> object;
		const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(Json);
		FJsonSerializer::Deserialize(reader, object);
		return object;
	}

	static TArray<TSharedPtr<FJsonValue>> ParseArray(const FString& Json)
	{
		TArray<TSharedPtr<FJsonValue>> array;
		const TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(Json);
		FJsonSerializer::Deserialize(reader, array);
		return array;
	}

	static UStringTable* NewTable()
	{
		return NewObject<UStringTable>(GetTransientPackage());
	}

	static bool TryGetEntry(const UStringTable* Table, const FString& Key, FString& OutText)
	{
		return Table->GetStringTable()->GetSourceString(FTextKey(Key), OutText);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueImportResolveSelectionTitleTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.ResolveSelectionTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueImportResolveSelectionTitleTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueImportTitleTest;

	const TMap<FString, FString> lookup = {
		{ TEXT("k.full"), TEXT("Knock again") },
		{ TEXT("k.empty"), TEXT("") }
	};

	TestEqual(TEXT("A selectionTitleKey resolves through the lookup"),
		FMounteaDialogueImportTestAccess::ResolveNodeSelectionTitle(ParseObject(TEXT(R"({"selectionTitleKey":"k.full"})")), lookup),
		FString(TEXT("Knock again")));

	TestEqual(TEXT("An empty entry falls back to an inline selectionTitle"),
		FMounteaDialogueImportTestAccess::ResolveNodeSelectionTitle(ParseObject(TEXT(R"({"selectionTitleKey":"k.empty","selectionTitle":"Legacy"})")), lookup),
		FString(TEXT("Legacy")));

	TestEqual(TEXT("An empty entry with no inline value stays empty (caller falls back to display name)"),
		FMounteaDialogueImportTestAccess::ResolveNodeSelectionTitle(ParseObject(TEXT(R"({"selectionTitleKey":"k.empty"})")), lookup),
		FString());

	TestEqual(TEXT("An unknown key stays empty"),
		FMounteaDialogueImportTestAccess::ResolveNodeSelectionTitle(ParseObject(TEXT(R"({"selectionTitleKey":"k.missing"})")), lookup),
		FString());

	TestEqual(TEXT("No key reads the inline selectionTitle"),
		FMounteaDialogueImportTestAccess::ResolveNodeSelectionTitle(ParseObject(TEXT(R"({"selectionTitle":"Inline"})")), lookup),
		FString(TEXT("Inline")));

	TestEqual(TEXT("A node with no selection title at all is empty"),
		FMounteaDialogueImportTestAccess::ResolveNodeSelectionTitle(ParseObject(TEXT(R"({"label":"New Player"})")), lookup),
		FString());

	TestEqual(TEXT("Null node data is tolerated"),
		FMounteaDialogueImportTestAccess::ResolveNodeSelectionTitle(nullptr, lookup),
		FString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueImportResolveDisplayNameTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.ResolveDisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueImportResolveDisplayNameTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueImportTitleTest;

	const TMap<FString, FString> lookup = { { TEXT("d.full"), TEXT("Player") } };

	TestEqual(TEXT("displayNameKey resolves through the lookup"),
		FMounteaDialogueImportTestAccess::ResolveNodeDisplayName(ParseObject(TEXT(R"({"displayNameKey":"d.full","label":"New Player"})")), lookup),
		FString(TEXT("Player")));

	TestEqual(TEXT("Without a usable key, UE round-trip additionalInfo.displayName is used"),
		FMounteaDialogueImportTestAccess::ResolveNodeDisplayName(ParseObject(TEXT(R"({"additionalInfo":{"displayName":"Round trip"},"label":"New Player"})")), lookup),
		FString(TEXT("Round trip")));

	TestEqual(TEXT("With neither, the node label is the last resort"),
		FMounteaDialogueImportTestAccess::ResolveNodeDisplayName(ParseObject(TEXT(R"({"displayNameKey":"d.missing","label":"New Player"})")), lookup),
		FString(TEXT("New Player")));

	TestEqual(TEXT("A node with no name at all is empty"),
		FMounteaDialogueImportTestAccess::ResolveNodeDisplayName(ParseObject(TEXT("{}")), lookup),
		FString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueImportPopulateNodesTableTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.PopulateNodesTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueImportPopulateNodesTableTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueImportTitleTest;

	const TMap<FString, FString> lookup = {
		{ TEXT("a.dn"), TEXT("Player") },
		{ TEXT("a.st"), TEXT("Knock again") },
		{ TEXT("b.dn"), TEXT("Complete") },
		{ TEXT("b.st"), TEXT("") }
	};

	const TArray<TSharedPtr<FJsonValue>> nodes = ParseArray(TEXT(R"([
		{"id":"A","data":{"label":"New Player","displayNameKey":"a.dn","selectionTitleKey":"a.st"}},
		{"id":"B","data":{"label":"New Complete","displayNameKey":"b.dn","selectionTitleKey":"b.st"}},
		{"id":"C","data":{"label":"Label C"}}
	])"));

	UStringTable* table = NewTable();
	FMounteaDialogueImportTestAccess::PopulateNodesStringTable(table, nodes, lookup);

	FString text;
	const FString selectionKeyA = FMounteaDialogueImportTestAccess::GetSelectionTitleEntryKey(TEXT("A"));
	TestTrue(TEXT("A: display name entry exists"), TryGetEntry(table, TEXT("A"), text));
	TestEqual(TEXT("A: display name text"), text, FString(TEXT("Player")));
	TestTrue(TEXT("A: selection title entry exists"), TryGetEntry(table, selectionKeyA, text));
	TestEqual(TEXT("A: selection title text"), text, FString(TEXT("Knock again")));

	TestTrue(TEXT("B: display name entry exists"), TryGetEntry(table, TEXT("B"), text));
	TestEqual(TEXT("B: display name text"), text, FString(TEXT("Complete")));
	TestFalse(TEXT("B: an empty selection title must not create an entry"),
		TryGetEntry(table, FMounteaDialogueImportTestAccess::GetSelectionTitleEntryKey(TEXT("B")), text));

	TestTrue(TEXT("C: a node with neither title falls back to its label"), TryGetEntry(table, TEXT("C"), text));
	TestEqual(TEXT("C: label text"), text, FString(TEXT("Label C")));
	TestFalse(TEXT("C: no selection title entry"),
		TryGetEntry(table, FMounteaDialogueImportTestAccess::GetSelectionTitleEntryKey(TEXT("C")), text));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueImportRowTitleTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.RowTitleChoosesSelectionTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueImportRowTitleTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueImportTitleTest;

	UStringTable* table = NewTable();
	MounteaDialogueCompat::SetStringTableSourceString(*table->GetMutableStringTable(), TEXT("WithSelection"), TEXT("Player"));
	MounteaDialogueCompat::SetStringTableSourceString(*table->GetMutableStringTable(), FMounteaDialogueImportTestAccess::GetSelectionTitleEntryKey(TEXT("WithSelection")), TEXT("Knock again"));
	MounteaDialogueCompat::SetStringTableSourceString(*table->GetMutableStringTable(), TEXT("DisplayOnly"), TEXT("Complete"));

	TestEqual(TEXT("Selection title entry wins when present"),
		FMounteaDialogueImportTestAccess::GetRowTitleEntryKey(table, TEXT("WithSelection")),
		FMounteaDialogueImportTestAccess::GetSelectionTitleEntryKey(TEXT("WithSelection")));
	TestEqual(TEXT("Without a selection title entry the bare node id (display name) is used"),
		FMounteaDialogueImportTestAccess::GetRowTitleEntryKey(table, TEXT("DisplayOnly")),
		FString(TEXT("DisplayOnly")));

	TestEqual(TEXT("RowTitle reads the selection title"),
		FMounteaDialogueImportTestAccess::MakeRowTitle(table, TEXT("WithSelection")).ToString(), FString(TEXT("Knock again")));
	TestEqual(TEXT("RowTitle falls back to the display name"),
		FMounteaDialogueImportTestAccess::MakeRowTitle(table, TEXT("DisplayOnly")).ToString(), FString(TEXT("Complete")));
	return true;
}

// Trimmed copy of the real export from the report (TCoTT_Demo / FirstMeeting): an Answer and a Complete node
// with Selection Titles, and a Start node whose selection title entry exists but is empty. Runs the same
// stringTable.json -> lookup -> Nodes table -> RowTitle path the importer uses.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueImportReportScenarioTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.ReportScenario",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueImportReportScenarioTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueImportTitleTest;

	TMap<FString, FString> extractedFiles;
	extractedFiles.Add(TEXT("stringTable.json"), TEXT(R"({
		"version": 2, "format": "stringTable.v2", "defaultLocale": "en", "locales": ["en"],
		"entries": {
			"dlg.firstmeeting.n_complete_49d7.display_name":   { "en": "Complete" },
			"dlg.firstmeeting.n_complete_49d7.selection_title": { "en": "What is this?" },
			"dlg.firstmeeting.n_player_e3db.display_name":     { "en": "Player" },
			"dlg.firstmeeting.n_player_e3db.selection_title":  { "en": "Knock again" },
			"dlg.firstmeeting.n_start_552c.display_name":      { "en": "Start" },
			"dlg.firstmeeting.n_start_552c.selection_title":   { "en": "" }
		}
	})"));

	FString defaultLocale;
	TMap<FString, FString> lookup;
	TSharedPtr<FJsonObject> entries;
	FMounteaDialogueImportTestAccess::BuildStringTableLookup(extractedFiles, defaultLocale, lookup, entries);
	TestEqual(TEXT("Default locale is read from the export"), defaultLocale, FString(TEXT("en")));

	const TArray<TSharedPtr<FJsonValue>> nodes = ParseArray(TEXT(R"([
		{"id":"00000000-0000-0000-0000-000000000001","type":"startNode","data":{"label":"Dialogue entry point",
			"displayNameKey":"dlg.firstmeeting.n_start_552c.display_name","selectionTitleKey":"dlg.firstmeeting.n_start_552c.selection_title"}},
		{"id":"29456d61-96cd-4e69-8937-1dbee4df554b","type":"answerNode","data":{"label":"New Player",
			"displayNameKey":"dlg.firstmeeting.n_player_e3db.display_name","selectionTitleKey":"dlg.firstmeeting.n_player_e3db.selection_title"}},
		{"id":"45e4a48d-8a52-497f-9f33-bccaffd72994","type":"completeNode","data":{"label":"New Complete",
			"displayNameKey":"dlg.firstmeeting.n_complete_49d7.display_name","selectionTitleKey":"dlg.firstmeeting.n_complete_49d7.selection_title"}}
	])"));

	UStringTable* table = NewTable();
	FMounteaDialogueImportTestAccess::PopulateNodesStringTable(table, nodes, lookup);

	TestEqual(TEXT("Answer choice reads its Selection Title, not 'Player'"),
		FMounteaDialogueImportTestAccess::MakeRowTitle(table, TEXT("29456d61-96cd-4e69-8937-1dbee4df554b")).ToString(), FString(TEXT("Knock again")));
	TestEqual(TEXT("Complete choice reads its Selection Title, not 'Complete'"),
		FMounteaDialogueImportTestAccess::MakeRowTitle(table, TEXT("45e4a48d-8a52-497f-9f33-bccaffd72994")).ToString(), FString(TEXT("What is this?")));
	TestEqual(TEXT("A node with an empty Selection Title falls back to its display name"),
		FMounteaDialogueImportTestAccess::MakeRowTitle(table, TEXT("00000000-0000-0000-0000-000000000001")).ToString(), FString(TEXT("Start")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
