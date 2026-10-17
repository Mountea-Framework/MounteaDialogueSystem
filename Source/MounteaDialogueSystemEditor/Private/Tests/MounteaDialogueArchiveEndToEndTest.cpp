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
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Engine/DataTable.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"
#include "HAL/FileManager.h"
#include "ImportConfig/MounteaDialogueImportConfig.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "UObject/Package.h"

// End to end: golden Dialoguer archive -> assets -> reimport (twice, from another folder) -> export -> read back.
// Creates real assets under /Game, so it is an EngineFilter test (not part of the default product run) that
// removes everything it created. Run it in the commandlet (-nullrhi), not in an interactive editor session.

namespace MounteaEndToEnd
{
	const FGuid RootGuid(0x00000000, 0x00004000, 0x80000000, 0x00000010);
	const FGuid ChildGuid(0x00000000, 0x00004000, 0x80000000, 0x00000011);

	int32 CountExportedEdges(const UMounteaDialogueGraph* Graph, const FString& Path, FAutomationTestBase& Test, MounteaArchive::FDialogueArchive* OutArchive = nullptr)
	{
		if (!UMounteaDialogueSystemImportExportHelpers::ExportDialogueGraph(Graph, Path))
		{
			Test.AddError(TEXT("export failed"));
			return -1;
		}

		TArray<uint8> bytes;
		MounteaArchive::FDialogueArchive archive;
		TArray<FString> errors;
		if (!FFileHelper::LoadFileToArray(bytes, *Path) || !MounteaArchive::ReadArchiveFromBytes(bytes, archive, errors))
		{
			for (const FString& error : errors)
				Test.AddError(error);
			return -1;
		}

		if (OutArchive)
			*OutArchive = archive;
		return archive.Edges.Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaArchiveEndToEndTest,
	"Mountea.DialogueSystemEditor.Import.EndToEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMounteaArchiveEndToEndTest::RunTest(const FString& Parameters)
{
	const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	const FString sourcePath = FPaths::Combine(plugin->GetBaseDir(), TEXT("Source/MounteaDialogueSystemEditor/Private/Tests/Fixtures/GoldenGreeting.mnteadlg"));

	const FString contentDir = FPaths::ProjectContentDir();
	const FString configDir = FPaths::ProjectConfigDir();
	const FString exportDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MounteaArchiveEndToEnd"));
	const FString gameIniPath = FPaths::Combine(configDir, TEXT("DefaultGame.ini"));
	const FString tagsPath = FPaths::Combine(configDir, TEXT("Tags"), TEXT("MounteaDialogueParticipants.ini"));
	const FString localizationTarget = FPaths::Combine(contentDir, TEXT("Localization"), TEXT("MounteaDialogue"));

	// State to restore afterwards.
	FString gameIniBefore;
	const bool bGameIniExisted = FFileHelper::LoadFileToString(gameIniBefore, *gameIniPath);
	const bool bTagsExisted = FPaths::FileExists(tagsPath);
	const bool bLocalizationExisted = IFileManager::Get().DirectoryExists(*localizationTarget);

	UPackage* package = CreatePackage(TEXT("/Game/MounteaArchiveE2E/GoldenGreeting"));
	package->AddToRoot(); // the import triggers GC; the factory normally keeps the outer alive
	UPackage* otherPackage = CreatePackage(TEXT("/Game/MounteaArchiveE2E_Other/GoldenGreeting"));
	otherPackage->AddToRoot();

	UMounteaDialogueGraph* graph = nullptr;
	FString message;
	const bool bImported = UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraph(sourcePath, package, TEXT("GoldenGreeting"), RF_Public | RF_Standalone, graph, message);
	if (TestTrue(TEXT("import succeeds"), bImported) && TestNotNull(TEXT("root graph"), graph))
	{
		TestEqual(TEXT("graph guid comes from the archive"), graph->GetGraphGUID(), MounteaEndToEnd::RootGuid);

		UMounteaDialogueGraphNode* lead = nullptr;
		UMounteaDialogueGraphNode_OpenChildGraph* openChild = nullptr;
		for (UMounteaDialogueGraphNode* node : graph->AllNodes)
		{
			if (node->IsA<UMounteaDialogueGraphNode_LeadNode>())
				lead = node;
			if (UMounteaDialogueGraphNode_OpenChildGraph* candidate = Cast<UMounteaDialogueGraphNode_OpenChildGraph>(node))
				openChild = candidate;
		}

		if (TestNotNull(TEXT("lead node"), lead))
		{
			TestEqual(TEXT("lead children"), lead->ChildrenNodes.Num(), 3);
			for (int32 index = 0; index < lead->ChildrenNodes.Num(); ++index)
				TestEqual(FString::Printf(TEXT("child %d execution order follows the edge order"), index), lead->ChildrenNodes[index]->ExecutionOrder, index);

			if (const UMounteaDialogueGraphEdge* firstEdge = lead->Edges.FindRef(lead->ChildrenNodes[0]))
				TestEqual(TEXT("first edge condition rules"), firstEdge->EdgeConditions.Rules.Num(), 1);
			else
				AddError(TEXT("first edge missing"));

			if (UMounteaDialogueGraphNode_DialogueNodeBase* dialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(lead))
			{
				const FDialogueRow* row = dialogueNode->GetDataTable() ? dialogueNode->GetDataTable()->FindRow<FDialogueRow>(dialogueNode->GetRowName(), TEXT("e2e")) : nullptr;
				if (TestNotNull(TEXT("dialogue row"), row) && TestEqual(TEXT("row data"), row->RowData.Num(), 3))
				{
					TestTrue(TEXT("awaitInput"), row->RowData[1].RowExecutionBehaviour == ERowExecutionMode::EREM_AwaitInput);
					TestTrue(TEXT("add mode with override"), row->RowData[1].RowDurationMode == ERowDurationMode::EDRM_Add && FMath::IsNearlyEqual(row->RowData[1].RowDurationOverride, 2.f));
					TestTrue(TEXT("stopping + calculate"), row->RowData[2].RowExecutionBehaviour == ERowExecutionMode::EREM_Stopping && row->RowData[2].RowDurationMode == ERowDurationMode::ERDM_AutoCalculate);
					TestNotNull(TEXT("audio imported"), row->RowData[0].RowSound.Get());
					TestFalse(TEXT("row text resolves"), row->RowData[0].RowText.ToString().IsEmpty());
				}
			}
		}

		if (TestNotNull(TEXT("open child node"), openChild))
			TestFalse(TEXT("child dialogue resolved"), openChild->TargetDialogue.IsNull());

		TestTrue(TEXT("cs locres written"), FPaths::FileExists(FPaths::Combine(localizationTarget, TEXT("cs"), TEXT("MounteaDialogue.locres"))));

		// Export, then check translations survived the .po round trip.
		MounteaArchive::FDialogueArchive exported;
		const int32 edgesAfterImport = MounteaEndToEnd::CountExportedEdges(graph, FPaths::Combine(exportDir, TEXT("first.mnteadlg")), *this, &exported);
		TestEqual(TEXT("exported edges after import"), edgesAfterImport, 10);
		TestEqual(TEXT("exported dialogues (root + child)"), exported.Dialogues.Num(), 2);
		const bool bHasCzech = exported.Strings.ContainsByPredicate([](const MounteaArchive::FLocalizedString& Entry) { return Entry.Values.FindRef(TEXT("cs")).StartsWith(TEXT("CS ")); });
		TestTrue(TEXT("exported strings keep the cs translations"), bHasCzech);
		TestTrue(TEXT("cs is a supported locale"), exported.Project.Localization.SupportedLocales.Contains(TEXT("cs")));

		// Reimport twice in place: nothing may pile up.
		for (int32 round = 1; round <= 2; ++round)
		{
			UMounteaDialogueGraph* again = graph;
			FString reimportMessage;
			TestTrue(FString::Printf(TEXT("reimport %d succeeds"), round), UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(sourcePath, package, again, reimportMessage));
			TestTrue(FString::Printf(TEXT("reimport %d reuses the graph"), round), again == graph);
			const int32 edges = MounteaEndToEnd::CountExportedEdges(graph, FPaths::Combine(exportDir, FString::Printf(TEXT("reimport%d.mnteadlg"), round)), *this);
			TestEqual(FString::Printf(TEXT("exported edges after reimport %d"), round), edges, 10);
		}

		// Dropping the same file into another folder updates the existing graph and keeps shared assets with it.
		UMounteaDialogueGraph* elsewhere = nullptr;
		FString elsewhereMessage;
		const bool bElsewhere = UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(sourcePath, otherPackage, elsewhere, elsewhereMessage);
		TestTrue(TEXT("reimport from another folder succeeds"), bElsewhere);
		TestTrue(TEXT("it is the same graph"), elsewhere == graph);
		TestFalse(TEXT("no Decorators folder in the other location"), IFileManager::Get().DirectoryExists(*FPaths::Combine(contentDir, TEXT("MounteaArchiveE2E_Other"), TEXT("Decorators"))));
		TestFalse(TEXT("no thumbnails in the other location"), IFileManager::Get().DirectoryExists(*FPaths::Combine(contentDir, TEXT("MounteaArchiveE2E_Other"), TEXT("thumbnails"))));
	}

	package->RemoveFromRoot();
	otherPackage->RemoveFromRoot();

	// Cleanup: everything this test created.
	IFileManager::Get().DeleteDirectory(*FPaths::Combine(contentDir, TEXT("MounteaArchiveE2E")), false, true);
	IFileManager::Get().DeleteDirectory(*FPaths::Combine(contentDir, TEXT("MounteaArchiveE2E_Other")), false, true);
	for (const TCHAR* table : { TEXT("ST_Greeting_DialogueRows"), TEXT("ST_Greeting_Nodes"), TEXT("ST_Haggle_DialogueRows"), TEXT("ST_Haggle_Nodes") })
		IFileManager::Get().DeleteDirectory(*FPaths::Combine(contentDir, TEXT("Localization"), table), false, true);
	if (!bLocalizationExisted)
		IFileManager::Get().DeleteDirectory(*localizationTarget, false, true);
	if (!bTagsExisted)
		IFileManager::Get().Delete(*tagsPath);
	if (bGameIniExisted)
		FFileHelper::SaveStringToFile(gameIniBefore, *gameIniPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	else
		IFileManager::Get().Delete(*gameIniPath);
	IFileManager::Get().DeleteDirectory(*exportDir, false, true);

	if (UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>())
	{
		importConfig->DialogueHistory.Remove(MounteaEndToEnd::RootGuid);
		importConfig->DialogueHistory.Remove(MounteaEndToEnd::ChildGuid);
		importConfig->SaveToFile();
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
