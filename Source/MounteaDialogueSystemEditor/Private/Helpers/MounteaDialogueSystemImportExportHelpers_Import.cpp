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

// Import half of the archive pipeline: mountea.archive.v3 model -> assets.
// The format itself (parsing, validation, ids) lives in Archive/MounteaDialogueArchive.*.

#include "MounteaDialogueSystemImportExportHelpers.h"

#include "Archive/MounteaDialogueArchive.h"
#include "Archive/MounteaDialogueArchiveZip.h"

#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AudioEditorModule.h"
#include "Conditions/MounteaDialogueConditionBase.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueEngineCompat.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "ImportConfig/MounteaDialogueImportConfig.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/TextLocalizationResource.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"

namespace
{
	/** JSON object keys are FString up to UE 5.7 and a shared-string type from 5.8. */
	template <typename KeyType>
	FString JsonKeyToString(const KeyType& Key)
	{
		if constexpr (std::is_convertible_v<const KeyType&, FString>)
			return FString(Key);
		else
			return FString(Key.ToView());
	}

	FString SanitizeAssetName(const FString& Raw, const TCHAR* Fallback)
	{
		FString result;
		for (const TCHAR character : Raw)
		{
			const bool bSafe = (character >= TEXT('0') && character <= TEXT('9')) || (character >= TEXT('A') && character <= TEXT('Z'))
				|| (character >= TEXT('a') && character <= TEXT('z')) || character == TEXT('_') || character == TEXT('-');
			result.AppendChar(bSafe ? character : TEXT('_'));
		}
		return result.IsEmpty() ? FString(Fallback) : result;
	}

	/** Typed JSON scalar -> the text form UProperty::ImportText understands. */
	bool JsonScalarToText(const TSharedPtr<FJsonValue>& Value, FString& OutText)
	{
		if (!Value.IsValid())
			return false;

		switch (Value->Type)
		{
			case EJson::String: OutText = Value->AsString(); return true;
			case EJson::Number: OutText = FString::SanitizeFloat(Value->AsNumber()); return true;
			case EJson::Boolean: OutText = Value->AsBool() ? TEXT("true") : TEXT("false"); return true;
			default: return false;
		}
	}

	/** Applies decorator / condition / rule values (keyed by property name) to an instance. */
	void ApplyJsonValues(UObject* Target, const TSharedPtr<FJsonObject>& Values)
	{
		if (!Target || !Values.IsValid())
			return;

		for (const auto& pair : Values->Values)
		{
			FProperty* property = Target->GetClass()->FindPropertyByName(FName(*JsonKeyToString(pair.Key)));
			if (!property)
				continue;

			FString valueText;
			if (JsonScalarToText(pair.Value, valueText))
				property->ImportText_InContainer(*valueText, Target, Target, PPF_None);
		}
	}

	FString TempFileFor(const FString& Prefix, const FString& Extension, const TArray<uint8>& Bytes)
	{
		const FString path = FPaths::CreateTempFilename(FPlatformProcess::UserTempDir(), *Prefix, *Extension);
		return FFileHelper::SaveArrayToFile(Bytes, *path) ? path : FString();
	}

	FString AudioExtension(const MounteaArchive::FMedia& Media)
	{
		const FString extension = FPaths::GetExtension(Media.Name).ToLower();
		if (extension == TEXT("wav") || extension == TEXT("mp3") || extension == TEXT("ogg") || extension == TEXT("flac"))
			return TEXT(".") + extension;
		if (Media.MimeType.Contains(TEXT("mpeg")) || Media.MimeType.Contains(TEXT("mp3")))
			return TEXT(".mp3");
		if (Media.MimeType.Contains(TEXT("ogg")))
			return TEXT(".ogg");
		if (Media.MimeType.Contains(TEXT("flac")))
			return TEXT(".flac");
		return TEXT(".wav");
	}

	/** Default-locale text of an archive string key. Empty when absent. */
	FString DefaultText(const MounteaArchive::FDialogueArchive& Archive, const FString& Key)
	{
		if (Key.IsEmpty())
			return FString();
		const MounteaArchive::FLocalizedString* entry = Archive.FindString(Key);
		return entry ? entry->Values.FindRef(Archive.Project.Localization.DefaultLocale) : FString();
	}

	FString RootDialogueIdOf(const MounteaArchive::FDialogueArchive& Archive)
	{
		if (Archive.Kind == MounteaArchive::EKind::Dialogue && !Archive.RootDialogueId.IsEmpty())
			return Archive.RootDialogueId;
		return Archive.Dialogues.IsEmpty() ? FString() : Archive.Dialogues[0].Id;
	}
}

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemImportExportHelpers"

// ---------------------------------------------------------------------------------------------
// Entry points
// ---------------------------------------------------------------------------------------------

bool UMounteaDialogueSystemImportExportHelpers::ReadArchiveFile(const FString& FilePath, MounteaArchive::FDialogueArchive& OutArchive, FString& OutMessage)
{
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *FilePath))
	{
		OutMessage = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReadArchiveFile] %s"), *OutMessage);
		return false;
	}

	TArray<FString> errors;
	if (!MounteaArchive::ReadArchiveFromBytes(fileData, OutArchive, errors))
	{
		OutMessage = FString::Join(errors, TEXT("\n"));
		EditorLOG_ERROR(TEXT("[ReadArchiveFile] %s: %s"), *FilePath, *OutMessage);
		return false;
	}
	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::IsReimport(const FString& Filename)
{
	if (Filename.IsEmpty())
		return false;

	MounteaArchive::FDialogueArchive archive;
	FString message;
	if (!ReadArchiveFile(Filename, archive, message))
		return false;

	const FString rootId = RootDialogueIdOf(archive);
	if (rootId.IsEmpty())
		return false;

	const UMounteaDialogueImportConfig* importConfig = GetDefault<UMounteaDialogueImportConfig>();
	return importConfig && importConfig->IsDialogueImported(MounteaArchive::IdToGuid(rootId));
}

bool UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(const FString& FilePath, UObject* ObjectRedirector, UMounteaDialogueGraph*& OutGraph, FString& OutMessage)
{
	if (FilePath.IsEmpty())
	{
		OutMessage = TEXT("Source file path is empty");
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	MounteaArchive::FDialogueArchive archive;
	if (!ReadArchiveFile(FilePath, archive, OutMessage))
		return false;

	UObject* parent = OutGraph ? OutGraph->GetOuter() : ObjectRedirector;
	if (!parent)
	{
		OutMessage = TEXT("No asset to reimport into");
		return false;
	}

	FMounteaArchiveImportRequest request;
	request.SourceFilePath = FilePath;
	request.RootParent = parent;
	request.RootName = OutGraph ? FName(*OutGraph->GetName()) : NAME_None;
	request.Flags = OutGraph ? OutGraph->GetFlags() : RF_Public | RF_Standalone | RF_Transactional;
	request.ExistingRoot = OutGraph;
	request.bProjectImport = archive.Kind == MounteaArchive::EKind::Project;
	request.TargetFolder = FPackageName::GetLongPackagePath(parent->GetPathName());

	// A project-sourced graph is refreshed from the whole project, placed where the project went last time.
	if (request.bProjectImport)
	{
		FString projectFolder;
		UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
		if (importConfig && importConfig->LookupProjectFolder(MounteaArchive::IdToGuid(archive.Project.Id), projectFolder))
		{
			request.TargetFolder = projectFolder;
			request.DialoguesFolder = projectFolder / TEXT("Dialogues");
		}
	}

	TMap<FGuid, UMounteaDialogueGraph*> graphs;
	UMounteaDialogueGraph* root = nullptr;
	if (!ImportArchiveModel(archive, request, graphs, root, OutMessage))
		return false;

	if (OutGraph)
	{
		// Reimport from a project file: hand back the graph that was actually refreshed.
		UMounteaDialogueGraph* const* same = graphs.Find(OutGraph->GetGraphGUID());
		if (same && *same)
			OutGraph = *same;
	}
	else
		OutGraph = root;
	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraph(const FString& FilePath, UObject* InParent, const FName Name, const EObjectFlags Flags, UMounteaDialogueGraph*& OutGraph, FString& OutMessage)
{
	MounteaArchive::FDialogueArchive archive;
	if (!ReadArchiveFile(FilePath, archive, OutMessage))
		return false;

	if (!InParent)
	{
		OutMessage = TEXT("No package to import into");
		return false;
	}

	FMounteaArchiveImportRequest request;
	request.SourceFilePath = FilePath;
	request.RootParent = InParent;
	request.RootName = Name;
	request.Flags = Flags;
	request.ExistingRoot = OutGraph;
	request.bProjectImport = false;
	request.TargetFolder = FPackageName::GetLongPackagePath(InParent->GetPathName());

	TMap<FGuid, UMounteaDialogueGraph*> graphs;
	UMounteaDialogueGraph* root = nullptr;
	if (!ImportArchiveModel(archive, request, graphs, root, OutMessage))
		return false;

	OutGraph = root;
	return true;
}

TArray<UMounteaDialogueGraph*> UMounteaDialogueSystemImportExportHelpers::FindGraphsToBeReimported(const MounteaArchive::FDialogueArchive& Archive)
{
	TArray<UMounteaDialogueGraph*> result;
	for (const MounteaArchive::FDialogue& dialogue : Archive.Dialogues)
	{
		if (UMounteaDialogueGraph* existing = LookupExistingGraphByGuid(MounteaArchive::IdToGuid(dialogue.Id)))
			result.AddUnique(existing);
	}
	return result;
}

// ---------------------------------------------------------------------------------------------
// Core import
// ---------------------------------------------------------------------------------------------

bool UMounteaDialogueSystemImportExportHelpers::ImportArchiveModel(const MounteaArchive::FDialogueArchive& Archive, const FMounteaArchiveImportRequest& Request,
	TMap<FGuid, UMounteaDialogueGraph*>& OutGraphs, UMounteaDialogueGraph*& OutRoot, FString& OutMessage)
{
	OutGraphs.Reset();
	OutRoot = nullptr;

	if (Archive.Dialogues.IsEmpty())
	{
		OutMessage = TEXT("The archive contains no dialogue.");
		EditorLOG_ERROR(TEXT("[ImportArchiveModel] %s"), *OutMessage);
		return false;
	}

	FMounteaArchiveImportContext context;
	context.Archive = &Archive;
	context.Request = Request;
	if (context.Request.DialoguesFolder.IsEmpty())
		context.Request.DialoguesFolder = context.Request.TargetFolder;

	const FGuid rootGuid = MounteaArchive::IdToGuid(RootDialogueIdOf(Archive));

	// A single-file reimport keeps its shared assets (Decorators, Conditions, thumbnails) next to the graph that
	// already exists, wherever the file was dropped this time. Projects keep the folder chosen at first import.
	if (!Request.bProjectImport)
	{
		const UMounteaDialogueGraph* existingRoot = Request.ExistingRoot ? Request.ExistingRoot : LookupExistingGraphByGuid(rootGuid);
		if (existingRoot)
		{
			const FString existingFolder = FPackageName::GetLongPackagePath(existingRoot->GetOutermost()->GetName());
			context.Request.TargetFolder = existingFolder;
			context.Request.DialoguesFolder = existingFolder;
		}
	}

	// Shared assets first: they are referenced by every dialogue.
	context.DecoratorClasses = CreateDefinitionClasses(Archive.Decorators, context.Request.TargetFolder / TEXT("Decorators"), true);
	context.ConditionClasses = CreateDefinitionClasses(Archive.Conditions, context.Request.TargetFolder / TEXT("Conditions"), false);
	context.ParticipantTags = BuildParticipantTags(Archive);

	TArray<FString> tags;
	context.ParticipantTags.GenerateValueArray(tags);
	RegisterParticipantTags(tags);
	ImportParticipantThumbnails(Archive, context.Request.TargetFolder);

	TArray<FString> failures;
	for (const MounteaArchive::FDialogue& dialogue : Archive.Dialogues)
	{
		const FGuid dialogueGuid = MounteaArchive::IdToGuid(dialogue.Id);
		const bool bIsRoot = dialogueGuid == rootGuid;
		const FString assetName = SanitizeAssetName(dialogue.Name, TEXT("Dialogue"));

		UMounteaDialogueGraph* graph = nullptr;
		if (bIsRoot && Request.ExistingRoot)
			graph = Request.ExistingRoot;
		else
			graph = LookupExistingGraphByGuid(dialogueGuid);

		if (graph)
		{
			// Assets found through the asset registry may be partially loaded; saving them then crashes.
			if (UPackage* existingPackage = graph->GetOutermost())
				existingPackage->FullyLoad();
			graph->ClearGraph();
		}
		else
		{
			UObject* parent = (bIsRoot && Request.RootParent)
				? Request.RootParent
				: CreatePackage(*(context.Request.DialoguesFolder / assetName));
			const FName name = (bIsRoot && Request.RootParent && Request.RootName != NAME_None && dialogue.Name.IsEmpty()) ? Request.RootName : FName(*assetName);
			graph = NewObject<UMounteaDialogueGraph>(parent, UMounteaDialogueGraph::StaticClass(), name, Request.Flags | RF_Transactional);
		}

		if (!graph)
		{
			failures.Add(FString::Printf(TEXT("Failed to create graph for dialogue '%s'"), *dialogue.Name));
			continue;
		}

		if (!PopulateGraphFromArchive(graph, context, dialogue))
		{
			failures.Add(FString::Printf(TEXT("Failed to populate dialogue '%s'"), *dialogue.Name));
			continue;
		}

		OutGraphs.Add(dialogueGuid, graph);
		if (bIsRoot)
			OutRoot = graph;
	}

	// Child graph targets can point at dialogues imported later in the loop.
	for (const TPair<FGuid, UMounteaDialogueGraph*>& pair : OutGraphs)
		ResolveOpenChildGraphTargets(pair.Value, OutGraphs);

	if (!OutRoot && !OutGraphs.IsEmpty())
	{
		OutRoot = OutGraphs.CreateConstIterator().Value();
	}

	if (!failures.IsEmpty())
	{
		OutMessage = FString::Join(failures, TEXT("\n"));
		EditorLOG_ERROR(TEXT("[ImportArchiveModel] %s"), *OutMessage);
		return OutGraphs.Num() > 0 && failures.Num() == 0;
	}

	OutMessage = FString::Printf(TEXT("%d dialogue(s) imported."), OutGraphs.Num());
	return true;
}

// ---------------------------------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------------------------------

TMap<FGuid, UClass*> UMounteaDialogueSystemImportExportHelpers::CreateDefinitionClasses(const TArray<MounteaArchive::FDefinition>& Definitions, const FString& PackagePath, const bool bDecorators)
{
	TMap<FGuid, UClass*> result;

	for (const MounteaArchive::FDefinition& definition : Definitions)
	{
		const FGuid definitionGuid = MounteaArchive::IdToGuid(definition.Id);
		if (result.Contains(definitionGuid))
			continue;

		const FString assetName = SanitizeAssetName(definition.Name, TEXT("Definition"));

		UClass* existingClass = bDecorators
			? FindExistingDecoratorClass(definitionGuid, assetName, PackagePath)
			: FindExistingConditionClass(definitionGuid, assetName, PackagePath);
		if (existingClass)
		{
			result.Add(definitionGuid, existingClass);
			continue;
		}

		UPackage* package = CreatePackage(*(PackagePath / assetName));
		if (!package)
			continue;

		UClass* parentClass = bDecorators ? UMounteaDialogueDecoratorBase::StaticClass() : UMounteaDialogueConditionBase::StaticClass();
		UBlueprint* blueprint = FKismetEditorUtilities::CreateBlueprint(parentClass, package, FName(*assetName), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
		if (!blueprint || !blueprint->GeneratedClass)
			continue;

		for (const MounteaArchive::FPropertyDef& property : definition.Properties)
		{
			FEdGraphPinType pinType;
			if (property.Type == MounteaArchive::Contract::PropertyString)
				pinType.PinCategory = UEdGraphSchema_K2::PC_String;
			else if (property.Type == MounteaArchive::Contract::PropertyBoolean)
				pinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			else if (property.Type == MounteaArchive::Contract::PropertyNumber)
			{
				pinType.PinCategory = UEdGraphSchema_K2::PC_Real;
				pinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
			}
			else
				continue;

			FBlueprintEditorUtils::AddMemberVariable(blueprint, FName(*property.Name), pinType, property.DefaultValue);
		}

		FKismetEditorUtilities::CompileBlueprint(blueprint);

		if (bDecorators)
		{
			if (UMounteaDialogueDecoratorBase* cdo = blueprint->GeneratedClass->GetDefaultObject<UMounteaDialogueDecoratorBase>())
			{
				cdo->DecoratorGUID = definitionGuid;
				cdo->DecoratorName = FText::FromString(definition.Name);
			}
		}
		else if (UMounteaDialogueConditionBase* cdo = blueprint->GeneratedClass->GetDefaultObject<UMounteaDialogueConditionBase>())
		{
			cdo->SetConditionGUID(definitionGuid);
			cdo->ConditionName = FName(*definition.Name);
		}

		FAssetRegistryModule::AssetCreated(blueprint);
		blueprint->MarkPackageDirty();
		SaveAsset(blueprint);

		result.Add(definitionGuid, blueprint->GeneratedClass);
	}

	return result;
}

// ---------------------------------------------------------------------------------------------
// Participants and gameplay tags
// ---------------------------------------------------------------------------------------------

TMap<FString, FString> UMounteaDialogueSystemImportExportHelpers::BuildParticipantTags(const MounteaArchive::FDialogueArchive& Archive)
{
	TMap<FString, FString> result;

	for (const MounteaArchive::FParticipant& participant : Archive.Participants)
	{
		TArray<FString> chain;
		TSet<FString> seen;
		const MounteaArchive::FCategory* category = Archive.FindCategory(participant.CategoryId);
		while (category && !seen.Contains(category->Id))
		{
			seen.Add(category->Id);
			chain.Insert(category->Name, 0);
			category = category->ParentId.IsEmpty() ? nullptr : Archive.FindCategory(category->ParentId);
		}

		chain.Add(participant.Name);
		result.Add(participant.Id, TEXT("Mountea_Dialogue.") + FString::Join(chain, TEXT(".")));
	}
	return result;
}

void UMounteaDialogueSystemImportExportHelpers::RegisterParticipantTags(const TArray<FString>& FullTags)
{
	if (FullTags.IsEmpty())
		return;

	// Project config, not the plugin folder: survives plugin updates and stays per-project.
	// Own filename: tag source name is the bare filename, so reusing the plugin's would collide with it.
	const FString tagsDir = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Tags"));
	const FString tagsIniPath = FPaths::Combine(tagsDir, TEXT("MounteaDialogueParticipants.ini"));
	IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*tagsDir);

	FString fileContent;
	if (FPaths::FileExists(tagsIniPath))
		FFileHelper::LoadFileToString(fileContent, *tagsIniPath);

	const FString sectionHeader = TEXT("[/Script/GameplayTags.GameplayTagsList]");
	if (!fileContent.Contains(sectionHeader))
	{
		if (!fileContent.IsEmpty() && !fileContent.EndsWith(TEXT("\n")))
			fileContent.AppendChar(TEXT('\n'));
		fileContent.Append(sectionHeader);
		fileContent.AppendChar(TEXT('\n'));
	}

	// Collect existing tags to avoid duplicates
	TSet<FString> existingTags;
	{
		TArray<FString> iniLines;
		fileContent.ParseIntoArrayLines(iniLines);
		for (const FString& line : iniLines)
		{
			if (!line.StartsWith(TEXT("GameplayTagList=")) && !line.StartsWith(TEXT("+GameplayTagList=")))
				continue;
			int32 tagStart = line.Find(TEXT("Tag=\""));
			if (tagStart == INDEX_NONE)
				continue;
			tagStart += 5;
			const int32 tagEnd = line.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, tagStart);
			if (tagEnd != INDEX_NONE)
				existingTags.Add(line.Mid(tagStart, tagEnd - tagStart));
		}
	}

	bool bAnyAdded = false;
	for (const FString& fullTag : FullTags)
	{
		if (existingTags.Contains(fullTag))
			continue;

		if (!fileContent.EndsWith(TEXT("\n")))
			fileContent.AppendChar(TEXT('\n'));
		fileContent.Append(FString::Printf(TEXT("GameplayTagList=(Tag=\"%s\",DevComment=\"Mountea Dialogue Participant\")\n"), *fullTag));
		existingTags.Add(fullTag);
		bAnyAdded = true;
	}

	if (bAnyAdded)
	{
		FFileHelper::SaveStringToFile(fileContent, *tagsIniPath);
		UGameplayTagsManager::Get().EditorRefreshGameplayTagTree();
	}
}

void UMounteaDialogueSystemImportExportHelpers::ImportParticipantThumbnails(const MounteaArchive::FDialogueArchive& Archive, const FString& TargetFolder)
{
	FAssetToolsModule& assetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& assetTools = assetToolsModule.Get();
	FAssetRegistryModule& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	const FString thumbnailsPath = TargetFolder / TEXT("thumbnails");

	for (const MounteaArchive::FParticipant& participant : Archive.Participants)
	{
		if (!participant.bHasThumbnail || !participant.Thumbnail.IsSet())
			continue;

		const FString assetName = participant.Name;

		// Re-use existing asset if already imported; keeps any manual tweaks.
		const FSoftObjectPath existingPath(thumbnailsPath / assetName + TEXT(".") + assetName);
		if (assetRegistry.Get().GetAssetByObjectPath(existingPath).IsValid())
			continue;

		const FString tempPath = TempFileFor(TEXT("MounteaThumbnail"), TEXT(".png"), participant.Thumbnail.Bytes);
		if (tempPath.IsEmpty())
		{
			EditorLOG_WARNING(TEXT("[ImportParticipantThumbnails] Could not write temp file for '%s'"), *assetName);
			continue;
		}

		UAssetImportTask* importTask = NewObject<UAssetImportTask>();
		importTask->Filename = tempPath;
		importTask->DestinationPath = thumbnailsPath;
		importTask->DestinationName = assetName;
		importTask->bReplaceExisting = true;
		importTask->bSave = true;

		TArray<UAssetImportTask*> tasks = { importTask };
		assetTools.ImportAssetTasks(tasks);

		if (importTask->GetObjects().Num() == 0)
			EditorLOG_WARNING(TEXT("[ImportParticipantThumbnails] Failed to import thumbnail of '%s'"), *assetName);

		IFileManager::Get().Delete(*tempPath);
	}
}

// ---------------------------------------------------------------------------------------------
// One dialogue
// ---------------------------------------------------------------------------------------------

bool UMounteaDialogueSystemImportExportHelpers::PopulateGraphFromArchive(UMounteaDialogueGraph* Graph, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue)
{
	if (!IsValid(Graph) || !Context.Archive)
		return false;

	const FGuid dialogueGuid = MounteaArchive::IdToGuid(Dialogue.Id);
	Graph->SetGraphGUID(dialogueGuid);
	Graph->SourceFile = Context.Request.SourceFilePath;
	Graph->SourceData.Empty();

	if (UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>())
	{
		const FString graphPath = FString::Printf(TEXT("%s/%s"), *FPackageName::GetLongPackagePath(Graph->GetPathName()), *Graph->GetName());
		importConfig->RecordDialogueImport(dialogueGuid, graphPath, Context.Request.SourceFilePath, Context.Request.bProjectImport);
		importConfig->SaveToFile();
	}

	FAssetToolsModule& assetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& assetTools = assetToolsModule.Get();

	UStringTable* stDialogueRows = nullptr;
	UStringTable* stNodes = nullptr;
	if (!CreateGraphStringTables(Graph, assetTools, Context, Dialogue, stDialogueRows, stNodes))
		return false;

	UDataTable* dtParticipants = nullptr;
	UDataTable* dtDialogueRows = nullptr;
	if (!CreateGraphDataTables(Graph, assetTools, dtParticipants, dtDialogueRows))
		return false;

	const TMap<FGuid, USoundWave*> audioMap = ImportRowAudio(*Context.Archive, Dialogue, Graph->GetOuter());

	TMap<FGuid, UMounteaDialogueGraphNode*> spawnedNodes;
	if (!PopulateNodes(Graph, Context, Dialogue, spawnedNodes, stNodes))
		return false;
	if (!PopulateEdges(Graph, Context, Dialogue))
		return false;
	ResolveReturnToNodeTargets(Graph, spawnedNodes);

	if (!FillParticipantsDataTable(Graph, dtParticipants, Context, Dialogue))
		return false;
	if (!FillDialogueRowsDataTable(Graph, dtDialogueRows, dtParticipants, stDialogueRows, stNodes, Context, Dialogue, audioMap))
		return false;

	// Visual graph is rebuilt from the runtime nodes next time the editor opens it.
	Graph->CreateGraph();
	if (UEdGraph_MounteaDialogueGraph* edGraph = Cast<UEdGraph_MounteaDialogueGraph>(Graph->EdGraph))
		edGraph->Nodes.Empty();

	SaveAsset(Graph);
	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::CreateGraphStringTables(UMounteaDialogueGraph* Graph, IAssetTools& AssetTools, const FMounteaArchiveImportContext& Context,
	const MounteaArchive::FDialogue& Dialogue, UStringTable*& OutDialogueRowsStringTable, UStringTable*& OutNodesStringTable)
{
	const MounteaArchive::FDialogueArchive& archive = *Context.Archive;
	const FString packagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString rowsTableName = FString::Printf(TEXT("ST_%s_DialogueRows"), *Graph->GetName());
	const FString nodesTableName = FString::Printf(TEXT("ST_%s_Nodes"), *Graph->GetName());
	const FString& defaultLocale = archive.Project.Localization.DefaultLocale;

	// Keys per table, with every locale the archive carries, for the .po files.
	TMap<FString, TMap<FString, FString>> rowTexts;
	TMap<FString, TMap<FString, FString>> nodeTexts;
	for (const MounteaArchive::FNode& node : archive.Nodes)
	{
		if (node.DialogueId != Dialogue.Id)
			continue;

		for (const MounteaArchive::FRow& row : node.Rows)
		{
			if (const MounteaArchive::FLocalizedString* entry = archive.FindString(row.TextKey))
				rowTexts.Add(row.TextKey, entry->Values);
		}
		for (const FString& key : { node.DisplayNameKey, node.SelectionTitleKey })
		{
			if (const MounteaArchive::FLocalizedString* entry = key.IsEmpty() ? nullptr : archive.FindString(key))
				nodeTexts.Add(key, entry->Values);
		}
	}

	OutDialogueRowsStringTable = CreateStringTable(AssetTools, packagePath, rowsTableName, [&](UStringTable* table)
	{
		for (const TPair<FString, TMap<FString, FString>>& pair : rowTexts)
			MounteaDialogueCompat::SetStringTableSourceString(*table->GetMutableStringTable(), pair.Key, pair.Value.FindRef(defaultLocale));
	});

	OutNodesStringTable = CreateStringTable(AssetTools, packagePath, nodesTableName, [&](UStringTable* table)
	{
		for (const TPair<FString, TMap<FString, FString>>& pair : nodeTexts)
			MounteaDialogueCompat::SetStringTableSourceString(*table->GetMutableStringTable(), pair.Key, pair.Value.FindRef(defaultLocale));
	});

	if (!OutDialogueRowsStringTable || !OutNodesStringTable)
	{
		EditorLOG_ERROR(TEXT("[CreateGraphStringTables] Failed to create one or more string tables for '%s'"), *Graph->GetName());
		return false;
	}

	SaveAsset(OutDialogueRowsStringTable);
	SaveAsset(OutNodesStringTable);

	WriteLocalizationPoFiles(rowsTableName, defaultLocale, rowTexts);
	WriteLocalizationPoFiles(nodesTableName, defaultLocale, nodeTexts);

	WriteLocalizationResources(rowsTableName, defaultLocale, rowTexts);
	WriteLocalizationResources(nodesTableName, defaultLocale, nodeTexts);
	RegisterLocalizationPath();
	return true;
}

void UMounteaDialogueSystemImportExportHelpers::WriteLocalizationPoFiles(const FString& StringTableID, const FString& DefaultLocale, const TMap<FString, TMap<FString, FString>>& TextsByKeyAndLocale)
{
	TSet<FString> locales;
	for (const TPair<FString, TMap<FString, FString>>& entry : TextsByKeyAndLocale)
	{
		for (const TPair<FString, FString>& localePair : entry.Value)
			locales.Add(localePair.Key);
	}
	locales.Remove(DefaultLocale);

	// Same escaping Unreal's PortableObjectPipeline::ConditionArchiveStrForPO applies.
	auto escapePo = [](const FString& Input) -> FString
	{
		FString output = Input;
		output.ReplaceInline(TEXT("\\"), TEXT("\\\\"), ESearchCase::CaseSensitive);
		output.ReplaceInline(TEXT("\""), TEXT("\\\""), ESearchCase::CaseSensitive);
		output.ReplaceInline(TEXT("\r"), TEXT("\\r"), ESearchCase::CaseSensitive);
		output.ReplaceInline(TEXT("\n"), TEXT("\\n"), ESearchCase::CaseSensitive);
		output.ReplaceInline(TEXT("\t"), TEXT("\\t"), ESearchCase::CaseSensitive);
		return output;
	};

	// Unreal identity of an entry: "<Namespace>,<Key>" with commas inside either part escaped
	// (PortableObjectPipeline::ConditionIdentityForPO), so the Localization Dashboard can import the file.
	auto identity = [&](const FString& Key) -> FString
	{
		FString escapedNamespace = StringTableID;
		escapedNamespace.ReplaceInline(TEXT(","), TEXT("\\,"), ESearchCase::CaseSensitive);
		FString escapedKey = Key;
		escapedKey.ReplaceInline(TEXT(","), TEXT("\\,"), ESearchCase::CaseSensitive);
		return escapePo(escapedNamespace + TEXT(",") + escapedKey);
	};

	for (const FString& locale : locales)
	{
		FString poContent;
		poContent.Append(FString::Printf(TEXT("# Mountea Dialogue localization - %s (%s)\n"), *StringTableID, *locale));
		poContent.Append(TEXT("msgid \"\"\n"));
		poContent.Append(TEXT("msgstr \"\"\n"));
		poContent.Append(TEXT("\"Content-Type: text/plain; charset=UTF-8\\n\"\n"));
		poContent.Append(TEXT("\"Content-Transfer-Encoding: 8bit\\n\"\n"));
		poContent.Append(FString::Printf(TEXT("\"Language: %s\\n\"\n\n"), *locale));

		for (const TPair<FString, TMap<FString, FString>>& entry : TextsByKeyAndLocale)
		{
			poContent.Append(FString::Printf(TEXT("msgctxt \"%s\"\n"), *identity(entry.Key)));
			poContent.Append(FString::Printf(TEXT("msgid \"%s\"\n"), *escapePo(entry.Value.FindRef(DefaultLocale))));
			poContent.Append(FString::Printf(TEXT("msgstr \"%s\"\n\n"), *escapePo(entry.Value.FindRef(locale))));
		}

		// {ProjectDir}/Content/Localization/{StringTableID}/{Locale}/{StringTableID}.po
		const FString locDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content"), TEXT("Localization"), StringTableID, locale);
		IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*locDir);

		const FString poPath = FPaths::Combine(locDir, StringTableID + TEXT(".po"));
		if (!FFileHelper::SaveStringToFile(poContent, *poPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
			EditorLOG_WARNING(TEXT("[WriteLocalizationPoFiles] Failed to write .po file: %s"), *poPath);
	}
}

void UMounteaDialogueSystemImportExportHelpers::WriteLocalizationResources(const FString& StringTableID, const FString& DefaultLocale, const TMap<FString, TMap<FString, FString>>& TextsByKeyAndLocale)
{
	if (TextsByKeyAndLocale.IsEmpty())
		return;

	const FString targetRoot = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Localization"), TEXT("MounteaDialogue"));

	TSet<FString> locales;
	locales.Add(DefaultLocale);
	for (const TPair<FString, TMap<FString, FString>>& entry : TextsByKeyAndLocale)
	{
		for (const TPair<FString, FString>& localePair : entry.Value)
			locales.Add(localePair.Key);
	}

	const FTextKey tableNamespace(StringTableID);

	for (const FString& locale : locales)
	{
		const FString locresPath = FPaths::Combine(targetRoot, locale, TEXT("MounteaDialogue.locres"));
		IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*FPaths::GetPath(locresPath));

		// One file per culture holds every Mountea table; only this table's entries are replaced.
		FTextLocalizationResource resource;
		if (FPaths::FileExists(locresPath))
			resource.LoadFromFile(locresPath, 0);

		for (auto it = resource.Entries.CreateIterator(); it; ++it)
		{
			if (it.Key().GetNamespace() == tableNamespace)
				it.RemoveCurrent();
		}

		for (const TPair<FString, TMap<FString, FString>>& entry : TextsByKeyAndLocale)
		{
			const FString source = entry.Value.FindRef(DefaultLocale);
			const FString* translated = entry.Value.Find(locale);
			// A missing translation is left out so the text falls back to the string table's source.
			if (locale != DefaultLocale && (!translated || translated->IsEmpty()))
				continue;

			resource.AddEntry(tableNamespace, FTextKey(entry.Key), source, locale == DefaultLocale ? source : *translated, 0);
		}

		if (!resource.SaveToFile(locresPath))
			EditorLOG_WARNING(TEXT("[WriteLocalizationResources] Failed to write %s"), *locresPath);
	}

	// The .locmeta tells the engine which culture is the native one and which cultures exist.
	FTextLocalizationMetaDataResource metaData;
	metaData.NativeCulture = DefaultLocale;
	metaData.NativeLocRes = DefaultLocale / FString(TEXT("MounteaDialogue.locres"));
	TArray<FString> cultureDirectories;
	IFileManager::Get().FindFiles(cultureDirectories, *(targetRoot / TEXT("*")), false, true);
	metaData.CompiledCultures = cultureDirectories;
	if (!metaData.SaveToFile(FPaths::Combine(targetRoot, TEXT("MounteaDialogue.locmeta"))))
		EditorLOG_WARNING(TEXT("[WriteLocalizationResources] Failed to write the .locmeta file"));
}

void UMounteaDialogueSystemImportExportHelpers::RegisterLocalizationPath()
{
	const FString pathLine = TEXT("+LocalizationPaths=%GAMEDIR%Content/Localization/MounteaDialogue");
	const FString iniPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));

	FString content;
	FFileHelper::LoadFileToString(content, *iniPath);
	if (content.Contains(TEXT("Content/Localization/MounteaDialogue")))
		return;

	TArray<FString> lines;
	content.ParseIntoArrayLines(lines, false);

	const int32 section = lines.IndexOfByPredicate([](const FString& Line) { return Line.TrimStartAndEnd().Equals(TEXT("[Internationalization]")); });
	if (section == INDEX_NONE)
	{
		if (!lines.IsEmpty() && !lines.Last().IsEmpty())
			lines.Add(FString());
		lines.Add(TEXT("[Internationalization]"));
		lines.Add(pathLine);
	}
	else
		lines.Insert(pathLine, section + 1);

	if (FFileHelper::SaveStringToFile(FString::Join(lines, LINE_TERMINATOR) + LINE_TERMINATOR, *iniPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		EditorLOG_INFO(TEXT("[RegisterLocalizationPath] Added the Mountea localization folder to DefaultGame.ini. Restart the editor to load the translations."));
	}
	else
	{
		EditorLOG_WARNING(TEXT("[RegisterLocalizationPath] Could not update %s (read-only?). Add '%s' under [Internationalization] to load the translations."), *iniPath, *pathLine);
	}
}

FString UMounteaDialogueSystemImportExportHelpers::GetRowTitleEntryKey(const UStringTable* NodesStringTable, const MounteaArchive::FNode& NodeRecord)
{
	if (!NodesStringTable)
		return FString();

	// Selection Title is what the choice button shows; fall back to the display name when it is empty.
	for (const FString& key : { NodeRecord.SelectionTitleKey, NodeRecord.DisplayNameKey })
	{
		FString text;
		if (!key.IsEmpty() && NodesStringTable->GetStringTable()->GetSourceString(FTextKey(key), text) && !text.IsEmpty())
			return key;
	}
	return FString();
}

FText UMounteaDialogueSystemImportExportHelpers::MakeRowTitle(const UStringTable* NodesStringTable, const MounteaArchive::FNode& NodeRecord)
{
	const FString key = GetRowTitleEntryKey(NodesStringTable, NodeRecord);
	if (key.IsEmpty())
		return NodeRecord.Label.IsEmpty() ? FText::GetEmpty() : FText::FromString(NodeRecord.Label);

	return FText::FromStringTable(NodesStringTable->GetStringTableId(), key);
}

// ---------------------------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------------------------

TMap<FGuid, USoundWave*> UMounteaDialogueSystemImportExportHelpers::ImportRowAudio(const MounteaArchive::FDialogueArchive& Archive, const MounteaArchive::FDialogue& Dialogue, UObject* InParent)
{
	TMap<FGuid, USoundWave*> audioMap;
	if (!InParent)
		return audioMap;

	bool bAnyAudio = false;
	for (const MounteaArchive::FNode& node : Archive.Nodes)
	{
		if (node.DialogueId != Dialogue.Id)
			continue;
		for (const MounteaArchive::FRow& row : node.Rows)
			bAnyAudio |= row.bHasAudio && row.Audio.IsSet();
	}
	if (!bAnyAudio)
		return audioMap;

	FModuleManager::LoadModuleChecked<IAudioEditorModule>("AudioEditor");
	IAudioEditorModule& audioEditorModule = FModuleManager::GetModuleChecked<IAudioEditorModule>("AudioEditor");
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FString packagePath = FPackageName::GetLongPackagePath(InParent->GetPathName());

	{
		FString audioFolderPath = FPaths::Combine(FPaths::ProjectContentDir(), packagePath.RightChop(6), TEXT("audio"));
		FPaths::MakeStandardFilename(audioFolderPath);
		if (!FPaths::DirectoryExists(audioFolderPath))
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*audioFolderPath);
	}

	for (const MounteaArchive::FNode& node : Archive.Nodes)
	{
		if (node.DialogueId != Dialogue.Id)
			continue;

		for (const MounteaArchive::FRow& row : node.Rows)
		{
			if (!row.bHasAudio || !row.Audio.IsSet())
				continue;

			const FGuid rowGuid = MounteaArchive::IdToGuid(row.Id);
			const FString subfolder = FString::Printf(TEXT("/audio/%s"), *rowGuid.ToString(EGuidFormats::DigitsWithHyphensLower));
			const FString assetName = SanitizeAssetName(FPaths::GetBaseFilename(row.Audio.Name), TEXT("Audio"));
			const FString tempPath = TempFileFor(TEXT("MounteaAudio"), AudioExtension(row.Audio), row.Audio.Bytes);
			if (tempPath.IsEmpty())
			{
				EditorLOG_WARNING(TEXT("[ImportRowAudio] Could not write temp file for row '%s'"), *row.Id);
				continue;
			}

			USoundWave* importedSound = ImportSingleAudioAsset(tempPath, packagePath + subfolder / assetName, assetName, packagePath + subfolder, audioEditorModule, assetRegistryModule);
			IFileManager::Get().Delete(*tempPath);

			if (importedSound)
				audioMap.Add(rowGuid, importedSound);
			else
				EditorLOG_WARNING(TEXT("[ImportRowAudio] Failed to import audio of row '%s'"), *row.Id);
		}
	}

	return audioMap;
}

// ---------------------------------------------------------------------------------------------
// Nodes and edges
// ---------------------------------------------------------------------------------------------

bool UMounteaDialogueSystemImportExportHelpers::PopulateNodes(UMounteaDialogueGraph* Graph, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue,
	TMap<FGuid, UMounteaDialogueGraphNode*>& OutSpawnedNodes, const UStringTable* NodesStringTable)
{
	if (!IsValid(Graph) || !Graph->GetOutermost()->IsValidLowLevel())
	{
		EditorLOG_ERROR(TEXT("[PopulateNodes] Invalid Graph object or package"));
		return false;
	}

	using namespace MounteaArchive;

	for (const FNode& record : Context.Archive->Nodes)
	{
		if (record.DialogueId != Dialogue.Id)
			continue;

		UMounteaDialogueGraphNode* node = nullptr;

		if (record.Type == Contract::NodeStart)
		{
			node = Graph->GetStartNode();
			if (!node)
			{
				if (UMounteaDialogueGraphNode_StartNode* newStart = Graph->ConstructDialogueNode<UMounteaDialogueGraphNode_StartNode>())
				{
					Graph->StartNode = newStart;
					node = newStart;
				}
			}
		}
		else
		{
			TSubclassOf<UMounteaDialogueGraphNode> nodeClass;
			if (record.Type == Contract::NodeLead) nodeClass = UMounteaDialogueGraphNode_LeadNode::StaticClass();
			else if (record.Type == Contract::NodeAnswer) nodeClass = UMounteaDialogueGraphNode_AnswerNode::StaticClass();
			else if (record.Type == Contract::NodeComplete) nodeClass = UMounteaDialogueGraphNode_CompleteNode::StaticClass();
			else if (record.Type == Contract::NodeReturn) nodeClass = UMounteaDialogueGraphNode_ReturnToNode::StaticClass();
			else if (record.Type == Contract::NodeOpenChildGraph) nodeClass = UMounteaDialogueGraphNode_OpenChildGraph::StaticClass();
			else if (record.Type == Contract::NodeDelay) nodeClass = UMounteaDialogueGraphNode_Delay::StaticClass();

			if (nodeClass)
			{
				node = Graph->ConstructDialogueNode(nodeClass);
				if (node)
					Graph->AllNodes.Add(node);
			}
		}

		if (!node)
		{
			EditorLOG_WARNING(TEXT("[PopulateNodes] Node '%s' (%s) could not be created - skipped"), *record.Id, *record.Type);
			continue;
		}

		PopulateNodeData(node, record, Context, NodesStringTable);
		OutSpawnedNodes.Add(node->GetNodeGUID(), node);
	}

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::PopulateNodeData(UMounteaDialogueGraphNode* Node, const MounteaArchive::FNode& NodeRecord, const FMounteaArchiveImportContext& Context, const UStringTable* NodesStringTable)
{
	if (!Node)
		return;

	const MounteaArchive::FDialogueArchive& archive = *Context.Archive;

	Node->SetNodeGUID(MounteaArchive::IdToGuid(NodeRecord.Id));
	// Sibling order comes from the edge array; PopulateEdges overwrites this for nodes that have a parent.
	Node->ExecutionOrder = 0;
	Node->NodePosition = FIntPoint(FMath::RoundToInt(NodeRecord.PosX), FMath::RoundToInt(NodeRecord.PosY));

	FString title = NodeRecord.Label;
	if (title.IsEmpty())
		title = DefaultText(archive, NodeRecord.DisplayNameKey);
	Node->NodeTitle = FText::FromString(title);

	if (NodeRecord.bHasDuration)
	{
		if (UMounteaDialogueGraphNode_Delay* delayNode = Cast<UMounteaDialogueGraphNode_Delay>(Node))
			delayNode->SetDelayDuration(FMath::RoundToInt(NodeRecord.Duration));
	}

	for (const MounteaArchive::FDecoratorInstance& instance : NodeRecord.Decorators)
	{
		UClass* decoratorClass = Context.DecoratorClasses.FindRef(MounteaArchive::IdToGuid(instance.DefinitionId));
		if (!decoratorClass)
			decoratorClass = FindDecoratorClassByName(instance.Name);

		if (!decoratorClass)
		{
			EditorLOG_WARNING(TEXT("[PopulateNodeData] No UE decorator class for '%s' (%s) - skipped"), *instance.Name, *instance.DefinitionId);
			continue;
		}

		FMounteaDialogueDecorator decoratorInstance;
		decoratorInstance.DecoratorType = NewObject<UMounteaDialogueDecoratorBase>(Node, decoratorClass);
		if (decoratorInstance.DecoratorType)
		{
			// Fresh per-instance GUID (definition GUID lives on the class CDO)
			decoratorInstance.DecoratorType->DecoratorGUID = FGuid::NewGuid();
			ApplyJsonValues(decoratorInstance.DecoratorType, instance.Values);
		}
		Node->NodeDecorators.Add(decoratorInstance);
	}

	// Return-to-node: target is stored raw and resolved once every node exists.
	if (UMounteaDialogueGraphNode_ReturnToNode* returnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(Node))
	{
		if (!NodeRecord.TargetNode.IsEmpty())
			returnNode->SelectedNodeIndex = MounteaArchive::GuidToId(MounteaArchive::IdToGuid(NodeRecord.TargetNode));
	}

	// Open-child-graph: target dialogue is resolved by ResolveOpenChildGraphTargets after all graphs exist.
	if (UMounteaDialogueGraphNode_OpenChildGraph* openNode = Cast<UMounteaDialogueGraphNode_OpenChildGraph>(Node))
	{
#if WITH_EDITORONLY_DATA
		if (!NodeRecord.TargetDialogue.IsEmpty())
			openNode->PendingTargetDialogueGUID = MounteaArchive::IdToGuid(NodeRecord.TargetDialogue);
#endif
	}
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateEdges(UMounteaDialogueGraph* Graph, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[PopulateEdges] Invalid Graph object"));
		return false;
	}

	TMap<UMounteaDialogueGraphNode*, int32> outgoingCount;

	for (const MounteaArchive::FArchiveEdge& edgeRecord : Context.Archive->Edges)
	{
		if (edgeRecord.DialogueId != Dialogue.Id)
			continue;

		UMounteaDialogueGraphNode* sourceNode = Graph->FindNodeByGuid(MounteaArchive::IdToGuid(edgeRecord.SourceId));
		UMounteaDialogueGraphNode* targetNode = Graph->FindNodeByGuid(MounteaArchive::IdToGuid(edgeRecord.TargetId));
		if (!sourceNode || !targetNode)
		{
			EditorLOG_WARNING(TEXT("[PopulateEdges] Could not find source or target node for edge: %s -> %s"), *edgeRecord.SourceId, *edgeRecord.TargetId);
			continue;
		}

		UMounteaDialogueGraphEdge* newEdge = NewObject<UMounteaDialogueGraphEdge>(Graph);
		if (!newEdge)
		{
			EditorLOG_ERROR(TEXT("[PopulateEdges] Failed to create edge object for: %s -> %s"), *edgeRecord.SourceId, *edgeRecord.TargetId);
			continue;
		}

		newEdge->Graph = Graph;
		newEdge->StartNode = sourceNode;
		newEdge->EndNode = targetNode;

		sourceNode->ChildrenNodes.AddUnique(targetNode);
		sourceNode->Edges.Add(targetNode, newEdge);
		targetNode->ParentNodes.AddUnique(sourceNode);

		// Edge array order is the sibling order (same rule Dialoguer applies).
		int32& count = outgoingCount.FindOrAdd(sourceNode);
		targetNode->ExecutionOrder = count++;

		// For reimport: the old EdNode still points at the previous blank edge; keep it on the new one
		// so the conditions set below survive the visual graph rebuild.
		if (UEdGraph_MounteaDialogueGraph* edGraph = Cast<UEdGraph_MounteaDialogueGraph>(Graph->EdGraph))
		{
			for (UEdGraphNode* edNode : edGraph->Nodes)
			{
				UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(edNode);
				if (!edgeNode || !edgeNode->MounteaDialogueGraphEdge)
					continue;
				const UMounteaDialogueGraphEdge* oldEdge = edgeNode->MounteaDialogueGraphEdge;
				if (oldEdge->StartNode && oldEdge->StartNode->GetNodeGUID() == sourceNode->GetNodeGUID()
					&& oldEdge->EndNode && oldEdge->EndNode->GetNodeGUID() == targetNode->GetNodeGUID())
				{
					edgeNode->MounteaDialogueGraphEdge = newEdge;
					break;
				}
			}
		}

		if (edgeRecord.Rules.IsEmpty())
			continue;

		newEdge->EdgeConditions.Mode = edgeRecord.ConditionMode == TEXT("any") ? EConditionEvaluationMode::Any : EConditionEvaluationMode::All;

		for (const MounteaArchive::FConditionRule& rule : edgeRecord.Rules)
		{
			UClass* conditionClass = Context.ConditionClasses.FindRef(MounteaArchive::IdToGuid(rule.DefinitionId));
			if (!conditionClass)
				conditionClass = FindConditionClassByName(rule.Name);
			if (!conditionClass)
			{
				EditorLOG_WARNING(TEXT("[PopulateEdges] No UE condition class for '%s' (%s) - using base class as placeholder"), *rule.Name, *rule.DefinitionId);
				conditionClass = UMounteaDialogueConditionBase::StaticClass();
			}

			FMounteaDialogueCondition conditionInstance;
			conditionInstance.ConditionClass = NewObject<UMounteaDialogueConditionBase>(newEdge, conditionClass);
			conditionInstance.bNegate = rule.bNegate;
			if (conditionInstance.ConditionClass)
			{
				// Preserve the original Dialoguer name so the user knows what to replace it with
				if (!rule.Name.IsEmpty())
					conditionInstance.ConditionClass->ConditionName = FName(*rule.Name);
				conditionInstance.ConditionClass->SetConditionGUID(MounteaArchive::IdToGuid(rule.DefinitionId));
				ApplyJsonValues(conditionInstance.ConditionClass, rule.Values);
			}
			newEdge->EdgeConditions.Rules.Add(conditionInstance);
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------------
// Data tables
// ---------------------------------------------------------------------------------------------

bool UMounteaDialogueSystemImportExportHelpers::FillParticipantsDataTable(UMounteaDialogueGraph* Graph, UDataTable* ParticipantsTable, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue)
{
	if (!Graph || !ParticipantsTable)
	{
		EditorLOG_ERROR(TEXT("[FillParticipantsDataTable] Invalid Graph or DataTable"));
		return false;
	}

	const MounteaArchive::FDialogueArchive& archive = *Context.Archive;

	// Clear stale rows so reimport starts fresh
	ParticipantsTable->EmptyTable();

	TSet<FString> usedParticipants;
	for (const MounteaArchive::FNode& node : archive.Nodes)
	{
		if (node.DialogueId != Dialogue.Id)
			continue;
		if (!node.ParticipantId.IsEmpty())
			usedParticipants.Add(node.ParticipantId);
		for (const MounteaArchive::FRow& row : node.Rows)
		{
			if (!row.ParticipantId.IsEmpty())
				usedParticipants.Add(row.ParticipantId);
		}
	}

	for (const MounteaArchive::FParticipant& participant : archive.Participants)
	{
		if (!usedParticipants.Contains(participant.Id))
			continue;

		FDialogueParticipant newRow;
		newRow.ParticipantName = FName(*participant.Name);

		if (const FString* fullTag = Context.ParticipantTags.Find(participant.Id))
			newRow.ParticipantCategoryTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(**fullTag), false);

		if (participant.bHasThumbnail)
		{
			const FString textureSoftPath = FString::Printf(TEXT("%s/thumbnails/%s.%s"), *Context.Request.TargetFolder, *participant.Name, *participant.Name);
			newRow.ParticipantIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(textureSoftPath));
		}

		ParticipantsTable->AddRow(FName(*participant.Name), newRow);
	}

	SaveAsset(ParticipantsTable);
	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::FillDialogueRowsDataTable(UMounteaDialogueGraph* Graph, UDataTable* DialogueRowsTable, UDataTable* ParticipantsTable,
	UStringTable* DialogueRowsStringTable, UStringTable* NodesStringTable, const FMounteaArchiveImportContext& Context,
	const MounteaArchive::FDialogue& Dialogue, const TMap<FGuid, USoundWave*>& AudioMap)
{
	if (!IsValid(Graph) || !DialogueRowsTable || !ParticipantsTable || !DialogueRowsStringTable || !NodesStringTable)
	{
		EditorLOG_ERROR(TEXT("[FillDialogueRowsDataTable] One or more required assets are null"));
		return false;
	}

	const MounteaArchive::FDialogueArchive& archive = *Context.Archive;

	// Clear stale rows so reimport starts fresh
	DialogueRowsTable->EmptyTable();

	for (const MounteaArchive::FNode& node : archive.Nodes)
	{
		if (node.DialogueId != Dialogue.Id || node.Rows.IsEmpty())
			continue;

		const MounteaArchive::FParticipant* participant = archive.FindParticipant(node.ParticipantId);
		if (!participant)
		{
			EditorLOG_WARNING(TEXT("[FillDialogueRowsDataTable] No participant for node: %s - skipped"), *node.Id);
			continue;
		}

		ProcessDialogueRowGroup(Graph, node, participant->Name, ParticipantsTable, DialogueRowsTable, DialogueRowsStringTable, NodesStringTable, AudioMap);
	}

	SaveAsset(DialogueRowsTable);
	return true;
}

void UMounteaDialogueSystemImportExportHelpers::ProcessDialogueRowGroup(UMounteaDialogueGraph* Graph, const MounteaArchive::FNode& NodeRecord, const FString& ParticipantName,
	UDataTable* ParticipantsTable, UDataTable* DialogueRowsTable, UStringTable* DialogueRowsStringTable, UStringTable* NodesStringTable,
	const TMap<FGuid, USoundWave*>& AudioMap)
{
	FDialogueParticipant* participant = ParticipantsTable->FindRow<FDialogueParticipant>(FName(*ParticipantName), TEXT(""));
	if (!participant)
	{
		EditorLOG_WARNING(TEXT("[ProcessDialogueRowGroup] Participant '%s' not in table - skipped"), *ParticipantName);
		return;
	}

	FDialogueRow newRow;
	newRow.RowGUID = MounteaArchive::IdToGuid(NodeRecord.Id);
	newRow.DialogueParticipantName = participant->ParticipantName;
	newRow.CompatibleTags.AddTag(participant->ParticipantCategoryTag);
	newRow.RowTitle = MakeRowTitle(NodesStringTable, NodeRecord);

	for (const MounteaArchive::FRow& row : NodeRecord.Rows)
	{
		if (!row.ParticipantId.IsEmpty() && row.ParticipantId != NodeRecord.ParticipantId)
			EditorLOG_WARNING(TEXT("[ProcessDialogueRowGroup] Row '%s' has its own participant; Unreal rows follow the node's participant"), *row.Id);

		FDialogueRowData rowData;
		rowData.RowGUID = MounteaArchive::IdToGuid(row.Id);
		rowData.RowText = FText::FromStringTable(DialogueRowsStringTable->GetStringTableId(), row.TextKey);
		rowData.RowDuration = row.Duration;
		rowData.RowDurationMode = row.DurationMode;
		rowData.RowDurationOverride = row.DurationOverride;
		rowData.RowExecutionBehaviour = row.ExecutionBehaviour;

		USoundWave* const* foundSound = AudioMap.Find(rowData.RowGUID);
		if (foundSound && *foundSound)
			rowData.RowSound = *foundSound;

		newRow.RowData.Add(rowData);
	}

	TArray<FDialogueRow*> existingRows;
	FString context;
	DialogueRowsTable->GetAllRows(context, existingRows);

	FString rowName = newRow.DialogueParticipantName.ToString();
	rowName.Append(TEXT("_")).Append(FString::FromInt(existingRows.Num()));
	DialogueRowsTable->AddRow(FName(*rowName), newRow);

	// Link the matching dialogue node to its row
	for (UMounteaDialogueGraphNode* node : Graph->GetAllNodes())
	{
		if (!node || node->GetNodeGUID() != newRow.RowGUID)
			continue;

		if (UMounteaDialogueGraphNode_DialogueNodeBase* dialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(node))
		{
			dialogueNode->SetDataTable(DialogueRowsTable);
			dialogueNode->SetRowName(FName(*rowName));
		}
	}
}

#undef LOCTEXT_NAMESPACE
