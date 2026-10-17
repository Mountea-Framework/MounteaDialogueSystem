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
#include "ImportConfig/MounteaDialogueImportConfig.h"

// The import history decides whether a dropped file is a reimport and where its assets live. It is persisted
// to Config/Import/MounteaDialogueImportConfig.json; these tests use their own instances and restore that file.

namespace MounteaImportConfigTest
{
	// New instances copy the CDO, which already holds the project's real history; start each test empty.
	static UMounteaDialogueImportConfig* NewEmptyConfig()
	{
		UMounteaDialogueImportConfig* config = NewObject<UMounteaDialogueImportConfig>(GetTransientPackage());
		config->DialogueHistory.Reset();
		config->ProjectHistory.Reset();
		return config;
	}
}

// Records are found by GUID; unknown GUIDs and empty paths are "not imported".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaImportConfigRecordsTest,
	"Mountea.DialogueSystemEditor.ImportConfig.RecordAndLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaImportConfigRecordsTest::RunTest(const FString& Parameters)
{
	UMounteaDialogueImportConfig* config = MounteaImportConfigTest::NewEmptyConfig();
	const FGuid dialogue = FGuid::NewGuid();

	FString assetPath;
	TestFalse(TEXT("Unknown dialogue is not imported"), config->IsDialogueImported(dialogue));
	TestFalse(TEXT("Unknown dialogue has no asset path"), config->LookupDialogueAssetPath(dialogue, assetPath));

	config->RecordDialogueImport(dialogue, TEXT("/Game/Dialogues/Greeting.Greeting"), TEXT("C:/In/Greeting.mnteadlg"));
	TestTrue(TEXT("Recorded dialogue is imported"), config->IsDialogueImported(dialogue));
	TestTrue(TEXT("Its asset path is found"), config->LookupDialogueAssetPath(dialogue, assetPath));
	TestEqual(TEXT("The asset path is the recorded one"), assetPath, FString(TEXT("/Game/Dialogues/Greeting.Greeting")));

	config->RecordDialogueImport(dialogue, TEXT("/Game/Moved/Greeting.Greeting"), TEXT("C:/In/Greeting.mnteadlg"));
	config->LookupDialogueAssetPath(dialogue, assetPath);
	TestEqual(TEXT("Recording again updates the path"), assetPath, FString(TEXT("/Game/Moved/Greeting.Greeting")));
	TestEqual(TEXT("and keeps one record"), config->DialogueHistory.Num(), 1);

	const FGuid empty = FGuid::NewGuid();
	config->RecordDialogueImport(empty, FString(), TEXT("C:/In/X.mnteadlg"));
	TestFalse(TEXT("A record without an asset path is not a usable lookup"), config->LookupDialogueAssetPath(empty, assetPath));

	const FGuid project = FGuid::NewGuid();
	TestFalse(TEXT("Unknown project is not imported"), config->IsProjectImported(project));
	config->RecordProjectImport(project, TEXT("Golden"), TEXT("C:/In/Golden.mnteadlgproj"), TEXT("/Game/Golden"), { dialogue });
	TestTrue(TEXT("Recorded project is imported"), config->IsProjectImported(project));
	TestEqual(TEXT("Project keeps its dialogue list"), config->ProjectHistory[project].DialogueGuids.Num(), 1);
	return true;
}

// Save then load gives back the same history, so reimport works across editor sessions.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaImportConfigPersistenceTest,
	"Mountea.DialogueSystemEditor.ImportConfig.SaveAndLoadRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaImportConfigPersistenceTest::RunTest(const FString& Parameters)
{
	MounteaEditorTest::FScopedFileBackup backup(UMounteaDialogueImportConfig::GetImportConfigFilePath());

	UMounteaDialogueImportConfig* saved = MounteaImportConfigTest::NewEmptyConfig();
	const FGuid dialogue = FGuid::NewGuid();
	const FGuid project = FGuid::NewGuid();
	saved->RecordDialogueImport(dialogue, TEXT("/Game/D/Greeting.Greeting"), TEXT("C:/In/Greeting.mnteadlg"), true);
	saved->RecordProjectImport(project, TEXT("Golden"), TEXT("C:/In/Golden.mnteadlgproj"), TEXT("/Game/Golden"), { dialogue, FGuid::NewGuid() });
	saved->SaveToFile();
	TestTrue(TEXT("The history file is written"), FPaths::FileExists(UMounteaDialogueImportConfig::GetImportConfigFilePath()));

	UMounteaDialogueImportConfig* loaded = MounteaImportConfigTest::NewEmptyConfig();
	loaded->DialogueHistory.Add(FGuid::NewGuid(), FDialogueImportRecord());
	loaded->LoadFromFile();

	TestEqual(TEXT("Loading replaces in-memory dialogue history"), loaded->DialogueHistory.Num(), 1);
	if (const FDialogueImportRecord* record = loaded->DialogueHistory.Find(dialogue); TestNotNull(TEXT("The dialogue record survives"), record))
	{
		TestEqual(TEXT("Asset path"), record->AssetPath, FString(TEXT("/Game/D/Greeting.Greeting")));
		TestEqual(TEXT("Source path"), record->SourcePath, FString(TEXT("C:/In/Greeting.mnteadlg")));
		TestTrue(TEXT("Project-import flag"), record->bIsProjectImport);
	}
	if (const FProjectImportRecord* record = loaded->ProjectHistory.Find(project); TestNotNull(TEXT("The project record survives"), record))
	{
		TestEqual(TEXT("Project name"), record->ProjectName, FString(TEXT("Golden")));
		TestEqual(TEXT("Project folder"), record->ProjectFolder, FString(TEXT("/Game/Golden")));
		TestEqual(TEXT("Dialogue GUIDs"), record->DialogueGuids.Num(), 2);
	}
	return true;
}

// A missing or broken history file must leave the in-memory history alone instead of wiping it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaImportConfigBadFileTest,
	"Mountea.DialogueSystemEditor.ImportConfig.MissingOrBrokenFileKeepsHistory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaImportConfigBadFileTest::RunTest(const FString& Parameters)
{
	const FString path = UMounteaDialogueImportConfig::GetImportConfigFilePath();
	MounteaEditorTest::FScopedFileBackup backup(path);

	UMounteaDialogueImportConfig* config = MounteaImportConfigTest::NewEmptyConfig();
	config->RecordDialogueImport(FGuid::NewGuid(), TEXT("/Game/A.A"), TEXT("C:/A.mnteadlg"));

	IFileManager::Get().Delete(*path);
	config->LoadFromFile();
	TestEqual(TEXT("Missing file: history kept"), config->DialogueHistory.Num(), 1);

	FFileHelper::SaveStringToFile(TEXT("{ this is not json"), *path);
	config->LoadFromFile();
	TestEqual(TEXT("Broken file: history kept"), config->DialogueHistory.Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
