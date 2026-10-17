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

#include "Archive/MounteaDialogueArchive.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

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
	// The project is one v3 archive (kind "project"): every dialogue, definition and participant in a single file.
	MounteaArchive::FDialogueArchive archive;
	FString readMessage;
	if (!UMounteaDialogueSystemImportExportHelpers::ReadArchiveFile(Filename, archive, readMessage))
	{
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
			FText::Format(LOCTEXT("ReadFailed", "Failed to read project file:\n{0}\n{1}"), FText::FromString(Filename), FText::FromString(readMessage)),
			8.f, TEXT("MDSStyleSet.Info.Error"));
		bOutOperationCanceled = true;
		return nullptr;
	}

	const FString projectName = archive.Project.Name.IsEmpty() ? InName.ToString() : archive.Project.Name;
	const FGuid projectGuid = MounteaArchive::IdToGuid(archive.Project.Id);

	// Determine the project folder: reuse the stored one on reimport (prevents duplicate folders),
	// fall back to computing a new folder from the current drop location.
	FString projectFolder;
	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
	if (!(importConfig && projectGuid.IsValid() && importConfig->LookupProjectFolder(projectGuid, projectFolder)))
	{
		const FString packageRoot = FPackageName::GetLongPackagePath(InParent->GetPathName());
		projectFolder = FString::Printf(TEXT("%s/%s"), *packageRoot, *projectName);
	}

	if (GEditor)
	{
		const TArray<UMounteaDialogueGraph*> affectedGraphs =
			UMounteaDialogueSystemImportExportHelpers::FindGraphsToBeReimported(archive);

		TArray<UMounteaDialogueGraph*> openGraphs;
		if (UAssetEditorSubsystem* assetEditors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			for (UMounteaDialogueGraph* graph : affectedGraphs)
				if (assetEditors->FindEditorForAsset(graph, false))
					openGraphs.Add(graph);
		}

		if (!openGraphs.IsEmpty())
		{
			FString graphNames;
			for (UMounteaDialogueGraph* graph : openGraphs)
				graphNames += FString::Printf(TEXT("\n  • %s"), *graph->GetName());

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

			if (UAssetEditorSubsystem* assetEditors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
				for (UMounteaDialogueGraph* graph : openGraphs)
					assetEditors->CloseAllEditorsForAsset(graph);
		}
	}

	FMounteaArchiveImportRequest request;
	request.SourceFilePath = Filename;
	request.TargetFolder = projectFolder;
	request.DialoguesFolder = FString::Printf(TEXT("%s/Dialogues"), *projectFolder);
	request.bProjectImport = true;
	request.Flags = Flags;

	TMap<FGuid, UMounteaDialogueGraph*> importedGraphsByGuid;
	UMounteaDialogueGraph* rootGraph = nullptr;
	FString importMessage;
	const bool bSuccess = UMounteaDialogueSystemImportExportHelpers::ImportArchiveModel(archive, request, importedGraphsByGuid, rootGraph, importMessage);

	if (!bSuccess)
		EditorLOG_WARNING(TEXT("[ProjectFactory] Import of '%s' reported problems: %s"), *projectName, *importMessage);

	// Record the project-level import (creates or updates ProjectHistory).
	if (importConfig && projectGuid.IsValid())
	{
		TArray<FGuid> importedDialogueGuids;
		importedGraphsByGuid.GenerateKeyArray(importedDialogueGuids);
		importConfig->RecordProjectImport(projectGuid, projectName, Filename, projectFolder, importedDialogueGuids);
		importConfig->SaveToFile();
	}

	UMounteaDialogueSystemImportExportHelpers::ShowNotification(
		FText::Format(
			LOCTEXT("ProjectImportSummary", "Project '{0}' imported: {1} dialogue(s)."),
			FText::FromString(projectName),
			FText::AsNumber(importedGraphsByGuid.Num())),
		5.f,
		importedGraphsByGuid.Num() > 0 ? TEXT("MDSStyleSet.Icon.Success") : TEXT("MDSStyleSet.Info.Error"));

	// Return nullptr: this factory intentionally creates multiple sub-assets rather than one.
	// Mark as canceled so UE does not show its generic "Failed to create asset" error dialog;
	// our own ShowNotification above already reported the outcome to the user.
	bOutOperationCanceled = true;
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
