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

#include "Archive/MounteaDialogueArchive.h"
#include "Helpers/MounteaDialogueEngineCompat.h"
#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "UObject/Package.h"

// Node titles on import: the Nodes string table holds the archive's displayName / selectionTitle texts under the
// keys the archive names, and a node's RowTitle (the choice button text) prefers the Selection Title.

struct FMounteaDialogueImportTestAccess
{
	static FString GetRowTitleEntryKey(const UStringTable* Table, const MounteaArchive::FNode& Node)
	{
		return UMounteaDialogueSystemImportExportHelpers::GetRowTitleEntryKey(Table, Node);
	}

	static FText MakeRowTitle(const UStringTable* Table, const MounteaArchive::FNode& Node)
	{
		return UMounteaDialogueSystemImportExportHelpers::MakeRowTitle(Table, Node);
	}

	static TMap<FString, FString> BuildParticipantTags(const MounteaArchive::FDialogueArchive& Archive)
	{
		return UMounteaDialogueSystemImportExportHelpers::BuildParticipantTags(Archive);
	}
};

namespace MounteaImportTitleTest
{
	UStringTable* MakeTable(const TMap<FString, FString>& Entries)
	{
		UStringTable* table = NewObject<UStringTable>(GetTransientPackage());
		table->GetMutableStringTable()->SetNamespace(TEXT("ST_Test_Nodes"));
		for (const TPair<FString, FString>& pair : Entries)
			MounteaDialogueCompat::SetStringTableSourceString(*table->GetMutableStringTable(), pair.Key, pair.Value);
		return table;
	}

	MounteaArchive::FNode MakeNode(const TCHAR* DisplayNameKey, const TCHAR* SelectionTitleKey)
	{
		MounteaArchive::FNode node;
		node.Id = TEXT("node-1");
		node.DisplayNameKey = DisplayNameKey;
		node.SelectionTitleKey = SelectionTitleKey;
		return node;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaImportRowTitlePrefersSelectionTitleTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.PrefersSelectionTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaImportRowTitlePrefersSelectionTitleTest::RunTest(const FString& Parameters)
{
	UStringTable* table = MounteaImportTitleTest::MakeTable({
		{ TEXT("dlg.a.n_x.display_name"), TEXT("Greeting") },
		{ TEXT("dlg.a.n_x.selection_title"), TEXT("Hello there") } });
	const MounteaArchive::FNode node = MounteaImportTitleTest::MakeNode(TEXT("dlg.a.n_x.display_name"), TEXT("dlg.a.n_x.selection_title"));

	TestEqual(TEXT("selection title key wins"), FMounteaDialogueImportTestAccess::GetRowTitleEntryKey(table, node), FString(TEXT("dlg.a.n_x.selection_title")));
	TestEqual(TEXT("title text comes from the table"), FMounteaDialogueImportTestAccess::MakeRowTitle(table, node).ToString(), FString(TEXT("Hello there")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaImportRowTitleFallsBackTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.FallsBackToDisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaImportRowTitleFallsBackTest::RunTest(const FString& Parameters)
{
	// Selection title entry exists but is empty (Dialoguer writes one for every node that supports it).
	UStringTable* table = MounteaImportTitleTest::MakeTable({
		{ TEXT("dlg.a.n_x.display_name"), TEXT("Greeting") },
		{ TEXT("dlg.a.n_x.selection_title"), TEXT("") } });
	const MounteaArchive::FNode node = MounteaImportTitleTest::MakeNode(TEXT("dlg.a.n_x.display_name"), TEXT("dlg.a.n_x.selection_title"));

	TestEqual(TEXT("display name key used"), FMounteaDialogueImportTestAccess::GetRowTitleEntryKey(table, node), FString(TEXT("dlg.a.n_x.display_name")));
	TestEqual(TEXT("display name text"), FMounteaDialogueImportTestAccess::MakeRowTitle(table, node).ToString(), FString(TEXT("Greeting")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaImportRowTitleLabelFallbackTest,
	"Mountea.DialogueSystemEditor.Import.NodeTitles.NodeWithoutKeysUsesLabel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaImportRowTitleLabelFallbackTest::RunTest(const FString& Parameters)
{
	UStringTable* table = MounteaImportTitleTest::MakeTable({});
	MounteaArchive::FNode node = MounteaImportTitleTest::MakeNode(TEXT(""), TEXT(""));
	node.Label = TEXT("Dialogue entry point");

	TestTrue(TEXT("no table key"), FMounteaDialogueImportTestAccess::GetRowTitleEntryKey(table, node).IsEmpty());
	TestEqual(TEXT("label text"), FMounteaDialogueImportTestAccess::MakeRowTitle(table, node).ToString(), FString(TEXT("Dialogue entry point")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaImportParticipantTagsTest,
	"Mountea.DialogueSystemEditor.Import.ParticipantTags.UseCategoryChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaImportParticipantTagsTest::RunTest(const FString& Parameters)
{
	MounteaArchive::FDialogueArchive archive;
	archive.Categories.Add({ TEXT("c-npc"), TEXT("NPC"), FString() });
	archive.Categories.Add({ TEXT("c-merchant"), TEXT("Merchant"), TEXT("c-npc") });
	archive.Categories.Add({ TEXT("c-player"), TEXT("Player"), FString() });

	MounteaArchive::FParticipant waldermar;
	waldermar.Id = TEXT("p-1");
	waldermar.Name = TEXT("Waldermar");
	waldermar.CategoryId = TEXT("c-merchant");
	archive.Participants.Add(waldermar);

	MounteaArchive::FParticipant player;
	player.Id = TEXT("p-2");
	player.Name = TEXT("Player");
	player.CategoryId = TEXT("c-player");
	archive.Participants.Add(player);

	const TMap<FString, FString> tags = FMounteaDialogueImportTestAccess::BuildParticipantTags(archive);
	TestEqual(TEXT("nested category"), tags.FindRef(TEXT("p-1")), FString(TEXT("Mountea_Dialogue.NPC.Merchant.Waldermar")));
	TestEqual(TEXT("root category"), tags.FindRef(TEXT("p-2")), FString(TEXT("Mountea_Dialogue.Player.Player")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
