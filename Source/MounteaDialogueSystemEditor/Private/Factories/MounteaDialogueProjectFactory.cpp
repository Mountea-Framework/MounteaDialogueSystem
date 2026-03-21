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
	if (extractedFiles.Contains(TEXT("projectData.json")))
	{
		TSharedPtr<FJsonObject> projectData;
		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(extractedFiles[TEXT("projectData.json")]);
		if (FJsonSerializer::Deserialize(reader, projectData) && projectData.IsValid())
			projectData->TryGetStringField(TEXT("projectName"), projectName);
	}

	const FString packageRoot = FPackageName::GetLongPackagePath(InParent->GetPathName());
	const FString projectFolder = FString::Printf(TEXT("%s/%s"), *packageRoot, *projectName);

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
	TMap<FGuid, UMounteaDialogueGraph*> importedGraphsByGuid; // for cross-dialogue OpenChildGraph resolution

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
			UE_LOG(LogTemp, Warning, TEXT("[ProjectFactory] Temp file missing for '%s'"), *entry.Key);
			continue;
		}

		// Load and validate the nested archive
		TArray<uint8> dialogueData;
		if (!FFileHelper::LoadFileToArray(dialogueData, *tempPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectFactory] Failed to load nested archive: %s"), *entry.Key);
			IFileManager::Get().Delete(*tempPath);
			continue;
		}

		if (!UMounteaDialogueSystemImportExportHelpers::IsZipFile(dialogueData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectFactory] Nested file is not a valid archive: %s"), *entry.Key);
			IFileManager::Get().Delete(*tempPath);
			continue;
		}

		// Extract the nested .mnteadlg contents
		TMap<FString, FString> dialogueFiles;
		if (!UMounteaDialogueSystemImportExportHelpers::ExtractFilesFromZip(dialogueData, dialogueFiles))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectFactory] Failed to extract nested archive: %s"), *entry.Key);
			IFileManager::Get().Delete(*tempPath);
			continue;
		}

		// Inject project-level files that the nested archive may not contain
		for (const FString& projectFile : projectLevelFiles)
		{
			if (!dialogueFiles.Contains(projectFile) && extractedFiles.Contains(projectFile))
				dialogueFiles.Add(projectFile, extractedFiles[projectFile]);
		}

		// Create the target package and a blank graph object, then import into it
		const FString dialoguePackagePath = FString::Printf(TEXT("%s/Dialogues"), *projectFolder);
		UPackage* dialogueParent = CreatePackage(*FString::Printf(TEXT("%s/%s"), *dialoguePackagePath, *dialogueName));

		UMounteaDialogueGraph* importedGraph = NewObject<UMounteaDialogueGraph>(
			dialogueParent, UMounteaDialogueGraph::StaticClass(), FName(*dialogueName), Flags | RF_Transactional);

		FString importMessage;
		const bool bSuccess = UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraphFromFiles(
			dialogueFiles, Filename, dialogueParent, FName(*dialogueName), Flags, importedGraph, importMessage);

		if (bSuccess && importedGraph)
		{
			importedDialogues.Add(dialogueName);
			importedGraphsByGuid.Add(importedGraph->GetGraphGUID(), importedGraph);

			// PopulateDialogueData (inside ImportDialogueGraphFromFiles) already wrote DialogueAssetPath,
			// DialogueSourcePath, and ImportedAt. We only need to mark this as a project import and
			// update the source path to point to the .mnteadlgproj (not the individual temp .mnteadlg).
			UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
			if (importConfig)
			{
				FDialogueImportSourceData& sourceData =
					importConfig->ImportHistory.FindOrAdd(importedGraph->GetGraphGUID());
				sourceData.DialogueSourcePath = Filename;
				sourceData.bIsProjectImport = true;

				importConfig->SaveConfig(CPF_Config, *importConfig->GetDefaultConfigFilename());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ProjectFactory] Failed to import dialogue '%s': %s"),
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

	// ── 5b. Import project-level thumbnails ────────────────────────────────────
	// Thumbnails/*.png live in the outer project archive (not in individual .mnteadlg files).
	// Import them into a shared thumbnails folder under the project root.
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
