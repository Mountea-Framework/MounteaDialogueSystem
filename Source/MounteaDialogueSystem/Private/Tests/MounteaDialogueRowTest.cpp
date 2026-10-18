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
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Sound/SoundWave.h"

// Dialogue rows (DataTable rows) and their lines: validity, how long a line stays on screen, and the hashing
// that lets these structs be map keys.

namespace MounteaDialogueRowTest
{
	static FDialogueRowData Line(const TCHAR* Text, const ERowDurationMode Mode, const float Duration, const float Override, USoundBase* Sound = nullptr)
	{
		return FDialogueRowData(FText::FromString(Text), Sound, Mode, Duration, Override);
	}
}

// IsDialogueRowValid gates whether a row is played at all.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueRowValidityTest,
	"Mountea.DialogueSystem.Rows.Validity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueRowValidityTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueRowTest;

	FDialogueRow row;
	row.DialogueParticipantName = TEXT("NPC");
	row.RowData.Add(Line(TEXT("Hello"), ERowDurationMode::ERDM_Duration, 1.f, 0.f));
	TestTrue(TEXT("Participant + lines + GUID is valid"), UMounteaDialogueTraversalStatics::IsDialogueRowValid(row));

	FDialogueRow textParticipant = row;
	textParticipant.DialogueParticipantName = NAME_None;
	textParticipant.DialogueParticipant = FText::FromString(TEXT("NPC"));
	TestTrue(TEXT("A display-text participant is enough"), UMounteaDialogueTraversalStatics::IsDialogueRowValid(textParticipant));

	FDialogueRow noParticipant = row;
	noParticipant.DialogueParticipantName = NAME_None;
	TestFalse(TEXT("No participant: invalid"), UMounteaDialogueTraversalStatics::IsDialogueRowValid(noParticipant));

	FDialogueRow noLines = row;
	noLines.RowData.Empty();
	TestFalse(TEXT("No lines: invalid"), UMounteaDialogueTraversalStatics::IsDialogueRowValid(noLines));

	TestFalse(TEXT("The Invalid() row is invalid"), UMounteaDialogueTraversalStatics::IsDialogueRowValid(FDialogueRow::Invalid()));
	TestFalse(TEXT("The Invalid() row reports IsValid false"), FDialogueRow::Invalid().IsValid());

	FDialogueRowData line = Line(TEXT("Hello"), ERowDurationMode::ERDM_Duration, 1.f, 0.f);
	TestTrue(TEXT("A line with a GUID is valid"), UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(line));
	line.RowGUID.Invalidate();
	TestFalse(TEXT("A line without a GUID is invalid"), UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(line));

	TestFalse(TEXT("A missing table yields an invalid row"), UMounteaDialogueTraversalStatics::IsDialogueRowValid(UMounteaDialogueTraversalStatics::GetDialogueRow(nullptr, TEXT("Row"))));
	return true;
}

// GetRowDuration decides when the next line starts; each duration mode has its own rule, and the result is never
// zero or negative (a zero timer would never fire).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueRowDurationTest,
	"Mountea.DialogueSystem.Rows.DurationModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueRowDurationTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueRowTest;
	using UStatics = UMounteaDialogueTraversalStatics;

	USoundWave* sound = NewObject<USoundWave>(GetTransientPackage());
	sound->Duration = 3.f;

	TestEqual(TEXT("Duration: uses RowDuration without sound"), UStatics::GetRowDuration(Line(TEXT("x"), ERowDurationMode::ERDM_Duration, 2.5f, 9.f)), 2.5f);
	TestEqual(TEXT("Duration: uses the sound length when there is one"), UStatics::GetRowDuration(Line(TEXT("x"), ERowDurationMode::ERDM_Duration, 2.5f, 9.f, sound)), 3.f);
	TestEqual(TEXT("Override: uses the override"), UStatics::GetRowDuration(Line(TEXT("x"), ERowDurationMode::EDRM_Override, 2.5f, 4.f, sound)), 4.f);
	TestEqual(TEXT("Add: sound length plus override"), UStatics::GetRowDuration(Line(TEXT("x"), ERowDurationMode::EDRM_Add, 2.5f, 1.5f, sound)), 4.5f);
	TestEqual(TEXT("Add: override alone without sound"), UStatics::GetRowDuration(Line(TEXT("x"), ERowDurationMode::EDRM_Add, 2.5f, 1.5f)), 1.5f);

	const FString text(TEXT("Fifty characters of text for the calculate mode!!"));
	const float coefficient = GetDefault<UMounteaDialogueSystemSettings>()->GetDurationCoefficient();
	TestEqual(TEXT("Calculate: length * coefficient / 100"),
		UStatics::GetRowDuration(Line(*text, ERowDurationMode::ERDM_AutoCalculate, 0.f, 0.f)), text.Len() * coefficient / 100.f);

	TestTrue(TEXT("Zero is clamped above zero"), UStatics::GetRowDuration(Line(TEXT("x"), ERowDurationMode::EDRM_Override, 0.f, 0.f)) > 0.f);
	TestTrue(TEXT("Negative is clamped above zero"), UStatics::GetRowDuration(Line(TEXT("x"), ERowDurationMode::EDRM_Override, 0.f, -2.f)) > 0.f);

	FDialogueRowData invalid = Line(TEXT("x"), ERowDurationMode::EDRM_Override, 0.f, 7.f);
	invalid.RowGUID.Invalidate();
	TestEqual(TEXT("An invalid line falls back to one second"), UStatics::GetRowDuration(invalid), 1.f);
	return true;
}

// Equal keys must hash equally wherever they live in memory; FUIRowID keys the subtitle overrides map.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueRowHashTest,
	"Mountea.DialogueSystem.Rows.HashingIsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueRowHashTest::RunTest(const FString& Parameters)
{
	// Two equal ids followed by different bytes: a hash that reads past the struct would tell them apart.
	struct FPadded
	{
		FUIRowID Id;
		uint64 Trailing;
	};
	FPadded first{ FUIRowID(), 0x1111111111111111ull };
	FPadded second{ FUIRowID(), 0x2222222222222222ull };
	first.Id.UIRowID = 4;
	second.Id.UIRowID = 4;

	TestTrue(TEXT("Precondition: the ids are equal"), first.Id == second.Id);
	TestEqual(TEXT("Equal FUIRowIDs hash equally"), GetTypeHash(first.Id), GetTypeHash(second.Id));

	TMap<FUIRowID, int32> overrides;
	overrides.Add(first.Id, 42);
	FUIRowID lookup;
	lookup.UIRowID = 4;
	TestTrue(TEXT("A map keyed by FUIRowID finds an equal key"), overrides.Contains(lookup));

	FDialogueTraversePath pathA(FGuid::NewGuid(), FGuid::NewGuid(), 1);
	FDialogueTraversePath pathB(pathA.NodeGuid, pathA.GraphGuid, 5);
	TestTrue(TEXT("Traverse paths compare by GUIDs only"), pathA == pathB);
	TestEqual(TEXT("Traverse paths hash by GUIDs only"), GetTypeHash(pathA), GetTypeHash(pathB));
	pathA += pathB;
	TestEqual(TEXT("+= adds counts for the same node"), pathA.TraverseCount, 6);

	FDialogueRowData line;
	FDialogueRowData copy(line);
	TestTrue(TEXT("Copy-constructed lines keep identity"), line == copy);
	TestEqual(TEXT("and hash"), GetTypeHash(line), GetTypeHash(copy));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
