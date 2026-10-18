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

#include "GameplayTagContainer.h"
#include "Settings/MounteaDialogueConfiguration.h"
#include "UObject/Package.h"

// Graph-type resolution from a graph's tags (Dialogue vs Monologue). Uses a fresh configuration object with the
// shipped defaults, so the result does not depend on the project's own configuration asset.

namespace MounteaDialogueSettingsTest
{
	static FGameplayTagContainer Tags(std::initializer_list<const TCHAR*> Names)
	{
		FGameplayTagContainer container;
		for (const TCHAR* name : Names)
			container.AddTag(FGameplayTag::RequestGameplayTag(name));
		return container;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSettingsGraphTypeTest,
	"Mountea.DialogueSystem.Settings.ResolveGraphTypeFromTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSettingsGraphTypeTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueSettingsTest;

	const UMounteaDialogueConfiguration* config = NewObject<UMounteaDialogueConfiguration>(GetTransientPackage());
	FName typeId;
	FString reason;

	TestTrue(TEXT("No type tag: resolves"), config->ResolveGraphTypeFromTags(FGameplayTagContainer(), typeId, reason));
	TestEqual(TEXT("No type tag: the default type"), typeId, FName(TEXT("Dialogue")));

	TestTrue(TEXT("Monologue tag: resolves"), config->ResolveGraphTypeFromTags(Tags({ TEXT("Mountea_Dialogue.Graph.Type.Monologue") }), typeId, reason));
	TestEqual(TEXT("Monologue tag: Monologue"), typeId, FName(TEXT("Monologue")));

	TestTrue(TEXT("Dialogue tag: resolves"), config->ResolveGraphTypeFromTags(Tags({ TEXT("Mountea_Dialogue.Graph.Type.Dialogue") }), typeId, reason));
	TestEqual(TEXT("Dialogue tag: Dialogue"), typeId, FName(TEXT("Dialogue")));

	TestFalse(TEXT("Both type tags: ambiguous"),
		config->ResolveGraphTypeFromTags(Tags({ TEXT("Mountea_Dialogue.Graph.Type.Dialogue"), TEXT("Mountea_Dialogue.Graph.Type.Monologue") }), typeId, reason));
	TestTrue(TEXT("Ambiguity is explained"), reason.Contains(TEXT("Ambiguous")));
	TestEqual(TEXT("Nothing is resolved on failure"), typeId, FName(NAME_None));

	TestFalse(TEXT("The namespace tag alone is not a type"), config->ResolveGraphTypeFromTags(Tags({ TEXT("Mountea_Dialogue.Graph.Type") }), typeId, reason));
	TestTrue(TEXT("Unknown type tags are named"), reason.Contains(TEXT("Unknown graph type tags")));

	UMounteaDialogueConfiguration* broken = NewObject<UMounteaDialogueConfiguration>(GetTransientPackage());
	broken->DefaultGraphTypeId = TEXT("Missing");
	TestFalse(TEXT("A default type that is not defined fails"), broken->ResolveGraphTypeFromTags(FGameplayTagContainer(), typeId, reason));
	TestTrue(TEXT("and says which"), reason.Contains(TEXT("Missing")));

	broken->DefaultGraphTypeId = TEXT("Dialogue");
	const FMounteaDialogueGraphTypeDefinition duplicate = broken->GraphTypeDefinitions[0];
	broken->GraphTypeDefinitions.Add(duplicate);
	TestFalse(TEXT("Duplicate type ids fail"), broken->ResolveGraphTypeFromTags(FGameplayTagContainer(), typeId, reason));
	TestTrue(TEXT("and say so"), reason.Contains(TEXT("duplicate")));

	broken->GraphTypeDefinitions.Empty();
	TestFalse(TEXT("No definitions fail"), broken->ResolveGraphTypeFromTags(FGameplayTagContainer(), typeId, reason));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
