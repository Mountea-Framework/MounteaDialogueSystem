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

// Byte-level zip IO under the archive layer: archives come from users and Dialoguer, so malformed and hostile
// input must be refused with a code, never crash or write outside the archive.

// Whatever goes in comes back out, byte for byte, including binary and empty entries.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveZipRoundTripTest,
	"Mountea.DialogueSystemEditor.Archive.Zip.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveZipRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	FEntries entries;
	FTCHARToUTF8 json(TEXT("{\"hello\":\"wörld\"}"));
	entries.Add(TEXT("manifest.json"), TArray<uint8>(reinterpret_cast<const uint8*>(json.Get()), json.Length()));
	TArray<uint8> binary;
	for (int32 i = 0; i < 4096; ++i)
		binary.Add(static_cast<uint8>(i * 31));
	entries.Add(TEXT("media/audio/line.wav"), binary);
	entries.Add(TEXT("empty.txt"), TArray<uint8>());

	TArray<uint8> zipped;
	TArray<FString> errors;
	if (!TestTrue(TEXT("Zipping succeeds"), ZipEntries(entries, zipped, errors)))
		return false;

	FEntries unzipped;
	TestTrue(TEXT("Unzipping succeeds"), UnzipEntries(zipped, unzipped, errors));
	TestEqual(TEXT("Every entry comes back"), unzipped.Num(), entries.Num());
	for (const TPair<FString, TArray<uint8>>& pair : entries)
	{
		const TArray<uint8>* found = unzipped.Find(pair.Key);
		TestTrue(FString::Printf(TEXT("'%s' is byte-identical"), *pair.Key), found && *found == pair.Value);
	}
	TestEqual(TEXT("No errors"), errors.Num(), 0);
	return true;
}

// Garbage, truncated and unsafe input is refused with a contract error code.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveZipRejectsTest,
	"Mountea.DialogueSystemEditor.Archive.Zip.RejectsBadInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveZipRejectsTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	{
		FEntries out;
		TArray<FString> errors;
		TestFalse(TEXT("An empty buffer is refused"), UnzipEntries(TArray<uint8>(), out, errors));
		TestTrue(TEXT("with INVALID_ARCHIVE"), MounteaArchiveTest::HasError(errors, TEXT("INVALID_ARCHIVE")));
	}
	{
		TArray<uint8> garbage;
		for (int32 i = 0; i < 512; ++i)
			garbage.Add(static_cast<uint8>(i * 7 + 3));
		FEntries out;
		TArray<FString> errors;
		TestFalse(TEXT("Random bytes are refused"), UnzipEntries(garbage, out, errors));
		TestTrue(TEXT("with INVALID_ARCHIVE"), MounteaArchiveTest::HasError(errors, TEXT("INVALID_ARCHIVE")));
	}
	{
		TArray<uint8> golden;
		if (MounteaArchiveTest::LoadFixture(TEXT("GoldenGreeting.mnteadlg"), golden))
		{
			golden.SetNum(golden.Num() / 2);
			FEntries out;
			TArray<FString> errors;
			TestFalse(TEXT("A truncated archive is refused"), UnzipEntries(golden, out, errors));
			TestTrue(TEXT("and explains why"), errors.Num() > 0);
		}
	}

	for (const TCHAR* unsafe : { TEXT("../evil.json"), TEXT("/abs.json"), TEXT("C:/drive.json"), TEXT("a\\b.json"), TEXT("a//b.json"), TEXT("./x.json") })
	{
		FEntries entries;
		entries.Add(unsafe, TArray<uint8>({ 1, 2, 3 }));
		TArray<uint8> zipped;
		TArray<FString> errors;
		TestFalse(FString::Printf(TEXT("Writing '%s' is refused"), unsafe), ZipEntries(entries, zipped, errors));
		TestTrue(FString::Printf(TEXT("'%s' reports UNSAFE_ARCHIVE_PATH"), unsafe), MounteaArchiveTest::HasError(errors, TEXT("UNSAFE_ARCHIVE_PATH")));
		TestEqual(FString::Printf(TEXT("'%s' leaves no output"), unsafe), zipped.Num(), 0);
	}
	return true;
}

// Contract helpers not covered by MounteaDialogueArchiveTest: node types, limits, timing round trips.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveContractExtraTest,
	"Mountea.DialogueSystemEditor.Archive.Contract.NodeTypesAndTiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaArchiveContractExtraTest::RunTest(const FString& Parameters)
{
	using namespace MounteaArchive;

	for (const TCHAR* type : { Contract::NodeStart, Contract::NodeLead, Contract::NodeAnswer, Contract::NodeComplete,
		Contract::NodeReturn, Contract::NodeOpenChildGraph, Contract::NodeDelay })
	{
		TestTrue(FString::Printf(TEXT("'%s' is a known node type"), type), Contract::IsKnownNodeType(type));
	}
	TestFalse(TEXT("Unknown node types are refused"), Contract::IsKnownNodeType(TEXT("teleport")));
	TestFalse(TEXT("Empty node type is refused"), Contract::IsKnownNodeType(FString()));

	TestTrue(TEXT("16 characters is the longest valid name"), Contract::IsValidName(TEXT("Abcdefghijklmnop")));
	TestFalse(TEXT("17 characters is too long"), Contract::IsValidName(TEXT("Abcdefghijklmnopq")));
	TestFalse(TEXT("An empty name is invalid"), Contract::IsValidName(FString()));

	for (const ERowExecutionMode mode : { ERowExecutionMode::EREM_Automatic, ERowExecutionMode::EREM_AwaitInput, ERowExecutionMode::EREM_Stopping })
	{
		ERowExecutionMode parsed = ERowExecutionMode::Default;
		const FString text = Contract::ExecutionBehaviourToString(mode);
		TestTrue(FString::Printf(TEXT("Execution '%s' round-trips"), *text), Contract::ParseExecutionBehaviour(text, parsed) && parsed == mode);
	}
	for (const ERowDurationMode mode : { ERowDurationMode::ERDM_Duration, ERowDurationMode::EDRM_Override, ERowDurationMode::EDRM_Add, ERowDurationMode::ERDM_AutoCalculate })
	{
		ERowDurationMode parsed = ERowDurationMode::Default;
		const FString text = Contract::DurationModeToString(mode);
		TestTrue(FString::Printf(TEXT("Duration '%s' round-trips"), *text), Contract::ParseDurationMode(text, parsed) && parsed == mode);
	}

	TestTrue(TEXT("Limits are ordered sensibly"), Contract::MaxJsonEntryBytes < Contract::MaxCompressedBytes && Contract::MaxCompressedBytes < Contract::MaxExpandedBytes);

	FDialogueArchive archive;
	if (MounteaArchiveTest::ReadFixture(*this, TEXT("GoldenGreeting.mnteadlg"), archive))
	{
		const FNode& anyNode = archive.Nodes[0];
		TestTrue(TEXT("FindNode finds a node by id"), archive.FindNode(anyNode.DialogueId, anyNode.Id) == &archive.Nodes[0]);
		TestNull(TEXT("FindNode returns null for unknown ids"), archive.FindNode(anyNode.DialogueId, TEXT("nope")));
		TestNotNull(TEXT("FindDialogue finds the root dialogue"), archive.FindDialogue(archive.RootDialogueId));
		TestNotNull(TEXT("FindParticipant finds a participant"), archive.FindParticipant(archive.Participants[0].Id));
		TestNull(TEXT("FindParticipant returns null for unknown ids"), archive.FindParticipant(TEXT("nope")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
