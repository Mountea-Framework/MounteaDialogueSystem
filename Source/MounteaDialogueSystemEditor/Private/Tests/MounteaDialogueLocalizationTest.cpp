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
#include "HAL/FileManager.h"
#include "Internationalization/TextLocalizationResource.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Translations leave Unreal as .po (translator / Localization Dashboard interchange, Unreal identity "Namespace,Key")
// and as a ready .locres the game loads without a Dashboard compile.

struct FMounteaDialogueLocalizationTestAccess
{
	static void WriteResources(const FString& TableId, const FString& DefaultLocale, const TMap<FString, TMap<FString, FString>>& Texts)
	{
		UMounteaDialogueSystemImportExportHelpers::WriteLocalizationResources(TableId, DefaultLocale, Texts);
	}

	static void WritePo(const FString& TableId, const FString& DefaultLocale, const TMap<FString, TMap<FString, FString>>& Texts)
	{
		UMounteaDialogueSystemImportExportHelpers::WriteLocalizationPoFiles(TableId, DefaultLocale, Texts);
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaLocalizationWritesLocresTest,
	"Mountea.DialogueSystemEditor.Localization.WritesLocresAndPo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaLocalizationWritesLocresTest::RunTest(const FString& Parameters)
{
	const FString tableId = TEXT("ST_MounteaLocTest_Rows");
	const FString localizationRoot = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Localization"));
	const FString targetRoot = FPaths::Combine(localizationRoot, TEXT("MounteaDialogue"));
	const bool bTargetExisted = IFileManager::Get().DirectoryExists(*targetRoot);

	TMap<FString, TMap<FString, FString>> texts;
	texts.Add(TEXT("dlg.a.n_x.r_one.text"), { { TEXT("en"), TEXT("Hello") }, { TEXT("cs"), TEXT("Ahoj") } });
	texts.Add(TEXT("key,with,comma"), { { TEXT("en"), TEXT("A, B") }, { TEXT("cs"), TEXT("A, B cs") } });
	texts.Add(TEXT("untranslated"), { { TEXT("en"), TEXT("Only English") } });

	FMounteaDialogueLocalizationTestAccess::WriteResources(tableId, TEXT("en"), texts);
	FMounteaDialogueLocalizationTestAccess::WritePo(tableId, TEXT("en"), texts);

	// .locres: translated entry carries the translation and the hash of the source text, so it applies at runtime.
	FTextLocalizationResource czech;
	if (TestTrue(TEXT("cs locres exists and loads"), czech.LoadFromFile(FPaths::Combine(targetRoot, TEXT("cs"), TEXT("MounteaDialogue.locres")), 0)))
	{
		const FTextLocalizationResource::FEntry* entry = czech.Entries.Find(FTextId(FTextKey(tableId), FTextKey(TEXT("dlg.a.n_x.r_one.text"))));
		if (TestNotNull(TEXT("translated entry present"), entry))
		{
			TestEqual(TEXT("translation"), entry->LocalizedString.IsValid() ? *entry->LocalizedString : FString(), FString(TEXT("Ahoj")));
			TestEqual(TEXT("source hash"), entry->SourceStringHash, FTextLocalizationResource::HashString(TEXT("Hello")));
		}
		TestNotNull(TEXT("key with commas survives"), czech.Entries.Find(FTextId(FTextKey(tableId), FTextKey(TEXT("key,with,comma")))));
		TestNull(TEXT("untranslated text is left to the source"), czech.Entries.Find(FTextId(FTextKey(tableId), FTextKey(TEXT("untranslated")))));
	}

	FTextLocalizationResource english;
	if (TestTrue(TEXT("native locres exists and loads"), english.LoadFromFile(FPaths::Combine(targetRoot, TEXT("en"), TEXT("MounteaDialogue.locres")), 0)))
		TestNotNull(TEXT("native entry present"), english.Entries.Find(FTextId(FTextKey(tableId), FTextKey(TEXT("untranslated")))));

	FTextLocalizationMetaDataResource metaData;
	if (TestTrue(TEXT("locmeta loads"), metaData.LoadFromFile(FPaths::Combine(targetRoot, TEXT("MounteaDialogue.locmeta")))))
	{
		TestEqual(TEXT("native culture"), metaData.NativeCulture, FString(TEXT("en")));
		TestTrue(TEXT("compiled cultures listed"), metaData.CompiledCultures.Contains(TEXT("cs")) && metaData.CompiledCultures.Contains(TEXT("en")));
	}

	// Writing again replaces this table's entries and leaves other tables alone.
	TMap<FString, TMap<FString, FString>> other;
	other.Add(TEXT("other.key"), { { TEXT("en"), TEXT("Other") }, { TEXT("cs"), TEXT("Jine") } });
	FMounteaDialogueLocalizationTestAccess::WriteResources(TEXT("ST_MounteaLocTest_Other"), TEXT("en"), other);
	texts.Remove(TEXT("untranslated"));
	FMounteaDialogueLocalizationTestAccess::WriteResources(tableId, TEXT("en"), texts);

	FTextLocalizationResource merged;
	merged.LoadFromFile(FPaths::Combine(targetRoot, TEXT("cs"), TEXT("MounteaDialogue.locres")), 0);
	TestNotNull(TEXT("other table kept"), merged.Entries.Find(FTextId(FTextKey(TEXT("ST_MounteaLocTest_Other")), FTextKey(TEXT("other.key")))));
	TestNotNull(TEXT("this table rewritten"), merged.Entries.Find(FTextId(FTextKey(tableId), FTextKey(TEXT("dlg.a.n_x.r_one.text")))));

	// .po: Unreal identity "Namespace,Key" (comma-escaped), not the old "Namespace|Key".
	FString po;
	if (TestTrue(TEXT("po exists"), FFileHelper::LoadFileToString(po, *FPaths::Combine(localizationRoot, tableId, TEXT("cs"), tableId + TEXT(".po")))))
	{
		TestTrue(TEXT("po identity uses a comma"), po.Contains(TEXT("msgctxt \"ST_MounteaLocTest_Rows,dlg.a.n_x.r_one.text\"")));
		TestTrue(TEXT("po escapes commas inside the key"), po.Contains(TEXT("ST_MounteaLocTest_Rows,key\\\\,with\\\\,comma")));
		TestFalse(TEXT("po no longer uses the pipe"), po.Contains(TEXT("|")));
	}

	// Cleanup
	IFileManager::Get().DeleteDirectory(*FPaths::Combine(localizationRoot, tableId), false, true);
	if (!bTargetExisted)
		IFileManager::Get().DeleteDirectory(*targetRoot, false, true);
	else
	{
		// Shared folder existed: strip only the entries this test added.
		TArray<FString> cultures;
		IFileManager::Get().FindFiles(cultures, *(targetRoot / TEXT("*")), false, true);
		for (const FString& culture : cultures)
		{
			const FString path = FPaths::Combine(targetRoot, culture, TEXT("MounteaDialogue.locres"));
			FTextLocalizationResource resource;
			if (!resource.LoadFromFile(path, 0))
				continue;
			for (auto it = resource.Entries.CreateIterator(); it; ++it)
			{
				const FString ns = it.Key().GetNamespace().ToString();
				if (ns.StartsWith(TEXT("ST_MounteaLocTest_")))
					it.RemoveCurrent();
			}
			resource.SaveToFile(path);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
