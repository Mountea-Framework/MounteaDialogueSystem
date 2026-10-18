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
#include "Factories/MounteaDialogueDataTableFactory.h"
#include "Factories/MounteaDialogueGraphFactory.h"
#include "Factories/MounteaDialogueProjectFactory.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

// Asset factories: which files they claim on import, and what "New asset" creates. Factories that open a class
// picker (Condition, Decorator, Node, Config, Additional Data) are not driven here: the picker is modal.

// Each factory claims its own extension, case-insensitively, and nothing else.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaFactoryCanImportTest,
	"Mountea.DialogueSystemEditor.Factory.CanImportByExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaFactoryCanImportTest::RunTest(const FString& Parameters)
{
	UMounteaDialogueGraphFactory* graphFactory = NewObject<UMounteaDialogueGraphFactory>(GetTransientPackage());
	UMounteaDialogueProjectFactory* projectFactory = NewObject<UMounteaDialogueProjectFactory>(GetTransientPackage());

	TestTrue(TEXT("Graph factory: .mnteadlg"), graphFactory->FactoryCanImport(TEXT("C:/In/Greeting.mnteadlg")));
	TestTrue(TEXT("Graph factory: upper-case extension"), graphFactory->FactoryCanImport(TEXT("C:/In/Greeting.MNTEADLG")));
	TestFalse(TEXT("Graph factory: not a project"), graphFactory->FactoryCanImport(TEXT("C:/In/Golden.mnteadlgproj")));
	TestFalse(TEXT("Graph factory: not json"), graphFactory->FactoryCanImport(TEXT("C:/In/dialogue.json")));
	TestFalse(TEXT("Graph factory: no extension"), graphFactory->FactoryCanImport(TEXT("C:/In/Greeting")));

	TestTrue(TEXT("Project factory: .mnteadlgproj"), projectFactory->FactoryCanImport(TEXT("C:/In/Golden.mnteadlgproj")));
	TestTrue(TEXT("Project factory: upper-case extension"), projectFactory->FactoryCanImport(TEXT("C:/In/Golden.MNTEADLGPROJ")));
	TestFalse(TEXT("Project factory: not a single dialogue"), projectFactory->FactoryCanImport(TEXT("C:/In/Greeting.mnteadlg")));

	TestTrue(TEXT("Graph factory supports the graph class"), graphFactory->SupportedClass == UMounteaDialogueGraph::StaticClass());
	TestTrue(TEXT("Graph factory lists its format"), graphFactory->Formats.ContainsByPredicate([](const FString& Format) { return Format.StartsWith(TEXT("mnteadlg;")); }));
	return true;
}

// "New Dialogue Graph" creates a graph with its Start node; only graphs can be reimported.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaFactoryCreateGraphTest,
	"Mountea.DialogueSystemEditor.Factory.CreateNewGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaFactoryCreateGraphTest::RunTest(const FString& Parameters)
{
	UMounteaDialogueGraphFactory* factory = NewObject<UMounteaDialogueGraphFactory>(GetTransientPackage());
	UObject* created = factory->FactoryCreateNew(UMounteaDialogueGraph::StaticClass(), GetTransientPackage(), TEXT("FactoryGraph"), RF_Transient, nullptr, GWarn);
	UMounteaDialogueGraph* graph = Cast<UMounteaDialogueGraph>(created);
	if (!TestNotNull(TEXT("A graph is created"), graph))
		return false;

	TestTrue(TEXT("It is transactional (undo works)"), graph->HasAnyFlags(RF_Transactional));
	TestTrue(TEXT("It already has a Start node"), graph->GetStartNode() && graph->GetStartNode()->IsA<UMounteaDialogueGraphNode_StartNode>());

	TArray<FString> files;
	TestTrue(TEXT("A graph can be reimported"), factory->CanReimport(graph, files));
	TestFalse(TEXT("Other objects cannot"), factory->CanReimport(NewObject<UMounteaDialogueDataTable>(GetTransientPackage()), files));
	TestFalse(TEXT("Null cannot"), factory->CanReimport(nullptr, files));
	return true;
}

// "New Dialogue Data Table" creates a dialogue row table, not a generic one.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaFactoryCreateDataTableTest,
	"Mountea.DialogueSystemEditor.Factory.CreateDialogueDataTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaFactoryCreateDataTableTest::RunTest(const FString& Parameters)
{
	UMounteaDialogueDataTableFactory* factory = NewObject<UMounteaDialogueDataTableFactory>(GetTransientPackage());
	UObject* created = factory->FactoryCreateNew(UMounteaDialogueDataTable::StaticClass(), GetTransientPackage(), TEXT("FactoryRows"), RF_Public | RF_Transient, nullptr, GWarn);
	const UDataTable* table = Cast<UDataTable>(created);
	if (!TestNotNull(TEXT("A table is created"), table))
		return false;

	TestTrue(TEXT("It is a Mountea dialogue table"), table->IsA<UMounteaDialogueDataTable>());
	TestTrue(TEXT("Its rows are dialogue rows"), table->GetRowStruct() == FDialogueRow::StaticStruct());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
