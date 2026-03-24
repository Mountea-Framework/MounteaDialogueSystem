// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "MounteaDialogueProjectFactory.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "Data/MounteaDialogueGraphExtraDataTypes.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"
#include "ImportConfig/MounteaDialogueImportConfig.h"

#include "Editor.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Misc/MessageDialog.h"
#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueProjectFactory"

UMounteaDialogueProjectFactory::UMounteaDialogueProjectFactory()
{
	SupportedClass = UObject::StaticClass();
	Formats.Add(TEXT("mnteadlgproj;Mountea Dialogue Project"));
	bCreateNew = false;
	bEditorImport = true;
}

bool UMounteaDialogueProjectFactory::FactoryCanImport(const FString& Filename)
{
	return FPaths::GetExtension(Filename).ToLower() == TEXT("mnteadlgproj");
}

UObject* UMounteaDialogueProjectFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	// ── 1. Load the outer archive ──────────────────────────────────────────────
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *Filename))
	{
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
			FText::Format(LOCTEXT("LoadFailed", "Failed to read project file:\n{0}"), FText::FromString(Filename)),
			5.f, TEXT("MDSStyleSet.Info.Error"));
		bOutOperationCanceled = true;
		return nullptr;
	}

	if (!UMounteaDialogueSystemImportExportHelpers::IsZipFile(fileData))
	{
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
			FText::Format(LOCTEXT("NotZip", "Project file is not a valid archive:\n{0}"), FText::FromString(Filename)),
			5.f, TEXT("MDSStyleSet.Info.Error"));
		bOutOperationCanceled = true;
		return nullptr;
	}

	// ── 2. Extract ─────────────────────────────────────────────────────────────
	TMap<FString, FString> extractedFiles;
	if (!UMounteaDialogueSystemImportExportHelpers::ExtractFilesFromZip(fileData, extractedFiles))
	{
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
			FText::Format(LOCTEXT("ExtractFailed", "Failed to extract project archive:\n{0}"), FText::FromString(Filename)),
			5.f, TEXT("MDSStyleSet.Info.Error"));
		bOutOperationCanceled = true;
		return nullptr;
	}

	// ── 3. Parse projectData.json ──────────────────────────────────────────────
	FString projectName = InName.ToString();
	FGuid projectGuid;
	if (extractedFiles.Contains(TEXT("projectData.json")))
	{
		TSharedPtr<FJsonObject> projectData;
		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(extractedFiles[TEXT("projectData.json")]);
		if (FJsonSerializer::Deserialize(reader, projectData) && projectData.IsValid())
		{
			projectData->TryGetStringField(TEXT("projectName"), projectName);
			FString projectGuidStr;
			if (projectData->TryGetStringField(TEXT("projectGuid"), projectGuidStr))
				FGuid::Parse(projectGuidStr, projectGuid);
		}
	}

	// Determine the project folder: reuse the stored one on reimport (prevents duplicate folders),
	// fall back to computing a new folder from the current drop location.
	FString projectFolder;
	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
	if (importConfig && projectGuid.IsValid() &&
		importConfig->LookupProjectFolder(projectGuid, projectFolder))
	{
		// Existing project — reimport in place.
	}
	else
	{
		// First import — place under the drop location.
		const FString packageRoot = FPackageName::GetLongPackagePath(InParent->GetPathName());
		projectFolder = FString::Printf(TEXT("%s/%s"), *packageRoot, *projectName);
	}

	if (GEditor)
	{
		const TArray<UMounteaDialogueGraph*> affectedGraphs =
			UMounteaDialogueSystemImportExportHelpers::FindGraphsToBeReimported(extractedFiles);

		TArray<UMounteaDialogueGraph*> openGraphs;
		if (UAssetEditorSubsystem* aes = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			for (UMounteaDialogueGraph* graph : affectedGraphs)
				if (aes->FindEditorForAsset(graph, false))
					openGraphs.Add(graph);
		}

		if (!openGraphs.IsEmpty())
		{
			FString graphNames;
			for (UMounteaDialogueGraph* graph : openGraphs)
				graphNames += FString::Printf(TEXT("\n  \u2022 %s"), *graph->GetName());

			const EAppReturnType::Type response = FMessageDialog::Open(
				EAppMsgType::YesNo,
				FText::Format(
					LOCTEXT("OpenEditorsDetected",
						"The following dialogue graph(s) are currently open and must be closed before reimport:{0}\n\nClose them and proceed with the import?"),
					FText::FromString(graphNames)));

			if (response != EAppReturnType::Yes)
			{
				bOutOperationCanceled = true;
				return nullptr;
			}

			if (UAssetEditorSubsystem* aes = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
				for (UMounteaDialogueGraph* graph : openGraphs)
					aes->CloseAllEditorsForAsset(graph);
		}
	}

	// Project-level files to inject into each dialogue if the nested archive is missing them.
	// Dialoguer exports these at the project level; standalone .mnteadlg files include them
	// directly. We inject them so the shared import pipeline sees a complete set.
	const TArray<FString> projectLevelFiles =
	{
		TEXT("participants.json"),
		TEXT("categories.json"),
		TEXT("decorators.json"),
		TEXT("conditions.json"),
	};

	// ── 4. Import each nested .mnteadlg ───────────────────────────────────────
	TArray<FString> importedDialogues;
	TArray<FGuid> importedDialogueGuids;
	TMap<FGuid, UMounteaDialogueGraph*> importedGraphsByGuid;

	for (const auto& entry : extractedFiles)
	{
		if (!entry.Key.StartsWith(TEXT("dialogues/")))
			continue;
		if (!entry.Key.EndsWith(TEXT(".mnteadlg")))
			continue;

		const FString dialogueName = FPaths::GetBaseFilename(entry.Key);

		// ExtractFilesFromZip stored the binary .mnteadlg as a temp file path
		const FString& tempPath = entry.Value;
		if (tempPath.IsEmpty() || !FPaths::FileExists(tempPath))
		{
			EditorLOG_WARNING(TEXT("[ProjectFactory] Temp file missing for '%s'"), *entry.Key);
			continue;
		}
		
		TArray<uint8> dialogueData;
		if (!FFileHelper::LoadFileToArray(dialogueData, *tempPath))
		{
			EditorLOG_WARNING(TEXT("[ProjectFactory] Failed to load nested archive: %s"), *entry.Key);
			IFileManager::Get().Delete(*tempPath);
			continue;
		}

		if (!UMounteaDialogueSystemImportExportHelpers::IsZipFile(dialogueData))
		{
			EditorLOG_WARNING(TEXT("[ProjectFactory] Nested file is not a valid archive: %s"), *entry.Key);
			IFileManager::Get().Delete(*tempPath);
			continue;
		}

		// Extract the nested .mnteadlg contents
		TMap<FString, FString> dialogueFiles;
		if (!UMounteaDialogueSystemImportExportHelpers::ExtractFilesFromZip(dialogueData, dialogueFiles))
		{
			EditorLOG_WARNING(TEXT("[ProjectFactory] Failed to extract nested archive: %s"), *entry.Key);
			IFileManager::Get().Delete(*tempPath);
			continue;
		}
		
		for (const FString& projectFile : projectLevelFiles)
		{
			if (!dialogueFiles.Contains(projectFile) && extractedFiles.Contains(projectFile))
				dialogueFiles.Add(projectFile, extractedFiles[projectFile]);
		}

		// Determine where a *new* graph would land if no existing one is found.
		// ImportDialogueGraphFromFiles first checks DialogueHistory via LookupExistingGraphByGuid;
		// if a previously-imported graph with the same GUID exists it will be reimported in-place
		// and this package will simply be GC'd. We must NOT pre-create the UMounteaDialogueGraph
		// here — passing a blank object (with a fresh random GUID) would bypass the GUID check and
		// always fall through to the "create new" branch, ignoring existing assets entirely.
		const FString dialoguePackagePath = FString::Printf(TEXT("%s/Dialogues"), *projectFolder);
		UPackage* dialogueParent = CreatePackage(*FString::Printf(TEXT("%s/%s"), *dialoguePackagePath, *dialogueName));

		UMounteaDialogueGraph* importedGraph = nullptr;

		FString importMessage;
		const bool bSuccess = UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraphFromFiles(
			dialogueFiles, Filename, dialogueParent, FName(*dialogueName), Flags, importedGraph, importMessage);

		if (bSuccess && importedGraph)
		{
			importedDialogues.Add(dialogueName);
			const FGuid graphGuid = importedGraph->GetGraphGUID();
			importedGraphsByGuid.Add(graphGuid, importedGraph);
			importedDialogueGuids.Add(graphGuid);

			// PopulateDialogueData already recorded the dialogue asset path and source.
			// Patch the source path to the .mnteadlgproj and mark as project import.
			if (importConfig)
			{
				const FString PackagePath = FPackageName::GetLongPackagePath(importedGraph->GetPathName());
				const FString GraphPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *importedGraph->GetName());
				importConfig->RecordDialogueImport(graphGuid, GraphPath, Filename, true);
				importConfig->SaveToFile();
			}
		}
		else
		{
			EditorLOG_WARNING(TEXT("[ProjectFactory] Failed to import dialogue '%s': %s"),
				*dialogueName, *importMessage);
		}

		IFileManager::Get().Delete(*tempPath);
	}

	// ── 5. Resolve cross-dialogue OpenChildGraph targets ──────────────────────
	// Each graph was imported independently so forward references to later-imported graphs
	// were left as pending GUIDs. Now that all graphs are in importedGraphsByGuid, resolve them.
	for (auto& graphPair : importedGraphsByGuid)
		UMounteaDialogueSystemImportExportHelpers::ResolveOpenChildGraphTargets(
			graphPair.Value, importedGraphsByGuid);

	// ── 5b. Record the project-level import (creates or updates ProjectHistory) ──
	if (importConfig && projectGuid.IsValid())
	{
		importConfig->RecordProjectImport(projectGuid, projectName, Filename,
			projectFolder, importedDialogueGuids);
		importConfig->SaveToFile();
	}

	// ── 5c. Import project-level thumbnails ────────────────────────────────────
	// Thumbnails/*.png live in the outer project archive (not in individual .mnteadlg files).
	// Import them into the Thumbnails folder under the resolved project root.
	UMounteaDialogueSystemImportExportHelpers::CreateTextureAssets(extractedFiles, projectFolder);

	// ── 6. Summary notification ────────────────────────────────────────────────
	UMounteaDialogueSystemImportExportHelpers::ShowNotification(
		FText::Format(
			LOCTEXT("ProjectImportSummary", "Project '{0}' imported: {1} dialogue(s)."),
			FText::FromString(projectName),
			FText::AsNumber(importedDialogues.Num())),
		5.f,
		importedDialogues.Num() > 0 ? TEXT("MDSStyleSet.Icon.Success") : TEXT("MDSStyleSet.Info.Error"));

	// Return nullptr — this factory intentionally creates multiple sub-assets rather than one.
	// Mark as canceled so UE does not show its generic "Failed to create asset" error dialog;
	// our own ShowNotification above already reported the outcome to the user.
	bOutOperationCanceled = true;
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
