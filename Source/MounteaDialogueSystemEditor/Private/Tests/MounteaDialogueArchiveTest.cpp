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
#include "Archive/MounteaDialogueArchiveZip.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Tests for the v3 archive contract layer. The golden files in Tests/Fixtures were produced by the real
// Mountea Dialoguer exporter (exportProjectArchive), so reading them proves compatibility with Dialoguer
// rather than with a hand-written approximation of it.

namespace MounteaArchiveTest
{
	bool LoadFixture(const TCHAR* Name, TArray<uint8>& OutBytes)
	{
		const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
		if (!plugin.IsValid())
			return false;
		const FString path = FPaths::Combine(plugin->GetBaseDir(), TEXT("Source/MounteaDialogueSystemEditor/Private/Tests/Fixtures"), Name);
		return FFileHelper::LoadFileToArray(OutBytes, *path);
	}

	bool ReadFixture(FAutomationTestBase& Test, const TCHAR* Name, MounteaArchive::FDialogueArchive& OutArchive)
	{
		TArray<uint8> bytes;
		if (!LoadFixture(Name, bytes))
		{
			Test.AddError(FString::Printf(TEXT("Fixture %s could not be loaded"), Name));
			return false;
		}
		TArray<FString> errors;
		if (!MounteaArchive::ReadArchiveFromBytes(bytes, OutArchive, errors))
		{
			for (const FString& error : errors)
				Test.AddError(error);
			return false;
		}
		return true;
	}

	bool LoadEntries(FAutomationTestBase& Test, const TCHAR* Name, MounteaArchive::FEntries& OutEntries)
	{
		TArray<uint8> bytes;
		TArray<FString> errors;
		if (!LoadFixture(Name, bytes) || !MounteaArchive::UnzipEntries(bytes, OutEntries, errors))
		{
			Test.AddError(FString::Printf(TEXT("Fixture %s could not be unzipped"), Name));
			return false;
		}
		return true;
	}

	bool HasError(const TArray<FString>& Errors, const TCHAR* Code)
	{
		return Errors.ContainsByPredicate([Code](const FString& Error) { return Error.StartsWith(Code); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveReadDialogueTest,
	"Mountea.DialogueSystemEditor.Archive.Read.DialoguerDialogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveReadDialogueTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	FDialogueArchive archive;
	if (!MounteaArchiveTest::ReadFixture(*this, TEXT("GoldenGreeting.mnteadlg"), archive))
		return false;

	TestTrue(TEXT("kind is dialogue"), archive.Kind == EKind::Dialogue);
	TestEqual(TEXT("root dialogue"), archive.RootDialogueId, FString(TEXT("00000000-0000-4000-8000-000000000010")));
	TestEqual(TEXT("dialogues (root + child)"), archive.Dialogues.Num(), 2);
	TestEqual(TEXT("categories"), archive.Categories.Num(), 3);
	TestEqual(TEXT("participants"), archive.Participants.Num(), 2);
	TestEqual(TEXT("decorator definitions"), archive.Decorators.Num(), 1);
	TestEqual(TEXT("condition definitions"), archive.Conditions.Num(), 1);
	TestEqual(TEXT("nodes"), archive.Nodes.Num(), 12);
	TestEqual(TEXT("edges"), archive.Edges.Num(), 10);
	TestEqual(TEXT("default locale"), archive.Project.Localization.DefaultLocale, FString(TEXT("en")));
	TestTrue(TEXT("cs is a supported locale"), archive.Project.Localization.SupportedLocales.Contains(TEXT("cs")));

	const FParticipant* waldermar = archive.FindParticipant(TEXT("00000000-0000-4000-8000-000000000030"));
	if (TestNotNull(TEXT("Waldermar exists"), waldermar))
	{
		TestEqual(TEXT("participant name"), waldermar->Name, FString(TEXT("Waldermar")));
		TestTrue(TEXT("thumbnail bytes are loaded from media"), waldermar->bHasThumbnail && waldermar->Thumbnail.IsSet());
		const FCategory* category = archive.FindCategory(waldermar->CategoryId);
		if (TestNotNull(TEXT("category exists"), category))
			TestEqual(TEXT("category chain parent"), category->ParentId, FString(TEXT("00000000-0000-4000-8000-000000000020")));
	}

	const FNode* lead = archive.FindNode(TEXT("00000000-0000-4000-8000-000000000010"), TEXT("00000000-0000-4000-8000-000000000101"));
	if (TestNotNull(TEXT("lead node exists"), lead) && TestEqual(TEXT("lead row count"), lead->Rows.Num(), 3))
	{
		TestTrue(TEXT("row 1 has audio bytes"), lead->Rows[0].bHasAudio && lead->Rows[0].Audio.Bytes.Num() == 844);
		TestTrue(TEXT("row 1 defaults"), lead->Rows[0].ExecutionBehaviour == ERowExecutionMode::EREM_Automatic && lead->Rows[0].DurationMode == ERowDurationMode::ERDM_Duration);
		TestTrue(TEXT("row 2 awaitInput"), lead->Rows[1].ExecutionBehaviour == ERowExecutionMode::EREM_AwaitInput);
		TestTrue(TEXT("row 2 add mode"), lead->Rows[1].DurationMode == ERowDurationMode::EDRM_Add);
		TestEqual(TEXT("row 2 override"), lead->Rows[1].DurationOverride, 2.f);
		TestTrue(TEXT("row 3 stopping"), lead->Rows[2].ExecutionBehaviour == ERowExecutionMode::EREM_Stopping);
		TestTrue(TEXT("row 3 calculate"), lead->Rows[2].DurationMode == ERowDurationMode::ERDM_AutoCalculate);
		TestEqual(TEXT("decorator instance count"), lead->Decorators.Num(), 1);
		if (lead->Decorators.Num() == 1 && lead->Decorators[0].Values.IsValid())
		{
			double intensity = 0.0;
			bool bLoud = true;
			TestTrue(TEXT("decorator number value stays a number"), lead->Decorators[0].Values->TryGetNumberField(TEXT("intensity"), intensity) && FMath::IsNearlyEqual(intensity, 0.5));
			TestTrue(TEXT("decorator boolean value stays a boolean"), lead->Decorators[0].Values->TryGetBoolField(TEXT("loud"), bLoud) && !bLoud);
		}
	}

	// Edge array order is the sibling execution order; the conditional edge is first among the Lead's outgoing edges.
	TArray<const FArchiveEdge*> leadEdges;
	for (const FArchiveEdge& edge : archive.Edges)
	{
		if (edge.SourceId == TEXT("00000000-0000-4000-8000-000000000101"))
			leadEdges.Add(&edge);
	}
	if (TestEqual(TEXT("lead outgoing edges"), leadEdges.Num(), 3))
	{
		TestEqual(TEXT("first edge keeps its condition rule"), leadEdges[0]->Rules.Num(), 1);
		TestEqual(TEXT("condition mode"), leadEdges[0]->ConditionMode, FString(TEXT("all")));
		TestEqual(TEXT("second edge has no rules"), leadEdges[1]->Rules.Num(), 0);
	}

	const FNode* pause = archive.FindNode(TEXT("00000000-0000-4000-8000-000000000010"), TEXT("00000000-0000-4000-8000-000000000105"));
	if (TestNotNull(TEXT("delay node exists"), pause))
		TestTrue(TEXT("delay duration"), pause->bHasDuration && FMath::IsNearlyEqual(pause->Duration, 1.5f));

	const FLocalizedString* rowText = archive.FindString(lead ? lead->Rows[2].TextKey : FString());
	if (TestNotNull(TEXT("row text entry exists"), rowText))
	{
		TestEqual(TEXT("default locale text"), rowText->Values.FindRef(TEXT("en")), FString(TEXT("Browse my wares.")));
		TestEqual(TEXT("translation"), rowText->Values.FindRef(TEXT("cs")), FString(TEXT("CS Browse my wares.")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveReadProjectTest,
	"Mountea.DialogueSystemEditor.Archive.Read.DialoguerProject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveReadProjectTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	FDialogueArchive archive;
	if (!MounteaArchiveTest::ReadFixture(*this, TEXT("GoldenProject.mnteadlgproj"), archive))
		return false;

	TestTrue(TEXT("kind is project"), archive.Kind == EKind::Project);
	TestEqual(TEXT("all dialogues"), archive.Dialogues.Num(), 3);
	TestEqual(TEXT("project name"), archive.Project.Name, FString(TEXT("GoldenProject")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveRoundTripTest,
	"Mountea.DialogueSystemEditor.Archive.RoundTrip.WriteThenRead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	FDialogueArchive original;
	if (!MounteaArchiveTest::ReadFixture(*this, TEXT("GoldenGreeting.mnteadlg"), original))
		return false;

	TArray<uint8> zipped;
	TArray<FString> errors;
	if (!TestTrue(TEXT("model writes to a zip"), WriteArchiveToBytes(original, zipped, errors)))
	{
		for (const FString& error : errors)
			AddError(error);
		return false;
	}

	// Keep the written file so it can be fed to Dialoguer's own importer (cross-check done outside this test).
	FFileHelper::SaveArrayToFile(zipped, *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MounteaArchiveTest"), TEXT("RoundTrip.mnteadlg")));

	FDialogueArchive again;
	errors.Reset();
	if (!TestTrue(TEXT("written zip reads back"), ReadArchiveFromBytes(zipped, again, errors)))
	{
		for (const FString& error : errors)
			AddError(error);
		return false;
	}

	TestEqual(TEXT("dialogues"), again.Dialogues.Num(), original.Dialogues.Num());
	TestEqual(TEXT("participants"), again.Participants.Num(), original.Participants.Num());
	TestEqual(TEXT("nodes"), again.Nodes.Num(), original.Nodes.Num());
	TestEqual(TEXT("edges"), again.Edges.Num(), original.Edges.Num());
	TestEqual(TEXT("strings"), again.Strings.Num(), original.Strings.Num());
	TestEqual(TEXT("root"), again.RootDialogueId, original.RootDialogueId);

	// Edge order is the sibling order and has to survive.
	for (int32 index = 0; index < original.Edges.Num() && index < again.Edges.Num(); ++index)
		TestEqual(FString::Printf(TEXT("edge %d id"), index), again.Edges[index].Id, original.Edges[index].Id);

	const FNode* originalLead = original.FindNode(original.RootDialogueId, TEXT("00000000-0000-4000-8000-000000000101"));
	const FNode* againLead = again.FindNode(again.RootDialogueId, TEXT("00000000-0000-4000-8000-000000000101"));
	if (originalLead && againLead && TestEqual(TEXT("row count"), againLead->Rows.Num(), originalLead->Rows.Num()))
	{
		for (int32 index = 0; index < originalLead->Rows.Num(); ++index)
		{
			TestTrue(TEXT("execution behaviour survives"), againLead->Rows[index].ExecutionBehaviour == originalLead->Rows[index].ExecutionBehaviour);
			TestTrue(TEXT("duration mode survives"), againLead->Rows[index].DurationMode == originalLead->Rows[index].DurationMode);
			TestEqual(TEXT("duration override survives"), againLead->Rows[index].DurationOverride, originalLead->Rows[index].DurationOverride);
			TestTrue(TEXT("audio bytes survive"), againLead->Rows[index].Audio.Bytes == originalLead->Rows[index].Audio.Bytes);
		}
	}

	const FParticipant* againWaldermar = again.FindParticipant(TEXT("00000000-0000-4000-8000-000000000030"));
	const FParticipant* originalWaldermar = original.FindParticipant(TEXT("00000000-0000-4000-8000-000000000030"));
	if (againWaldermar && originalWaldermar)
		TestTrue(TEXT("thumbnail bytes survive"), againWaldermar->Thumbnail.Bytes == originalWaldermar->Thumbnail.Bytes);

	// Same model written twice gives the same bytes apart from timestamps inside the records, so only size sanity is checked.
	TestTrue(TEXT("zip stays within the contract limit"), zipped.Num() < MounteaArchive::Contract::MaxCompressedBytes);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveRejectsInvalidTest,
	"Mountea.DialogueSystemEditor.Archive.Read.RejectsInvalidArchives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveRejectsInvalidTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	FEntries golden;
	if (!MounteaArchiveTest::LoadEntries(*this, TEXT("GoldenGreeting.mnteadlg"), golden))
		return false;

	// Old flat layout gets its own message, not "missing file".
	{
		FEntries flat;
		flat.Add(TEXT("dialogueData.json"), TArray<uint8>());
		flat.Add(TEXT("nodes.json"), TArray<uint8>());
		FDialogueArchive archive;
		TArray<FString> errors;
		TestFalse(TEXT("flat layout is rejected"), ReadArchive(flat, archive, errors));
		TestTrue(TEXT("flat layout error code"), MounteaArchiveTest::HasError(errors, TEXT("LEGACY_FLAT_ARCHIVE")));
	}

	// An entry the manifest does not declare.
	{
		FEntries stray = golden;
		stray.Add(TEXT("notes.txt"), TArray<uint8>());
		FDialogueArchive archive;
		TArray<FString> errors;
		TestFalse(TEXT("undeclared entry is rejected"), ReadArchive(stray, archive, errors));
		TestTrue(TEXT("undeclared entry error code"), MounteaArchiveTest::HasError(errors, TEXT("INVALID_ARCHIVE_MANIFEST")));
	}

	// Unknown row timing value inside a node record.
	{
		FEntries mutated = golden;
		bool bMutated = false;
		for (TPair<FString, TArray<uint8>>& entry : mutated)
		{
			if (!entry.Key.StartsWith(TEXT("nodes/")))
				continue;
			const FUTF8ToTCHAR Decoded(reinterpret_cast<const ANSICHAR*>(entry.Value.GetData()), entry.Value.Num());
			FString text(Decoded.Length(), Decoded.Get());
			if (!text.Contains(TEXT("\"awaitInput\"")))
				continue;
			text.ReplaceInline(TEXT("\"awaitInput\""), TEXT("\"bogus\""));
			const FTCHARToUTF8 Utf8(*text);
			entry.Value.Reset();
			entry.Value.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
			bMutated = true;
		}
		TestTrue(TEXT("fixture contains an awaitInput row"), bMutated);

		FDialogueArchive archive;
		TArray<FString> errors;
		TestFalse(TEXT("unknown executionBehaviour is rejected"), ReadArchive(mutated, archive, errors));
		TestTrue(TEXT("row timing error code"), MounteaArchiveTest::HasError(errors, TEXT("INVALID_ROW_TIMING")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveValidatesModelTest,
	"Mountea.DialogueSystemEditor.Archive.Validate.ModelRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveValidatesModelTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	FDialogueArchive archive;
	if (!MounteaArchiveTest::ReadFixture(*this, TEXT("GoldenGreeting.mnteadlg"), archive))
		return false;

	// Name rule: every offender is reported, not just the first.
	{
		FDialogueArchive broken = archive;
		broken.Participants[0].Name = TEXT("Name With Spaces");
		broken.Categories[0].Name = TEXT("ThisNameIsFarTooLong");
		TArray<FString> errors;
		ValidateArchive(broken, errors);
		TestTrue(TEXT("participant name offender is reported"), MounteaArchiveTest::HasError(errors, TEXT("INVALID_PARTICIPANT_NAME")));
		TestTrue(TEXT("category name offender is reported"), MounteaArchiveTest::HasError(errors, TEXT("INVALID_CATEGORY_NAME")));

		TArray<uint8> zipped;
		TArray<FString> writeErrors;
		TestFalse(TEXT("export is refused for invalid names"), WriteArchiveToBytes(broken, zipped, writeErrors));
		TestTrue(TEXT("nothing is produced on failure"), zipped.IsEmpty());
	}

	// Start node id.
	{
		FDialogueArchive broken = archive;
		for (FNode& node : broken.Nodes)
		{
			if (node.Type == Contract::NodeStart)
			{
				node.Id = TEXT("some-other-id");
				break;
			}
		}
		TArray<FString> errors;
		ValidateArchive(broken, errors);
		TestTrue(TEXT("wrong start id is reported"), MounteaArchiveTest::HasError(errors, TEXT("INVALID_START")));
	}

	// Default locale value must exist for every referenced key.
	{
		FDialogueArchive broken = archive;
		broken.Strings[0].Values.Remove(broken.Project.Localization.DefaultLocale);
		TArray<FString> errors;
		ValidateArchive(broken, errors);
		TestTrue(TEXT("missing default locale value is reported"), MounteaArchiveTest::HasError(errors, TEXT("MISSING_STRING")));
	}

	// Child dialogue must be part of the archive.
	{
		FDialogueArchive broken = archive;
		for (FNode& node : broken.Nodes)
		{
			if (node.Type == Contract::NodeOpenChildGraph)
				node.TargetDialogue = TEXT("not-in-archive");
		}
		TArray<FString> errors;
		ValidateArchive(broken, errors);
		TestTrue(TEXT("dangling child dialogue is reported"), MounteaArchiveTest::HasError(errors, TEXT("MISSING_TARGET")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveContractHelpersTest,
	"Mountea.DialogueSystemEditor.Archive.Contract.Helpers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveContractHelpersTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	TestTrue(TEXT("letters and digits are valid"), Contract::IsValidName(TEXT("Waldermar1")));
	TestFalse(TEXT("empty is invalid"), Contract::IsValidName(FString()));
	TestFalse(TEXT("space is invalid"), Contract::IsValidName(TEXT("Mr Smith")));
	TestFalse(TEXT("underscore is invalid"), Contract::IsValidName(TEXT("Mr_Smith")));
	TestFalse(TEXT("dot is invalid"), Contract::IsValidName(TEXT("NPC.Bob")));
	TestTrue(TEXT("exactly the maximum length is valid"), Contract::IsValidName(FString::ChrN(Contract::NameMaxLength, TEXT('a'))));
	TestFalse(TEXT("one over the maximum length is invalid"), Contract::IsValidName(FString::ChrN(Contract::NameMaxLength + 1, TEXT('a'))));

	// A GUID-shaped id is used as is; anything else maps to the same GUID every time.
	const FGuid direct = IdToGuid(TEXT("00000000-0000-4000-8000-000000000101"));
	TestEqual(TEXT("GUID id round trips"), GuidToId(direct), FString(TEXT("00000000-0000-4000-8000-000000000101")));
	const FGuid firstHash = IdToGuid(TEXT("not-a-guid"));
	TestEqual(TEXT("non-GUID id is deterministic"), IdToGuid(TEXT("not-a-guid")), firstHash);
	TestNotEqual(TEXT("different ids differ"), IdToGuid(TEXT("another-id")), firstHash);

	ERowExecutionMode execution;
	ERowDurationMode duration;
	TestTrue(TEXT("stopping parses"), Contract::ParseExecutionBehaviour(TEXT("stopping"), execution) && execution == ERowExecutionMode::EREM_Stopping);
	TestTrue(TEXT("calculate parses"), Contract::ParseDurationMode(TEXT("calculate"), duration) && duration == ERowDurationMode::ERDM_AutoCalculate);
	TestFalse(TEXT("unknown behaviour is refused"), Contract::ParseExecutionBehaviour(TEXT("manual"), execution));
	TestFalse(TEXT("Manual duration mode is not part of the contract"), Contract::ParseDurationMode(TEXT("manual"), duration));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
