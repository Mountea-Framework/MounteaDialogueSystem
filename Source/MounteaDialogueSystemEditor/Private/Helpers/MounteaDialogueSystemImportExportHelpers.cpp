// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueSystemImportExportHelpers.h"

#include "AssetImportTask.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"

#include "Sound/SoundWave.h"

#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Data/MounteaDialogueGraphExtraDataTypes.h"

#include "AssetToolsModule.h"
#include "AudioEditorModule.h"

#include "GameplayTagsManager.h"
#include "GameplayTagsSettings.h"

#include "Engine/DataTable.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/DateTime.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "zip.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Framework/Notifications/NotificationManager.h"

#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Conditions/MounteaDialogueConditionBase.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#include "ImportConfig/MounteaDialogueImportConfig.h"

#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"

#include "Interfaces/IPluginManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/TextureDefines.h"

#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"

// ---------------------------------------------------------------------------
// Minimal standard-ZIP writer (stored, no compression, no data descriptors).
// Produces ZIP files readable by JSZip 3.x without ZIP64 or data-descriptor quirks.
// ---------------------------------------------------------------------------
namespace
{
	struct FZipFileEntry
	{
		FString       Name;        // archive path (UTF-8 when serialised)
		TArray<uint8> Data;
		uint32        Crc32;
		uint32        LocalHeaderOffset;
	};

	struct FSimpleZipWriter
	{
		TArray<FZipFileEntry> Entries;

		void AddFile(const FString& Name, const TArray<uint8>& Data)
		{
			FZipFileEntry entry;
			entry.Name               = Name;
			entry.Data               = Data;
			entry.Crc32              = FCrc::MemCrc32(Data.GetData(), Data.Num());
			entry.LocalHeaderOffset  = 0;
			Entries.Add(MoveTemp(entry));
		}

		void AddFile(const FString& Name, const FString& Text)
		{
			FTCHARToUTF8 converted(*Text);
			TArray<uint8> data;
			data.Append(reinterpret_cast<const uint8*>(converted.Get()), converted.Length());
			AddFile(Name, data);
		}

		bool Save(const FString& FilePath) const
		{
			TArray<uint8> buf;
			// Pre-allocate a rough estimate to avoid repeated reallocations
			{
				int64 est = 0;
				for (const FZipFileEntry& e : Entries)
					est += 30 + e.Name.Len() + e.Data.Num() + 46 + e.Name.Len();
				est += 22;
				buf.Reserve(static_cast<int32>(est));
			}

			auto w16 = [&](uint16 v) { buf.Append(reinterpret_cast<const uint8*>(&v), 2); };
			auto w32 = [&](uint32 v) { buf.Append(reinterpret_cast<const uint8*>(&v), 4); };

			// DOS timestamp: fixed date for deterministic archives (1980-01-01 00:00:00)
			// Date bits: [15-9]=year_since_1980 [8-5]=month [4-0]=day
			const uint16 dosTime = 0x0000;
			const uint16 dosDate = 0x0021; // 1980-01-01 (year=0, month=1, day=1)

			// ── Local file headers + data ────────────────────────────────────────
			TArray<uint32> localOffsets;
			localOffsets.Reserve(Entries.Num());

			for (const FZipFileEntry& entry : Entries)
			{
				localOffsets.Add(static_cast<uint32>(buf.Num()));

				FTCHARToUTF8 nameUtf8(*entry.Name);
				uint16 nameLen = static_cast<uint16>(nameUtf8.Length());

				const uint8 lhSig[] = { 0x50, 0x4b, 0x03, 0x04 };
				buf.Append(lhSig, 4);
				w16(0x0014);                                // version needed: 2.0
				w16(0x0800);                                // general purpose: UTF-8 flag (bit 11)
				w16(0x0000);                                // compression: stored
				w16(dosTime);
				w16(dosDate);
				w32(entry.Crc32);
				w32(static_cast<uint32>(entry.Data.Num())); // compressed size
				w32(static_cast<uint32>(entry.Data.Num())); // uncompressed size
				w16(nameLen);
				w16(0x0000);                                // extra field length
				buf.Append(reinterpret_cast<const uint8*>(nameUtf8.Get()), nameLen);
				buf.Append(entry.Data);
			}

			// ── Central directory ────────────────────────────────────────────────
			uint32 cdStart = static_cast<uint32>(buf.Num());
			for (int32 i = 0; i < Entries.Num(); ++i)
			{
				const FZipFileEntry& entry = Entries[i];
				FTCHARToUTF8 nameUtf8(*entry.Name);
				uint16 nameLen = static_cast<uint16>(nameUtf8.Length());

				const uint8 cdSig[] = { 0x50, 0x4b, 0x01, 0x02 };
				buf.Append(cdSig, 4);
				w16(0x0314);     // version made by: 3.1 Unix (so tools don't add execute bits)
				w16(0x0014);     // version needed: 2.0
				w16(0x0800);     // general purpose: UTF-8
				w16(0x0000);     // compression: stored
				w16(dosTime);
				w16(dosDate);
				w32(entry.Crc32);
				w32(static_cast<uint32>(entry.Data.Num()));
				w32(static_cast<uint32>(entry.Data.Num()));
				w16(nameLen);
				w16(0x0000);     // extra field length
				w16(0x0000);     // file comment length
				w16(0x0000);     // disk number start
				w16(0x0000);     // internal attributes
				w32(0x00000000); // external attributes
				w32(localOffsets[i]);
				buf.Append(reinterpret_cast<const uint8*>(nameUtf8.Get()), nameLen);
			}
			uint32 cdSize = static_cast<uint32>(buf.Num()) - cdStart;

			// ── End of central directory record ─────────────────────────────────
			const uint8 eocdSig[] = { 0x50, 0x4b, 0x05, 0x06 };
			buf.Append(eocdSig, 4);
			w16(0x0000);                                   // disk number
			w16(0x0000);                                   // disk with CD
			w16(static_cast<uint16>(Entries.Num()));       // entries on this disk
			w16(static_cast<uint16>(Entries.Num()));       // total entries
			w32(cdSize);
			w32(cdStart);
			w16(0x0000);                                   // comment length

			return FFileHelper::SaveArrayToFile(buf, *FilePath);
		}
	};
}

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemImportExportHelpers"

bool UMounteaDialogueSystemImportExportHelpers::IsReimport(const FString& Filename)
{
	if (Filename.IsEmpty())
		return false;
	
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *Filename))
		return false;

	if (!IsZipFile(fileData))
		return false;

	TMap<FString, FString> extractedFiles;
	if (!ExtractFilesFromZip(fileData, extractedFiles))
		return false;

	if (!ValidateExtractedContent(extractedFiles))
		return false;

	FGuid dialogueGuid;
	if (extractedFiles.Contains("dialogueData.json"))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(extractedFiles["dialogueData.json"]);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasField(TEXT("dialogueGuid")))
				dialogueGuid = FGuid(JsonObject->GetStringField(TEXT("dialogueGuid")));
			
		}
	}
	else
		return false;

	const FString configFilePath = UMounteaDialogueImportConfig::GetImportConfigFilePath();
	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
	if (!importConfig)
		return false;

	if (FPaths::FileExists(configFilePath))
		importConfig->LoadConfig(nullptr, *configFilePath);

	return importConfig->ImportHistory.Contains(dialogueGuid);
}

bool UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(const FString& FilePath, UObject* ObjectRedirector, UMounteaDialogueGraph*& OutGraph, FString& OutMessage)
{
	if (FilePath.IsEmpty())
	{
		OutMessage = TEXT("Source file path is empty");
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *FilePath))
	{
		OutMessage = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	if (!IsZipFile(fileData))
	{
		OutMessage = FString::Printf(TEXT("Not a valid archive: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	TMap<FString, FString> extractedFiles;
	if (!ExtractFilesFromZip(fileData, extractedFiles))
	{
		OutMessage = FString::Printf(TEXT("Failed to extract archive: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	UObject* parent = OutGraph ? OutGraph->GetOuter() : ObjectRedirector;
	const FName assetName = OutGraph ? FName(*OutGraph->GetName()) : NAME_None;
	const EObjectFlags reimportFlags = OutGraph ? OutGraph->GetFlags() : RF_Public | RF_Standalone | RF_Transactional;
	return ImportDialogueGraphFromFiles(extractedFiles, FilePath, parent, assetName, reimportFlags, OutGraph, OutMessage);
}

bool UMounteaDialogueSystemImportExportHelpers::CanReimport(UObject* ObjectRedirector, TArray<FString>& OutFilenames)
{
	if (ObjectRedirector == nullptr) 
		return false;

	UMounteaDialogueGraph* dialogueGraph = Cast<UMounteaDialogueGraph>(ObjectRedirector);
	if (dialogueGraph == nullptr) 
		return false;
	
	return true;
}

void UMounteaDialogueSystemImportExportHelpers::UpdateGraphImportDataConfig(const UMounteaDialogueGraph* Graph, const FString& JsonName, const FString& Json, const FString& PackagePath, const FString& AssetName)
{
	const FString UpdatedConfigFile = UMounteaDialogueImportConfig::GetImportConfigFilePath();

	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();

	if (FPaths::FileExists(UpdatedConfigFile))
		importConfig->LoadConfig(nullptr, *UpdatedConfigFile);
	else
	{
		IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*FPaths::GetPath(UpdatedConfigFile));
		importConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);
	}

	if (importConfig)
	{
		if (FDialogueImportSourceData* dialogueConfig = importConfig->ImportHistory.Find(Graph->GetGraphGUID()))
		{
			TMap<FString, FDialogueImportData>& currentData = dialogueConfig->ImportData;
			currentData.Add(FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName), FDialogueImportData(JsonName, Json));
			importConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);
		}
	}
}

bool UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraph(const FString& FilePath, UObject* InParent, const FName Name, const EObjectFlags Flags, UMounteaDialogueGraph*& OutGraph, FString& OutMessage)
{
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *FilePath))
	{
		OutMessage = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ImportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	if (!IsZipFile(fileData))
	{
		OutMessage = FString::Printf(TEXT("Not a valid archive: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ImportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	TMap<FString, FString> extractedFiles;
	if (!ExtractFilesFromZip(fileData, extractedFiles))
	{
		OutMessage = FString::Printf(TEXT("Failed to extract archive: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ImportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	return ImportDialogueGraphFromFiles(extractedFiles, FilePath, InParent, Name, Flags, OutGraph, OutMessage);
}

bool UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraphFromFiles(TMap<FString, FString>& extractedFiles, const FString& sourceFilePath, UObject* inParent, FName name, EObjectFlags flags, UMounteaDialogueGraph*& outGraph, FString& outMessage)
{
	// 1. Parse dialogueData.json for GUID and dialogue name
	if (!extractedFiles.Contains(TEXT("dialogueData.json")))
	{
		outMessage = TEXT("dialogueData.json not found in extracted files");
		EditorLOG_ERROR(TEXT("[ImportDialogueGraphFromFiles] %s"), *outMessage);
		return false;
	}

	TSharedPtr<FJsonObject> dialogueDataJson;
	TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(extractedFiles[TEXT("dialogueData.json")]);
	if (!FJsonSerializer::Deserialize(jsonReader, dialogueDataJson) || !dialogueDataJson.IsValid())
	{
		outMessage = TEXT("Failed to parse dialogueData.json");
		EditorLOG_ERROR(TEXT("[ImportDialogueGraphFromFiles] %s"), *outMessage);
		return false;
	}

	FGuid importedGuid;
	FString guidStr;
	if (!dialogueDataJson->TryGetStringField(TEXT("dialogueGuid"), guidStr) || !FGuid::Parse(guidStr, importedGuid))
	{
		outMessage = TEXT("dialogueGuid missing or invalid in dialogueData.json");
		EditorLOG_ERROR(TEXT("[ImportDialogueGraphFromFiles] %s"), *outMessage);
		return false;
	}

	FString dialogueName;
	dialogueDataJson->TryGetStringField(TEXT("dialogueName"), dialogueName);

	// 2. Locate an existing graph with this GUID (via import config history)
	UMounteaDialogueGraph* existingGraph = outGraph ? outGraph : LookupExistingGraphByGuid(importedGuid);

	bool bSuccess = false;

	// 3a. Reimport path: existing graph found with matching GUID
	if (existingGraph && existingGraph->GetGraphGUID() == importedGuid)
	{
		// GetAsset() from the asset registry may return a partially-loaded object.
		// SavePackage will crash with "cannot be saved as it has only been partially loaded"
		// unless the package is fully resident before we modify and re-save it.
		if (UPackage* existingPackage = existingGraph->GetOutermost())
			existingPackage->FullyLoad();

		existingGraph->ClearGraph();
		bSuccess = PopulateAndSaveGraph(existingGraph, extractedFiles, sourceFilePath, outMessage);
		if (bSuccess)
		{
			outGraph = existingGraph;
			outMessage = FString::Printf(TEXT("Graph '%s' reimported successfully."), *existingGraph->GetName());
		}
	}
	else
	{
		// 3b. New import path
		const FName assetName = !dialogueName.IsEmpty() ? FName(*dialogueName) : name;

		if (!outGraph)
			outGraph = NewObject<UMounteaDialogueGraph>(inParent, UMounteaDialogueGraph::StaticClass(), assetName, flags | RF_Transactional);
		else
			outGraph->Rename(*assetName.ToString());

		if (!outGraph)
		{
			outMessage = TEXT("Failed to create new graph object");
			EditorLOG_ERROR(TEXT("[ImportDialogueGraphFromFiles] %s"), *outMessage);
			return false;
		}

		bSuccess = PopulateAndSaveGraph(outGraph, extractedFiles, sourceFilePath, outMessage);
		if (bSuccess)
			outMessage = FString::Printf(TEXT("Graph '%s' created successfully."), *outGraph->GetName());
	}

	// Always clean up audio temp files
	for (const auto& file : extractedFiles)
	{
		if (file.Key.StartsWith(TEXT("audio/")))
			IFileManager::Get().Delete(*file.Value);
	}

	return bSuccess;
}

UMounteaDialogueGraph* UMounteaDialogueSystemImportExportHelpers::LookupExistingGraphByGuid(const FGuid& ImportedGuid)
{
	if (!ImportedGuid.IsValid())
		return nullptr;

	FAssetRegistryModule& reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Primary: query the asset registry by the GraphGUID tag (AssetRegistrySearchable).
	// This requires no asset loads and no config file — it uses the serialized tag index.
	// The tag value is the GUID string in the same format FGuid::ToString() produces.
	{
		FARFilter filter;
		filter.bRecursivePaths = true;
		filter.PackagePaths.Add(FName(TEXT("/Game")));
		filter.ClassPaths.Add(UMounteaDialogueGraph::StaticClass()->GetClassPathName());
		filter.TagsAndValues.Add(FName(TEXT("GraphGUID")), ImportedGuid.ToString(EGuidFormats::Digits));

		TArray<FAssetData> results;
		reg.Get().GetAssets(filter, results);
		if (results.Num() > 0)
		{
			UMounteaDialogueGraph* graph = Cast<UMounteaDialogueGraph>(results[0].GetAsset());
			if (graph && graph->GetGraphGUID() == ImportedGuid)
				return graph;
		}
	}

	// Secondary: fast path via the stored DialogueAssetPath in the import config.
	// Handles assets imported before AssetRegistrySearchable was added.
	{
		const FString configFilePath = UMounteaDialogueImportConfig::GetImportConfigFilePath();
		UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
		if (importConfig && FPaths::FileExists(configFilePath))
			importConfig->LoadConfig(nullptr, *configFilePath);

		if (importConfig && importConfig->ImportHistory.Contains(ImportedGuid))
		{
			const FString& assetPath = importConfig->ImportHistory[ImportedGuid].DialogueAssetPath;
			if (!assetPath.IsEmpty())
			{
				const FString assetObjectPath = FString::Printf(TEXT("%s.%s"), *assetPath, *FPaths::GetBaseFilename(assetPath));
				const FAssetData directAsset = reg.Get().GetAssetByObjectPath(FSoftObjectPath(assetObjectPath));
				if (directAsset.IsValid())
				{
					UMounteaDialogueGraph* graph = Cast<UMounteaDialogueGraph>(directAsset.GetAsset());
					if (graph && graph->GetGraphGUID() == ImportedGuid)
						return graph;
				}
			}
		}
	}

	return nullptr;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateAndSaveGraph(UMounteaDialogueGraph* Graph, TMap<FString, FString>& ExtractedFiles, const FString& SourceFilePath, FString& OutMessage)
{
	if (!ValidateExtractedContent(ExtractedFiles))
	{
		OutMessage = TEXT("Archive is missing required files");
		EditorLOG_ERROR(TEXT("[PopulateAndSaveGraph] %s"), *OutMessage);
		return false;
	}

	if (!PopulateGraphFromExtractedFiles(Graph, ExtractedFiles, SourceFilePath))
	{
		OutMessage = TEXT("Failed to populate graph from extracted files");
		EditorLOG_ERROR(TEXT("[PopulateAndSaveGraph] %s"), *OutMessage);
		return false;
	}

	Graph->CreateGraph();
	if (Graph->EdGraph)
	{
		if (UEdGraph_MounteaDialogueGraph* edGraph = Cast<UEdGraph_MounteaDialogueGraph>(Graph->EdGraph))
			edGraph->RebuildMounteaDialogueGraph();
	}

	SaveAsset(Graph);

	// Resolve OpenChildGraph targets now that the graph is saved and visible to the asset registry.
	// For single-file imports this catches pre-existing target graphs.
	// For project imports the factory runs a second pass with the full imported-graphs map.
	ResolveOpenChildGraphTargets(Graph, TMap<FGuid, UMounteaDialogueGraph*>());

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::ExportDialogueGraph(const UMounteaDialogueGraph* Graph, const FString& FilePath)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("Invalid Graph object provided for export"));
		return false;
	}

	TMap<FString, FString> JsonFiles;
	TArray<FString> AudioFiles;

	if (!GatherAssetsFromGraph(Graph, JsonFiles, AudioFiles))
	{
		EditorLOG_ERROR(TEXT("Failed to gather assets from Graph"));
		return false;
	}

	const FString ExportDirectory = FPaths::GetPath(FilePath);
	TArray<FString> ExportedAudioFiles;
	TMap<FString, FString> ThumbnailFiles;
	GatherThumbnailFiles(Graph, ThumbnailFiles);
	bool bSuccess = false;

	if (ExportAudioFiles(AudioFiles, ExportDirectory, ExportedAudioFiles))
	{
		bSuccess = PackToMNTEADLG(JsonFiles, ExportedAudioFiles, ThumbnailFiles, FilePath);

		if (!bSuccess)
			EditorLOG_ERROR(TEXT("[ExportDialogueGraph] Failed to pack files into MNTEADLG"));
	}
	else
		EditorLOG_ERROR(TEXT("[ExportDialogueGraph] Failed to export audio files"));

	// Clean up temp thumbnail files
	for (const auto& thumb : ThumbnailFiles)
		IFileManager::Get().Delete(*thumb.Value);

	FString AudioDirectory = ExportDirectory;
	AudioDirectory.Append(TEXT("/audio"));

	TSet<FString> AudioDirectories;
	for (const FString& ExportedAudioFile : ExportedAudioFiles)
	{
		AudioDirectories.Add(FPaths::GetPath(ExportedAudioFile));
		IFileManager::Get().Delete(*ExportedAudioFile);
	}

	for (const FString& Directory : AudioDirectories)
	{
		IFileManager::Get().DeleteDirectory(*Directory, false, true);
	}
	IFileManager::Get().DeleteDirectory(*AudioDirectory, false, true);

	return bSuccess;
}

bool UMounteaDialogueSystemImportExportHelpers::IsZipFile(const TArray<uint8>& FileData)
{
	// ZIP files can start with several different signatures
	constexpr uint32 ZIP_SIGNATURE_1 = 0x04034b50; // Regular ZIP file
	constexpr uint32 ZIP_SIGNATURE_2 = 0x08074b50; // Spanned ZIP file
	constexpr uint32 ZIP_SIGNATURE_3 = 0x02014b50; // Central directory header
	constexpr uint32 ZIP_SIGNATURE_4 = 0x06054b50; // End of central directory record

	if (FileData.Num() < 4)
	{
		EditorLOG_WARNING(TEXT("File is too small to be a valid ZIP file"));
		return false;
	}

	// Read the first 4 bytes as a uint32
	uint32 fileSignature = (FileData[3] << 24) | (FileData[2] << 16) | (FileData[1] << 8) | FileData[0];

	// Check against known ZIP signatures
	if (fileSignature == ZIP_SIGNATURE_1 || fileSignature == ZIP_SIGNATURE_2 || fileSignature == ZIP_SIGNATURE_3 || fileSignature == ZIP_SIGNATURE_4)
		return true;

	EditorLOG_WARNING(TEXT("[IsZipFile] File does not have a valid ZIP signature"));
	return false;
}

bool UMounteaDialogueSystemImportExportHelpers::ExtractFilesFromZip(const TArray<uint8>& ZipData, TMap<FString, FString>& OutExtractedFiles)
{
	FString tempFilePath = FPaths::CreateTempFilename(FPlatformProcess::UserTempDir(), TEXT("MounteaDialogue"), TEXT(".zip"));
	if (!FFileHelper::SaveArrayToFile(ZipData, *tempFilePath))
	{
		EditorLOG_ERROR(TEXT("[ExtractFilesFromZip] Failed to save zip data to temporary file"));
		return false;
	}

	struct zip_t* zip = zip_open(TCHAR_TO_UTF8(*tempFilePath), 0, 'r');
	if (!zip)
	{
		EditorLOG_ERROR(TEXT("[ExtractFilesFromZip] Failed to open zip file"));
		IFileManager::Get().Delete(*tempFilePath);
		return false;
	}

	int n = zip_entries_total(zip);
	for (int i = 0; i < n; ++i)
	{
		zip_entry_openbyindex(zip, i);
		{
			const char* name = zip_entry_name(zip);
			int size = zip_entry_size(zip);

			FString FileName = UTF8_TO_TCHAR(name);
			const bool bIsPng = (FileName.StartsWith(TEXT("Thumbnails/")) || FileName.StartsWith(TEXT("thumbnails/"))) && FileName.EndsWith(TEXT(".png"));
			const bool bIsAudioFile = FileName.StartsWith(TEXT("audio/")) &&
				(FileName.EndsWith(TEXT(".wav")) || FileName.EndsWith(TEXT(".mp3"))
				|| FileName.EndsWith(TEXT(".ogg")) || FileName.EndsWith(TEXT(".flac")));
			const bool bIsBinaryFile = bIsAudioFile || FileName.EndsWith(TEXT(".mnteadlg")) || bIsPng;

			if (bIsBinaryFile)
			{
				// Binary files — extract to a temp file; store the path (not the content).
				// Preserve the original extension so format-detection in the audio/texture
				// importers works correctly (e.g. ImportSoundWave dispatches by extension).
				FString tempSuffix = TEXT(".wav");
				FString tempPrefix = TEXT("TempAudio");
				if (FileName.EndsWith(TEXT(".mnteadlg")))      { tempSuffix = TEXT(".mnteadlg"); tempPrefix = TEXT("MounteaDialogue"); }
				else if (bIsPng)                               { tempSuffix = TEXT(".png");       tempPrefix = TEXT("TempThumbnail");  }
				else if (FileName.EndsWith(TEXT(".mp3")))        tempSuffix = TEXT(".mp3");
				else if (FileName.EndsWith(TEXT(".ogg")))        tempSuffix = TEXT(".ogg");
				else if (FileName.EndsWith(TEXT(".flac")))       tempSuffix = TEXT(".flac");
				FString TempBinaryPath = FPaths::CreateTempFilename(FPlatformProcess::UserTempDir(), *tempPrefix, *tempSuffix);

				TArray<uint8> Buffer;
				Buffer.SetNum(size);

				if (zip_entry_noallocread(zip, Buffer.GetData(), size) != -1)
				{
					if (FFileHelper::SaveArrayToFile(Buffer, *TempBinaryPath))
						OutExtractedFiles.Add(FileName, TempBinaryPath);
					else
						EditorLOG_ERROR(TEXT("[ExtractFilesFromZip] Failed to save temp binary file: %s"), *TempBinaryPath);
				}
				else
					EditorLOG_ERROR(TEXT("[ExtractFilesFromZip] Failed to read binary file content: %hs"), name);
			}
			else
			{
				// For non-audio files, keep the existing string-based approach
				TArray<uint8> Buffer;
				Buffer.SetNum(size);

				if (zip_entry_noallocread(zip, Buffer.GetData(), size) != -1)
				{
					FString FileContent = BytesToString(Buffer.GetData(), size);
					OutExtractedFiles.Add(FileName, FileContent);
				}
				else
					EditorLOG_ERROR(TEXT("Failed to read file content: %hs"), name);
			}
		}
		zip_entry_close(zip);
	}

	zip_close(zip);
	IFileManager::Get().Delete(*tempFilePath);

	return true;
}

FString UMounteaDialogueSystemImportExportHelpers::BytesToString(const uint8* Bytes, const int32 Count)
{
	// Skip UTF-8 BOM (0xEF 0xBB 0xBF) when present so JSON deserialization never sees it
	int32 offset = 0;
	if (Count >= 3 && Bytes[0] == 0xEF && Bytes[1] == 0xBB && Bytes[2] == 0xBF)
		offset = 3;

	return FString(FUTF8ToTCHAR(reinterpret_cast<const UTF8CHAR*>(Bytes + offset), Count - offset));
}

bool UMounteaDialogueSystemImportExportHelpers::ValidateExtractedContent(const TMap<FString, FString>& ExtractedFiles)
{
	const TArray<FString> RequiredFiles = {
		"categories.json", "participants.json", "nodes.json", "edges.json", "dialogueData.json", "dialogueRows.json"
	};

	for (const auto& File : RequiredFiles)
	{
		if (!ExtractedFiles.Contains(File))
		{
			EditorLOG_ERROR(TEXT("[ValidateExtractedContent] Missing required file: %s"), *File);
			return false;
		}
	}

	// TODO: Add more specific validation

	return true;
}


UClass* UMounteaDialogueSystemImportExportHelpers::FindExistingDecoratorClass(const FGuid& DefinitionGUID, const FString& Name, const FString& PackagePath)
{
	// Pass 1: already-loaded classes (fast path — covers both C++ and previously loaded Blueprints)
	for (TObjectIterator<UClass> it; it; ++it)
	{
		if (!it->IsChildOf(UMounteaDialogueDecoratorBase::StaticClass())) 
			continue;
		if (it->HasAnyClassFlags(CLASS_Abstract)) 
			continue;

		const UMounteaDialogueDecoratorBase* cdo = it->GetDefaultObject<UMounteaDialogueDecoratorBase>();
		if (cdo && cdo->DecoratorGUID == DefinitionGUID)
			return *it;
	}

	// Pass 2: asset registry (handles Blueprint assets that are not yet loaded)
	FAssetRegistryModule& reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FSoftObjectPath assetPath(FString::Printf(TEXT("%s/%s.%s"), *PackagePath, *Name, *Name));
	const FAssetData assetData = reg.Get().GetAssetByObjectPath(assetPath);
	if (assetData.IsValid())
	{
		if (UBlueprint* bp = Cast<UBlueprint>(assetData.GetAsset()))
		{
			if (bp->GeneratedClass)
				return bp->GeneratedClass;
		}
	}

	return nullptr;
}

UClass* UMounteaDialogueSystemImportExportHelpers::FindExistingConditionClass(const FGuid& DefinitionGUID, const FString& Name, const FString& PackagePath)
{
	for (TObjectIterator<UClass> it; it; ++it)
	{
		if (!it->IsChildOf(UMounteaDialogueConditionBase::StaticClass())) 
			continue;

		if (it->HasAnyClassFlags(CLASS_Abstract)) 
			continue;

		const UMounteaDialogueConditionBase* cdo = it->GetDefaultObject<UMounteaDialogueConditionBase>();
		if (cdo && cdo->GetConditionGUID() == DefinitionGUID)
			return *it;
	}

	FAssetRegistryModule& reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FSoftObjectPath assetPath(FString::Printf(TEXT("%s/%s.%s"), *PackagePath, *Name, *Name));
	const FAssetData assetData = reg.Get().GetAssetByObjectPath(assetPath);
	if (assetData.IsValid())
	{
		if (UBlueprint* bp = Cast<UBlueprint>(assetData.GetAsset()))
		{
			if (bp->GeneratedClass)
				return bp->GeneratedClass;
		}
	}

	return nullptr;
}

static UClass* AddBlueprintPropertiesFromJson(UBlueprint* Blueprint, const TArray<TSharedPtr<FJsonValue>>* PropsArray)
{
	if (!PropsArray)
		return nullptr;

	for (const auto& propVal : *PropsArray)
	{
		const TSharedPtr<FJsonObject> propObj = propVal->AsObject();
		if (!propObj.IsValid())
			continue;

		FString propName, propType;
		propObj->TryGetStringField(TEXT("name"), propName);
		propObj->TryGetStringField(TEXT("type"), propType);
		if (propName.IsEmpty() || propType.IsEmpty())
			continue;

		FEdGraphPinType pinType;
		if      (propType == TEXT("string")) pinType.PinCategory = UEdGraphSchema_K2::PC_String;
		else if (propType == TEXT("bool"))   pinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		else if (propType == TEXT("int"))    pinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		else if (propType == TEXT("float"))  { pinType.PinCategory = UEdGraphSchema_K2::PC_Real; pinType.PinSubCategory = UEdGraphSchema_K2::PC_Float; }
		else continue;

		FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*propName), pinType);
	}

	return nullptr;
}

TMap<FGuid, UClass*> UMounteaDialogueSystemImportExportHelpers::CreateDecoratorBlueprints(const FString& Json, const FString& ProjectPackagePath)
{
	TMap<FGuid, UClass*> result;

	TArray<TSharedPtr<FJsonValue>> jsonArray;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(reader, jsonArray))
		return result;

	const FString decoratorsPath = ProjectPackagePath / TEXT("Decorators");

	for (const auto& val : jsonArray)
	{
		TSharedPtr<FJsonObject> obj = val->AsObject();
		if (!obj.IsValid())
			continue;

		FString idStr, name;
		obj->TryGetStringField(TEXT("id"), idStr);
		obj->TryGetStringField(TEXT("name"), name);
		if (idStr.IsEmpty() || name.IsEmpty())
			continue;

		const FGuid definitionGuid = FGuid(idStr);
		if (result.Contains(definitionGuid))
			continue;

		// Find existing class (by GUID, then by asset path)
		UClass* existingClass = FindExistingDecoratorClass(definitionGuid, name, decoratorsPath);
		if (existingClass)
		{
			result.Add(definitionGuid, existingClass);
			continue;
		}

		// Create new Blueprint subclass
		UPackage* package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *decoratorsPath, *name));
		if (!package)
			continue;

		UBlueprint* bp = FKismetEditorUtilities::CreateBlueprint(
			UMounteaDialogueDecoratorBase::StaticClass(),
			package,
			FName(*name),
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass());

		if (!bp || !bp->GeneratedClass)
			continue;

		// Add member variables for each property
		const TArray<TSharedPtr<FJsonValue>>* propsArray;
		if (obj->TryGetArrayField(TEXT("properties"), propsArray))
			AddBlueprintPropertiesFromJson(bp, propsArray);

		FKismetEditorUtilities::CompileBlueprint(bp);

		if (UMounteaDialogueDecoratorBase* cdo = bp->GeneratedClass->GetDefaultObject<UMounteaDialogueDecoratorBase>())
		{
			cdo->DecoratorGUID = definitionGuid;
			cdo->DecoratorName = FText::FromString(name);
		}

		FAssetRegistryModule::AssetCreated(bp);
		bp->MarkPackageDirty();
		SaveAsset(bp);

		result.Add(definitionGuid, bp->GeneratedClass);
	}

	return result;
}

TMap<FGuid, UClass*> UMounteaDialogueSystemImportExportHelpers::CreateConditionBlueprints(const FString& Json, const FString& ProjectPackagePath)
{
	TMap<FGuid, UClass*> result;

	TArray<TSharedPtr<FJsonValue>> jsonArray;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(reader, jsonArray))
		return result;

	const FString conditionsPath = ProjectPackagePath / TEXT("Conditions");

	for (const auto& val : jsonArray)
	{
		TSharedPtr<FJsonObject> obj = val->AsObject();
		if (!obj.IsValid())
			continue;

		FString idStr, name;
		obj->TryGetStringField(TEXT("id"), idStr);
		obj->TryGetStringField(TEXT("name"), name);
		if (idStr.IsEmpty() || name.IsEmpty())
			continue;

		const FGuid definitionGuid = FGuid(idStr);
		if (result.Contains(definitionGuid))
			continue;

		UClass* existingClass = FindExistingConditionClass(definitionGuid, name, conditionsPath);
		if (existingClass)
		{
			result.Add(definitionGuid, existingClass);
			continue;
		}

		UPackage* package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *conditionsPath, *name));
		if (!package)
			continue;

		UBlueprint* bp = FKismetEditorUtilities::CreateBlueprint(
			UMounteaDialogueConditionBase::StaticClass(),
			package,
			FName(*name),
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass());

		if (!bp || !bp->GeneratedClass)
			continue;

		const TArray<TSharedPtr<FJsonValue>>* propsArray;
		if (obj->TryGetArrayField(TEXT("properties"), propsArray))
			AddBlueprintPropertiesFromJson(bp, propsArray);

		FKismetEditorUtilities::CompileBlueprint(bp);

		if (UMounteaDialogueConditionBase* cdo = bp->GeneratedClass->GetDefaultObject<UMounteaDialogueConditionBase>())
		{
			cdo->SetConditionGUID(definitionGuid);
			cdo->ConditionName = FName(*name);
		}

		FAssetRegistryModule::AssetCreated(bp);
		bp->MarkPackageDirty();
		SaveAsset(bp);

		result.Add(definitionGuid, bp->GeneratedClass);
	}

	return result;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateGraphFromExtractedFiles(UMounteaDialogueGraph* Graph, const TMap<FString, FString>& ExtractedFiles, const FString& SourceFilePath)
{
	// ── Phase 0: Decorator / Condition Blueprint classes ──────────────────────────

	const FString projectPackagePath = FPackageName::GetLongPackagePath(Graph->GetOuter()->GetPathName());

	TMap<FGuid, UClass*> decoratorClasses;
	if (ExtractedFiles.Contains(TEXT("decorators.json")))
		decoratorClasses = CreateDecoratorBlueprints(ExtractedFiles[TEXT("decorators.json")], projectPackagePath);

	TMap<FGuid, UClass*> conditionClasses;
	if (ExtractedFiles.Contains(TEXT("conditions.json")))
		conditionClasses = CreateConditionBlueprints(ExtractedFiles[TEXT("conditions.json")], projectPackagePath);

	// ── Phase 1: Pre-graph assets ─────────────────────────────────────────────────

	if (!PopulateDialogueData(Graph, SourceFilePath, ExtractedFiles))
		return false;

	const TMap<FString, FString> tagMap = RegisterGameplayTags(Graph, ExtractedFiles["participants.json"]);

	FString defaultLocale;
	TMap<FString, FString> stringTableLookup;
	TSharedPtr<FJsonObject> entriesObj;
	BuildStringTableLookup(ExtractedFiles, defaultLocale, stringTableLookup, entriesObj);

	const TMap<FString, FString> rowIdToTextKey = BuildRowIdToTextKeyMap(ExtractedFiles["dialogueRows.json"]);

	FAssetToolsModule& assetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& assetTools = assetToolsModule.Get();

	UStringTable* stDialogueRows = nullptr;
	UStringTable* stNodes = nullptr;
	if (!CreateGraphStringTables(Graph, assetTools, ExtractedFiles, stringTableLookup, rowIdToTextKey, stDialogueRows, stNodes))
		return false;

	// Record both string tables in the import config so reimport can track them.
	// The underlying source for both is stringTable.json (or nodes.json/dialogueRows.json for
	// the node-title table). We store stringTable.json as the canonical source for both.
	if (ExtractedFiles.Contains(TEXT("stringTable.json")))
	{
		const FString stPackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
		const FString& stJson = ExtractedFiles[TEXT("stringTable.json")];
		UpdateGraphImportDataConfig(Graph, TEXT("stringTable.json"), stJson, stPackagePath,
			FString::Printf(TEXT("ST_%s_DialogueRows"), *Graph->GetName()));
		UpdateGraphImportDataConfig(Graph, TEXT("stringTable.json"), stJson, stPackagePath,
			FString::Printf(TEXT("ST_%s_Nodes"), *Graph->GetName()));
	}

	UDataTable* dtParticipants = nullptr;
	UDataTable* dtDialogueRows = nullptr;
	if (!CreateGraphDataTables(Graph, assetTools, dtParticipants, dtDialogueRows))
		return false;

	const TMap<FGuid, USoundWave*> audioMap = CreateAudioAssets(ExtractedFiles, Graph->GetOuter());

	// Import per-dialogue thumbnails (Thumbnails/*.png inside standalone .mnteadlg archives)
	CreateTextureAssets(ExtractedFiles, FPackageName::GetLongPackagePath(Graph->GetPathName()));

	// ── Phase 2: Graph population ─────────────────────────────────────────────────

	TMap<FGuid, UMounteaDialogueGraphNode*> spawnedNodes;
	if (!PopulateNodes(Graph, ExtractedFiles["nodes.json"], spawnedNodes, decoratorClasses))
		return false;

	if (!PopulateEdges(Graph, ExtractedFiles["edges.json"], conditionClasses))
		return false;

	ResolveReturnToNodeTargets(Graph, spawnedNodes);

	// ── Phase 3: Fill data tables ─────────────────────────────────────────────────

	if (!FillParticipantsDataTable(Graph, dtParticipants, ExtractedFiles["participants.json"], tagMap))
		return false;

	if (!FillDialogueRowsDataTable(Graph, dtDialogueRows, dtParticipants, stDialogueRows, stNodes, ExtractedFiles, rowIdToTextKey, audioMap))
		return false;

	if (entriesObj.IsValid())
	{
		ExportLocalizationPoFiles(
			FString::Printf(TEXT("ST_%s_DialogueRows"), *Graph->GetName()),
			defaultLocale,
			stringTableLookup,
			entriesObj);
	}

	return true;
}

TMap<FGuid, USoundWave*> UMounteaDialogueSystemImportExportHelpers::CreateAudioAssets(const TMap<FString, FString>& ExtractedFiles, UObject* InParent)
{
	TMap<FGuid, USoundWave*> audioMap;

	if (!InParent)
		return audioMap;

	auto IsSupportedAudioFile = [](const FString& path) -> bool
	{
		return path.EndsWith(TEXT(".wav"))
			|| path.EndsWith(TEXT(".mp3"))
			|| path.EndsWith(TEXT(".ogg"))
			|| path.EndsWith(TEXT(".flac"));
	};

	// Early-out if the archive has no audio files — avoids creating an empty content folder
	bool bHasAudio = false;
	for (const auto& file : ExtractedFiles)
	{
		if (file.Key.StartsWith(TEXT("audio/")) && IsSupportedAudioFile(file.Key))
		{
			bHasAudio = true;
			break;
		}
	}
	if (!bHasAudio)
		return audioMap;

	FModuleManager::LoadModuleChecked<IAudioEditorModule>("AudioEditor");
	IAudioEditorModule& audioEditorModule = FModuleManager::GetModuleChecked<IAudioEditorModule>("AudioEditor");

	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FString packagePath = FPackageName::GetLongPackagePath(InParent->GetPathName());

	// Create the audio content folder once, only when there are files to import
	{
		FString audioFolderPath = FPaths::Combine(FPaths::ProjectContentDir(), packagePath.RightChop(6), TEXT("audio"));
		FPaths::MakeStandardFilename(audioFolderPath);
		if (!FPaths::DirectoryExists(audioFolderPath))
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*audioFolderPath);
	}

	for (const auto& file : ExtractedFiles)
	{
		if (!file.Key.StartsWith(TEXT("audio/")) || !IsSupportedAudioFile(file.Key))
			continue;

		const FString& tempPath = file.Value;

		FString relativePath = file.Key;
		relativePath.RemoveFromStart(TEXT("audio/"));
		const FString subfolderPath = FPaths::GetPath(relativePath);
		const FGuid rowGuid = FGuid(FPaths::GetBaseFilename(subfolderPath));

		const FString relativePackagePath = FString::Printf(TEXT("/audio/%s"), *subfolderPath);
		const FString fullPackagePath = packagePath / relativePackagePath / FPaths::GetBaseFilename(file.Key);

		FARFilter filter;
		filter.PackagePaths.Add(FName(*(packagePath + relativePackagePath)));
		filter.ClassPaths.Add(USoundWave::StaticClass()->GetClassPathName());

		TArray<FAssetData> assetDataList;
		assetRegistryModule.Get().GetAssets(filter, assetDataList);

			USoundWave* importedSound = ImportSingleAudioAsset(
			tempPath,
			fullPackagePath,
			FPaths::GetBaseFilename(file.Key),
			packagePath + relativePackagePath,
			audioEditorModule,
			assetRegistryModule);

		if (importedSound)
			audioMap.Add(rowGuid, importedSound);
		else
			EditorLOG_WARNING(TEXT("[CreateAudioAssets] Failed to import audio: %s"), *tempPath);
	}

	return audioMap;
}

USoundWave* UMounteaDialogueSystemImportExportHelpers::ImportSingleAudioAsset(
	const FString& TempPath,
	const FString& FullPackagePath,
	const FString& AssetName,
	const FString& SearchPackagePath,
	IAudioEditorModule& AudioEditorModule,
	FAssetRegistryModule& AssetRegistryModule)
{
	FARFilter filter;
	filter.PackagePaths.Add(FName(*SearchPackagePath));
	filter.ClassPaths.Add(USoundWave::StaticClass()->GetClassPathName());

	TArray<FAssetData> assetDataList;
	AssetRegistryModule.Get().GetAssets(filter, assetDataList);

	UPackage* soundPackage = nullptr;
	USoundWave* importedSound = nullptr;

	if (assetDataList.Num() > 0)
	{
		if (USoundWave* existing = Cast<USoundWave>(assetDataList[0].GetAsset()))
		{
			soundPackage = existing->GetOutermost();
			soundPackage->FullyLoad();
			importedSound = AudioEditorModule.ImportSoundWave(soundPackage, existing->GetName(), TempPath);
		}
	}
	else
	{
		soundPackage = CreatePackage(*FullPackagePath);
		soundPackage->FullyLoad();
		importedSound = AudioEditorModule.ImportSoundWave(soundPackage, AssetName, TempPath);
	}

	if (soundPackage)
		soundPackage->FullyLoad();

	if (importedSound)
	{
		FAssetRegistryModule::AssetCreated(importedSound);
		SaveAsset(importedSound);
	}

	return importedSound;
}

void UMounteaDialogueSystemImportExportHelpers::CreateTextureAssets(const TMap<FString, FString>& ExtractedFiles, const FString& DestPackagePath)
{
	FAssetToolsModule& assetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& assetTools = assetToolsModule.Get();

	const FString thumbnailsPackagePath = DestPackagePath / TEXT("thumbnails");

	for (const auto& file : ExtractedFiles)
	{
		const bool bIsThumbnailPng =
			(file.Key.StartsWith(TEXT("Thumbnails/")) || file.Key.StartsWith(TEXT("thumbnails/")))
			&& file.Key.EndsWith(TEXT(".png"));

		if (!bIsThumbnailPng)
			continue;

		const FString& tempPath = file.Value;
		if (tempPath.IsEmpty() || !FPaths::FileExists(tempPath))
		{
			EditorLOG_WARNING(TEXT("[CreateTextureAssets] Temp file missing for '%s'"), *file.Key);
			continue;
		}

		const FString assetName = FPaths::GetBaseFilename(file.Key);

		// Re-use existing asset if already imported
		FAssetRegistryModule& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		const FSoftObjectPath existingPath = thumbnailsPackagePath / assetName + TEXT(".") + assetName;
		const FAssetData existingData = assetRegistry.Get().GetAssetByObjectPath(existingPath);
		if (existingData.IsValid())
		{
			// Asset already exists — skip re-import to preserve any manual tweaks
			EditorLOG_INFO(TEXT("[CreateTextureAssets] Texture '%s' already exists — skipped"), *assetName);
			continue;
		}

		UAssetImportTask* importTask = NewObject<UAssetImportTask>();
		importTask->Filename        = tempPath;
		importTask->DestinationPath = thumbnailsPackagePath;
		importTask->DestinationName = assetName;
		importTask->bReplaceExisting= true;
		importTask->bSave           = true;

		TArray<UAssetImportTask*> tasks = { importTask };
		assetTools.ImportAssetTasks(tasks);

		if (importTask->GetObjects().Num() == 0)
			EditorLOG_WARNING(TEXT("[CreateTextureAssets] Failed to import texture: %s"), *file.Key);
	}
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateDialogueData(UMounteaDialogueGraph* Graph, const FString& SourceFilePath, const TMap<FString, FString>& ExtractedFiles)
{
	const FString DialogueDataJson = ExtractedFiles["dialogueData.json"];
	const FString newSourceFilePath = SourceFilePath;

	Graph->SourceFile.Empty();
	Graph->SourceData.Empty();
	
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DialogueDataJson);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		Graph->SetGraphGUID(FGuid(JsonObject->GetStringField(TEXT("dialogueGuid"))));
	else
	{
		EditorLOG_ERROR(TEXT("[PopulateDialogueData] Failed to parse dialogueData.json during population"));
		return false;
	}
	
	Graph->SourceFile = newSourceFilePath;
	for (const auto& Itr : ExtractedFiles)
	{
		FDialogueImportData NewSourceData;
		NewSourceData.JsonFile = Itr.Key;
		NewSourceData.JsonData = Itr.Value;
		Graph->SourceData.Add(NewSourceData);
	}

	const FString UpdatedConfigFile = UMounteaDialogueImportConfig::GetImportConfigFilePath();

	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();

	if (FPaths::FileExists(UpdatedConfigFile))
		importConfig->LoadConfig(nullptr, *UpdatedConfigFile);
	else
	{
		IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*FPaths::GetPath(UpdatedConfigFile));
		importConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);
	}

	const FString PackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString AssetName = *Graph->GetName();
	const FString GraphPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);
	
	if (importConfig)
	{
		FDialogueImportSourceData importSourceData;
		importSourceData.DialogueAssetPath = GraphPath;
		importSourceData.DialogueSourcePath = SourceFilePath;
		importSourceData.ImportedAt = FDateTime::Now();
		importConfig->WriteToConfig(Graph->GetGraphGUID(), importSourceData);

		importConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);
	}

	return true;
}

TMap<FString, FString> UMounteaDialogueSystemImportExportHelpers::RegisterGameplayTags(const UMounteaDialogueGraph* Graph, const FString& ParticipantsJson)
{
	TMap<FString, FString> participantTagMap;

	TArray<TSharedPtr<FJsonValue>> participantsArray;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(ParticipantsJson);
	if (!FJsonSerializer::Deserialize(reader, participantsArray))
	{
		EditorLOG_ERROR(TEXT("[RegisterGameplayTags] Failed to parse participants.json"));
		return participantTagMap;
	}

	TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	if (!plugin.IsValid())
	{
		EditorLOG_ERROR(TEXT("[RegisterGameplayTags] MounteaDialogueSystem plugin not found — cannot write participant tags"));
		return participantTagMap;
	}

	const FString tagsDir = FPaths::Combine(plugin->GetBaseDir(), TEXT("Config"), TEXT("Tags"));
	const FString tagsIniPath = FPaths::Combine(tagsDir, TEXT("MounteaDialogueSystemTags.ini"));
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

	for (const auto& participantValue : participantsArray)
	{
		TSharedPtr<FJsonObject> participant = participantValue->AsObject();
		if (!participant.IsValid())
			continue;

		FString pName;
		FString pFullPath;
		participant->TryGetStringField(TEXT("name"), pName);
		if (!participant->TryGetStringField(TEXT("fullPath"), pFullPath))
			participant->TryGetStringField(TEXT("category"), pFullPath);

		if (pName.IsEmpty())
			continue;

		const FString fullTag = pFullPath.IsEmpty()
			? FString::Printf(TEXT("Mountea_Dialogue.%s"), *pName)
			: FString::Printf(TEXT("Mountea_Dialogue.%s.%s"), *pFullPath, *pName);

		participantTagMap.Add(pName, fullTag);

		if (!existingTags.Contains(fullTag))
		{
			if (!fileContent.EndsWith(TEXT("\n")))
				fileContent.AppendChar(TEXT('\n'));
			fileContent.Append(FString::Printf(
				TEXT("GameplayTagList=(Tag=\"%s\",DevComment=\"Mountea Dialogue Participant: %s\")\n"),
				*fullTag, *pName));
			existingTags.Add(fullTag);
			bAnyAdded = true;
		}
	}

	if (bAnyAdded)
	{
		FFileHelper::SaveStringToFile(fileContent, *tagsIniPath);
		UGameplayTagsManager::Get().AddTagIniSearchPath(tagsDir);
		UGameplayTagsManager::Get().EditorRefreshGameplayTagTree();
	}

	return participantTagMap;
}

bool UMounteaDialogueSystemImportExportHelpers::FillParticipantsDataTable(const UMounteaDialogueGraph* Graph, UDataTable* ParticipantsTable, const FString& ParticipantsJson, const TMap<FString, FString>& ParticipantTagMap)
{
	if (!Graph || !ParticipantsTable)
	{
		EditorLOG_ERROR(TEXT("[FillParticipantsDataTable] Invalid Graph or DataTable"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> participantsArray;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(ParticipantsJson);
	if (!FJsonSerializer::Deserialize(reader, participantsArray))
	{
		EditorLOG_ERROR(TEXT("[FillParticipantsDataTable] Failed to parse participants.json"));
		return false;
	}

	// Clear stale rows so reimport starts fresh
	ParticipantsTable->EmptyTable();

	for (const auto& participantValue : participantsArray)
	{
		TSharedPtr<FJsonObject> participant = participantValue->AsObject();
		if (!participant.IsValid())
		{
			EditorLOG_WARNING(TEXT("[FillParticipantsDataTable] Invalid participant object — skipped"));
			continue;
		}

		FString name;
		participant->TryGetStringField(TEXT("name"), name);
		if (name.IsEmpty())
			continue;

		FDialogueParticipant newRow;
		newRow.ParticipantName = FName(*name);

		const FString* fullTagPtr = ParticipantTagMap.Find(name);
		if (fullTagPtr)
			newRow.ParticipantCategoryTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(**fullTagPtr), false);

		FString participantImageName;
		participant->TryGetStringField(TEXT("participantImage"), participantImageName);
		if (!participantImageName.IsEmpty())
		{
			const FString dialoguePackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
			const FString textureSoftPath = FString::Printf(TEXT("%s/thumbnails/%s.%s"),
				*dialoguePackagePath, *participantImageName, *participantImageName);
			newRow.ParticipantIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(textureSoftPath));
		}

		ParticipantsTable->AddRow(FName(*name), newRow);
	}

	const FString packagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString assetName = FString::Printf(TEXT("DT_%s_Participants"), *Graph->GetName());
	UpdateGraphImportDataConfig(Graph, TEXT("participants.json"), ParticipantsJson, packagePath, assetName);

	SaveAsset(ParticipantsTable);
	return true;
}

void UMounteaDialogueSystemImportExportHelpers::ResolveOpenChildGraphTargets(
	UMounteaDialogueGraph* graph,
	const TMap<FGuid, UMounteaDialogueGraph*>& knownGraphs)
{
	if (!IsValid(graph))
		return;

	bool bAnySaved = false;

	for (UMounteaDialogueGraphNode* node : graph->GetAllNodes())
	{
		UMounteaDialogueGraphNode_OpenChildGraph* openNode =
			Cast<UMounteaDialogueGraphNode_OpenChildGraph>(node);
		if (!openNode || !openNode->PendingTargetDialogueGUID.IsValid())
			continue;

		const FGuid pendingGuid = openNode->PendingTargetDialogueGUID;

		// 1. Check the caller-supplied map first (O(1), no I/O)
		UMounteaDialogueGraph* const* knownEntry = knownGraphs.Find(pendingGuid);
		if (knownEntry && *knownEntry)
		{
			openNode->TargetDialogue = TSoftObjectPtr<UMounteaDialogueGraph>(*knownEntry);
			openNode->PendingTargetDialogueGUID = FGuid();
			bAnySaved = true;
			continue;
		}

		// 2. Fall back to a full asset-registry scan (pre-existing graphs not in the batch)
		FAssetRegistryModule& assetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		FARFilter filter;
		filter.bRecursivePaths = true;
		filter.ClassPaths.Add(UMounteaDialogueGraph::StaticClass()->GetClassPathName());

		TArray<FAssetData> assetDataList;
		assetRegistryModule.Get().GetAssets(filter, assetDataList);

		bool bFound = false;
		for (const FAssetData& assetData : assetDataList)
		{
			UMounteaDialogueGraph* candidate = Cast<UMounteaDialogueGraph>(assetData.GetAsset());
			if (!candidate || candidate->GetGraphGUID() != pendingGuid)
				continue;

			openNode->TargetDialogue = TSoftObjectPtr<UMounteaDialogueGraph>(candidate);
			openNode->PendingTargetDialogueGUID = FGuid();
			bAnySaved = true;
			bFound = true;
			break;
		}

		if (!bFound)
		{
			EditorLOG_WARNING(
				TEXT("[ResolveOpenChildGraphTargets] No dialogue found for GUID '%s' (node in '%s')"),
				*pendingGuid.ToString(), *graph->GetName());
		}
	}

	if (bAnySaved)
		SaveAsset(graph);
}

UClass* UMounteaDialogueSystemImportExportHelpers::FindConditionClassByName(const FString& nameOrGUID)
{
	for (TObjectIterator<UClass> classIt; classIt; ++classIt)
	{
		if (!classIt->IsChildOf(UMounteaDialogueConditionBase::StaticClass()))
			continue;
		if (classIt->HasAnyClassFlags(CLASS_Abstract))
			continue;

		if (classIt->GetName().Contains(nameOrGUID))
			return *classIt;

		const UMounteaDialogueConditionBase* cdo = classIt->GetDefaultObject<UMounteaDialogueConditionBase>();
		if (!cdo)
			continue;
		if (cdo->GetConditionName() == nameOrGUID)
			return *classIt;
		if (cdo->GetConditionGUID().ToString() == nameOrGUID)
			return *classIt;
	}
	return nullptr;
}

UClass* UMounteaDialogueSystemImportExportHelpers::FindDecoratorClassByName(const FString& nameOrGUID)
{
	for (TObjectIterator<UClass> classIt; classIt; ++classIt)
	{
		if (!classIt->IsChildOf(UMounteaDialogueDecoratorBase::StaticClass()))
			continue;
		if (classIt->HasAnyClassFlags(CLASS_Abstract))
			continue;

		if (classIt->GetName().Contains(nameOrGUID))
			return *classIt;

		const UMounteaDialogueDecoratorBase* cdo = classIt->GetDefaultObject<UMounteaDialogueDecoratorBase>();
		if (!cdo)
			continue;
		if (cdo->GetDecoratorName().ToString() == nameOrGUID)
			return *classIt;
		if (cdo->GetDecoratorGUID().ToString() == nameOrGUID)
			return *classIt;
	}
	return nullptr;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateNodes(UMounteaDialogueGraph* Graph, const FString& Json, TMap<FGuid, UMounteaDialogueGraphNode*>& OutSpawnedNodes, const TMap<FGuid, UClass*>& DecoratorClasses)
{
	if (!IsValid(Graph) || !Graph->GetOutermost()->IsValidLowLevel())
	{
		EditorLOG_ERROR(TEXT("[PopulateNodes] Invalid Graph object or package provided to PopulateNodes"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> nodesJsonArray;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(reader, nodesJsonArray))
	{
		EditorLOG_ERROR(TEXT("[PopulateNodes] Failed to parse nodes.json"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> leadNodes, answerNodes, closeDialogueNodes, jumpToNodes, openChildGraphNodes, startNodes, delayNodes;

	for (const auto& nodeValue : nodesJsonArray)
	{
		TSharedPtr<FJsonObject> nodeObject = nodeValue->AsObject();
		if (!nodeObject.IsValid())
			continue;

		const FString nodeType = nodeObject->GetStringField(TEXT("type"));
		if (nodeType == TEXT("leadNode"))
			leadNodes.Add(nodeValue);
		else if (nodeType == TEXT("answerNode"))
			answerNodes.Add(nodeValue);
		else if (nodeType == TEXT("closeDialogueNode") || nodeType == TEXT("completeNode"))
			closeDialogueNodes.Add(nodeValue);
		else if (nodeType == TEXT("jumpToNode") || nodeType == TEXT("returnNode"))
			jumpToNodes.Add(nodeValue);
		else if (nodeType == TEXT("openChildGraphNode"))
			openChildGraphNodes.Add(nodeValue);
		else if (nodeType == TEXT("startNode"))
			startNodes.Add(nodeValue);
		else if (nodeType == TEXT("delayNode"))
			delayNodes.Add(nodeValue);
	}

	if (startNodes.Num() > 0)
	{
		if (Graph->GetStartNode())
		{
			PopulateNodeData(Graph->GetStartNode(), startNodes[0]->AsObject(), DecoratorClasses);
			OutSpawnedNodes.Add(Graph->GetStartNode()->GetNodeGUID(), Graph->GetStartNode());
		}
		else
		{
			if (UMounteaDialogueGraphNode_StartNode* newStart = Graph->ConstructDialogueNode<UMounteaDialogueGraphNode_StartNode>())
			{
				Graph->StartNode = newStart;
				PopulateNodeData(newStart, startNodes[0]->AsObject(), DecoratorClasses);
				OutSpawnedNodes.Add(newStart->GetNodeGUID(), newStart);
			}
		}
	}

	auto CreateNodes = [&](const TArray<TSharedPtr<FJsonValue>>& nodes, const TSubclassOf<UMounteaDialogueGraphNode> nodeClass)
	{
		for (const auto& nodeValue : nodes)
		{
			UMounteaDialogueGraphNode* newNode = Graph->ConstructDialogueNode(nodeClass);
			if (!newNode)
				continue;

			PopulateNodeData(newNode, nodeValue->AsObject(), DecoratorClasses);
			Graph->AllNodes.Add(newNode);
			OutSpawnedNodes.Add(newNode->GetNodeGUID(), newNode);
		}
	};

	CreateNodes(leadNodes,          UMounteaDialogueGraphNode_LeadNode::StaticClass());
	CreateNodes(answerNodes,        UMounteaDialogueGraphNode_AnswerNode::StaticClass());
	CreateNodes(closeDialogueNodes, UMounteaDialogueGraphNode_CompleteNode::StaticClass());
	CreateNodes(jumpToNodes,        UMounteaDialogueGraphNode_ReturnToNode::StaticClass());
	CreateNodes(openChildGraphNodes,UMounteaDialogueGraphNode_OpenChildGraph::StaticClass());
	CreateNodes(delayNodes,         UMounteaDialogueGraphNode_Delay::StaticClass());

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::ResolveReturnToNodeTargets(UMounteaDialogueGraph* Graph, const TMap<FGuid, UMounteaDialogueGraphNode*>& SpawnedNodesByGuid)
{
	if (!IsValid(Graph))
		return;

	for (UMounteaDialogueGraphNode* node : Graph->AllNodes)
	{
		UMounteaDialogueGraphNode_ReturnToNode* returnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(node);
		if (!returnNode || returnNode->SelectedNode)
			continue;

		FGuid pendingGuid;
		if (!FGuid::Parse(returnNode->SelectedNodeIndex, pendingGuid))
			continue;

		UMounteaDialogueGraphNode* const* found = SpawnedNodesByGuid.Find(pendingGuid);
		if (found && *found)
		{
			returnNode->SelectedNodeIndex = FString::FromInt(Graph->AllNodes.Find(*found));
			returnNode->SelectedNode = *found;
			returnNode->ReturnNodeUpdated.ExecuteIfBound();
		}
		else
		{
			EditorLOG_WARNING(
				TEXT("[ResolveReturnToNodeTargets] Target GUID '%s' not found in spawned nodes"),
				*pendingGuid.ToString());
		}
	}
}

void UMounteaDialogueSystemImportExportHelpers::PopulateNodeData(UMounteaDialogueGraphNode* Node, const TSharedPtr<FJsonObject>& JsonObject, const TMap<FGuid, UClass*>& DecoratorClasses)
{
	if (!Node || !JsonObject.IsValid())
		return;

	Node->SetNodeGUID(FGuid(JsonObject->GetStringField(TEXT("id"))));
	Node->ExecutionOrder = JsonObject->GetIntegerField(TEXT("executionOrder"));

	// Restore canvas position so nodes appear where the user placed them in Dialoguer
	const TSharedPtr<FJsonObject>* positionObject;
	if (JsonObject->TryGetObjectField(TEXT("position"), positionObject))
	{
		double posX = 0.0, posY = 0.0;
		(*positionObject)->TryGetNumberField(TEXT("x"), posX);
		(*positionObject)->TryGetNumberField(TEXT("y"), posY);
		Node->NodePosition = FIntPoint(FMath::RoundToInt(posX), FMath::RoundToInt(posY));
	}

	const TSharedPtr<FJsonObject> dataObject = JsonObject->GetObjectField(TEXT("data"));
	if (!dataObject.IsValid())
		return;

	// Dialoguer exports "label"; UE round-trips use "title"
	FString nodeTitle;
	if (!dataObject->TryGetStringField(TEXT("label"), nodeTitle))
		dataObject->TryGetStringField(TEXT("title"), nodeTitle);
	Node->NodeTitle = FText::FromString(nodeTitle);

	// Delay node — read duration
	double delayDuration = 1.0;
	if (dataObject->TryGetNumberField(TEXT("duration"), delayDuration))
	{
		if (UMounteaDialogueGraphNode_Delay* delayNode = Cast<UMounteaDialogueGraphNode_Delay>(Node))
			delayNode->SetDelayDuration(FMath::RoundToInt(delayDuration));
	}

	// Decorators — resolve by definition GUID (preferred) then by name fallback
	PopulateNodeDecorators(Node, dataObject, DecoratorClasses);

	// Return-to-node: read target GUID and store it raw in SelectedNodeIndex for deferred
	// resolution in PopulateNodes (where all nodes are guaranteed to exist).
	// Dialoguer: data.targetNode  |  UE round-trip: data.additionalInfo.targetNodeId
	if (UMounteaDialogueGraphNode_ReturnToNode* returnToNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(Node))
	{
		FString targetGuidStr;
		if (!dataObject->TryGetStringField(TEXT("targetNode"), targetGuidStr))
		{
			const TSharedPtr<FJsonObject>* additionalInfoPtr;
			if (dataObject->TryGetObjectField(TEXT("additionalInfo"), additionalInfoPtr))
				(*additionalInfoPtr)->TryGetStringField(TEXT("targetNodeId"), targetGuidStr);
		}

		if (!targetGuidStr.IsEmpty())
			returnToNode->SelectedNodeIndex = targetGuidStr;
	}

	// Open-child-graph: store pending target GUID for deferred resolution by ResolveOpenChildGraphTargets.
	// Dialoguer: data.targetDialogue is a GUID string.
	// UE round-trip: data.additionalInfo.targetDialogue is a soft asset path.
	if (UMounteaDialogueGraphNode_OpenChildGraph* openChildGraphNode = Cast<UMounteaDialogueGraphNode_OpenChildGraph>(Node))
	{
		FString targetDialogueStr;
		if (!dataObject->TryGetStringField(TEXT("targetDialogue"), targetDialogueStr))
		{
			const TSharedPtr<FJsonObject>* additionalInfoPtr;
			if (dataObject->TryGetObjectField(TEXT("additionalInfo"), additionalInfoPtr))
				(*additionalInfoPtr)->TryGetStringField(TEXT("targetDialogue"), targetDialogueStr);
		}

		if (!targetDialogueStr.IsEmpty())
		{
#if WITH_EDITORONLY_DATA
			FGuid targetGuid;
			if (FGuid::Parse(targetDialogueStr, targetGuid))
				openChildGraphNode->PendingTargetDialogueGUID = targetGuid;
			else
				openChildGraphNode->TargetDialogue = TSoftObjectPtr<UMounteaDialogueGraph>(FSoftObjectPath(targetDialogueStr));
#endif
		}
	}
}

void UMounteaDialogueSystemImportExportHelpers::PopulateNodeDecorators(UMounteaDialogueGraphNode* Node, const TSharedPtr<FJsonObject>& DataObject, const TMap<FGuid, UClass*>& DecoratorClasses)
{
	const TArray<TSharedPtr<FJsonValue>>* decoratorsArray;
	if (!DataObject->TryGetArrayField(TEXT("decorators"), decoratorsArray))
		return;

	for (const auto& decorVal : *decoratorsArray)
	{
		TSharedPtr<FJsonObject> decorObj = decorVal->AsObject();
		if (!decorObj.IsValid())
			continue;

		FString decorName;
		FString decorIdStr;
		decorObj->TryGetStringField(TEXT("name"), decorName);
		decorObj->TryGetStringField(TEXT("id"), decorIdStr);

		// Try GUID-based lookup first (works for Blueprint classes created during Phase 0)
		UClass* decorClass = nullptr;
		if (!decorIdStr.IsEmpty())
			decorClass = DecoratorClasses.FindRef(FGuid(decorIdStr));

		// Fallback: name-based search for C++ decorators and old-format round-trips
		if (!decorClass)
			decorClass = FindDecoratorClassByName(decorName);

		if (!decorClass)
		{
			EditorLOG_WARNING(TEXT("[PopulateNodeDecorators] No UE decorator class for '%s' (%s) — skipped"), *decorName, *decorIdStr);
			continue;
		}

		FMounteaDialogueDecorator decorInst;
		decorInst.DecoratorType = NewObject<UMounteaDialogueDecoratorBase>(Node, decorClass);

		if (decorInst.DecoratorType)
		{
			// Fresh per-instance GUID (definition GUID lives on the class CDO)
			decorInst.DecoratorType->DecoratorGUID = FGuid::NewGuid();

			// Apply exported property values
			const TSharedPtr<FJsonObject>* valuesPtr;
			if (decorObj->TryGetObjectField(TEXT("values"), valuesPtr))
			{
				for (const auto& kvp : (*valuesPtr)->Values)
				{
					FProperty* prop = decorInst.DecoratorType->GetClass()->FindPropertyByName(FName(*kvp.Key));
					if (prop)
					{
						FString valueStr;
						kvp.Value->TryGetString(valueStr);
						prop->ImportText_InContainer(*valueStr, decorInst.DecoratorType, decorInst.DecoratorType, PPF_None);
					}
				}
			}
		}

		Node->NodeDecorators.Add(decorInst);
	}
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateEdges(UMounteaDialogueGraph* Graph, const FString& Json, const TMap<FGuid, UClass*>& ConditionClasses)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[PopulateEdges] Invalid Graph object provided to PopulateEdges"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> dialogueRowsJsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, dialogueRowsJsonArray))
	{
		EditorLOG_ERROR(TEXT("[PopulateEdges] Failed to parse edges.json"));
		return false;
	}

	int32 EdgesCreated = 0;

	for (const auto& EdgeValue : dialogueRowsJsonArray)
	{
		TSharedPtr<FJsonObject> EdgeObject = EdgeValue->AsObject();
		if (!EdgeObject.IsValid())
			continue;

		FString SourceID = EdgeObject->GetStringField(TEXT("source"));
		FString TargetID = EdgeObject->GetStringField(TEXT("target"));

		UMounteaDialogueGraphNode* SourceNode = Graph->FindNodeByGuid(FGuid(SourceID));
		UMounteaDialogueGraphNode* TargetNode = Graph->FindNodeByGuid(FGuid(TargetID));

		if (!SourceNode || !TargetNode)
		{
			EditorLOG_WARNING(TEXT("[PopulateEdges] Could not find source or target node for edge: %s -> %s"), *SourceID, *TargetID);
			continue;
		}

		UMounteaDialogueGraphEdge* NewEdge = NewObject<UMounteaDialogueGraphEdge>(Graph);
		if (NewEdge)
		{
			NewEdge->Graph = Graph;
			NewEdge->StartNode = SourceNode;
			NewEdge->EndNode = TargetNode;

			SourceNode->ChildrenNodes.AddUnique(TargetNode);
			SourceNode->Edges.Add(TargetNode, NewEdge);
			TargetNode->ParentNodes.AddUnique(SourceNode);

			// For reimport: update the EdNode's edge reference so that conditions set below
			// survive the RebuildMounteaDialogueGraph call that follows PopulateGraphFromExtractedFiles.
			// Without this, Clear() empties Node->Edges and RebuildMounteaDialogueGraph restores the
			// OLD blank edges that the EdNodes reference.
			if (UEdGraph_MounteaDialogueGraph* edGraph = Cast<UEdGraph_MounteaDialogueGraph>(Graph->EdGraph))
			{
				for (UEdGraphNode* edNode : edGraph->Nodes)
				{
					UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(edNode);
					if (!edgeNode || !edgeNode->MounteaDialogueGraphEdge)
						continue;
					const UMounteaDialogueGraphEdge* oldEdge = edgeNode->MounteaDialogueGraphEdge;
					if (oldEdge->StartNode && oldEdge->StartNode->GetNodeGUID() == SourceNode->GetNodeGUID()
						&& oldEdge->EndNode && oldEdge->EndNode->GetNodeGUID() == TargetNode->GetNodeGUID())
					{
						edgeNode->MounteaDialogueGraphEdge = NewEdge;
						break;
					}
				}
			}

			// Parse optional edge conditions
			ParseEdgeConditions(NewEdge, EdgeObject, ConditionClasses);

			EdgesCreated++;
		}
		else
			EditorLOG_ERROR(TEXT("[PopulateEdges] Failed to create edge object for: %s -> %s"), *SourceID, *TargetID);
	}

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::ParseEdgeConditions(UMounteaDialogueGraphEdge* Edge, const TSharedPtr<FJsonObject>& EdgeObject, const TMap<FGuid, UClass*>& ConditionClasses)
{
	const TSharedPtr<FJsonObject>* dataObjPtr;
	if (!EdgeObject->TryGetObjectField(TEXT("data"), dataObjPtr))
		return;

	const TSharedPtr<FJsonObject>* condObjPtr;
	if (!(*dataObjPtr)->TryGetObjectField(TEXT("conditions"), condObjPtr))
		return;

	FString modeStr;
	(*condObjPtr)->TryGetStringField(TEXT("mode"), modeStr);
	Edge->EdgeConditions.Mode = modeStr.Equals(TEXT("any"), ESearchCase::IgnoreCase)
		? EConditionEvaluationMode::Any
		: EConditionEvaluationMode::All;

	const TArray<TSharedPtr<FJsonValue>>* rulesArray;
	if (!(*condObjPtr)->TryGetArrayField(TEXT("rules"), rulesArray))
		return;

	for (const auto& ruleVal : *rulesArray)
	{
		const TSharedPtr<FJsonObject> ruleObj = ruleVal->AsObject();
		if (!ruleObj.IsValid())
			continue;

		FString condName;
		FString condIdStr;
		bool bNegate = false;
		ruleObj->TryGetStringField(TEXT("name"), condName);
		ruleObj->TryGetStringField(TEXT("id"), condIdStr);
		ruleObj->TryGetBoolField(TEXT("negate"), bNegate);

		// Try GUID-based lookup first (Blueprint classes created in Phase 0)
		UClass* condClass = nullptr;
		if (!condIdStr.IsEmpty())
			condClass = ConditionClasses.FindRef(FGuid(condIdStr));

		// Fallback: name-based search for C++ conditions and old-format round-trips
		if (!condClass)
			condClass = FindConditionClassByName(condName);

		if (!condClass)
		{
			EditorLOG_WARNING(
				TEXT("[ParseEdgeConditions] No UE condition class for '%s' (%s) — using base class as placeholder"),
				*condName, *condIdStr);
			condClass = UMounteaDialogueConditionBase::StaticClass();
		}

		FMounteaDialogueCondition condInst;
		condInst.ConditionClass = NewObject<UMounteaDialogueConditionBase>(Edge, condClass);
		condInst.bNegate = bNegate;
		if (condInst.ConditionClass)
		{
			// Preserve the original Dialoguer name so the user knows what to replace it with
			if (!condName.IsEmpty())
				condInst.ConditionClass->ConditionName = FName(*condName);
			if (!condIdStr.IsEmpty())
				condInst.ConditionClass->SetConditionGUID(FGuid(condIdStr));

			// Apply exported property values
			const TSharedPtr<FJsonObject>* valuesPtr;
			if (ruleObj->TryGetObjectField(TEXT("values"), valuesPtr))
			{
				for (const auto& kvp : (*valuesPtr)->Values)
				{
					FProperty* prop = condInst.ConditionClass->GetClass()->FindPropertyByName(FName(*kvp.Key));
					if (prop)
					{
						FString valueStr;
						kvp.Value->TryGetString(valueStr);
						prop->ImportText_InContainer(*valueStr, condInst.ConditionClass, condInst.ConditionClass, PPF_None);
					}
				}
			}
		}

		Edge->EdgeConditions.Rules.Add(condInst);
	}
}

void UMounteaDialogueSystemImportExportHelpers::BuildStringTableLookup(const TMap<FString, FString>& ExtractedFiles, FString& OutDefaultLocale, TMap<FString, FString>& OutLookup, TSharedPtr<FJsonObject>& OutEntriesObject)
{
	OutDefaultLocale = TEXT("en");
	OutLookup.Reset();
	OutEntriesObject.Reset();

	if (!ExtractedFiles.Contains(TEXT("stringTable.json")))
		return;

	TSharedPtr<FJsonObject> stRoot;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(ExtractedFiles[TEXT("stringTable.json")]);
	if (!FJsonSerializer::Deserialize(reader, stRoot) || !stRoot.IsValid())
		return;

	stRoot->TryGetStringField(TEXT("defaultLocale"), OutDefaultLocale);

	const TSharedPtr<FJsonObject>* entriesPtr;
	if (!stRoot->TryGetObjectField(TEXT("entries"), entriesPtr))
		return;

	OutEntriesObject = *entriesPtr;

	for (const auto& entry : OutEntriesObject->Values)
	{
		const TSharedPtr<FJsonObject> localeMap = entry.Value->AsObject();
		if (!localeMap.IsValid())
			continue;

		FString text;
		if (!localeMap->TryGetStringField(*OutDefaultLocale, text))
		{
			// Fall back to first available locale
			for (const auto& locPair : localeMap->Values)
			{
				text = locPair.Value->AsString();
				break;
			}
		}
		OutLookup.Add(entry.Key, text);
	}
}

TMap<FString, FString> UMounteaDialogueSystemImportExportHelpers::BuildRowIdToTextKeyMap(const FString& DialogueRowsJson)
{
	TMap<FString, FString> rowIdToTextKey;

	TArray<TSharedPtr<FJsonValue>> rowsArray;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(DialogueRowsJson);
	if (!FJsonSerializer::Deserialize(reader, rowsArray))
		return rowIdToTextKey;

	for (const auto& rowValue : rowsArray)
	{
		const TSharedPtr<FJsonObject> rowObj = rowValue->AsObject();
		if (!rowObj.IsValid())
			continue;

		FString rowId;
		FString textKey;
		rowObj->TryGetStringField(TEXT("id"), rowId);
		if (!rowObj->TryGetStringField(TEXT("textKey"), textKey) || textKey.IsEmpty())
			textKey = rowId;

		if (!rowId.IsEmpty())
			rowIdToTextKey.Add(rowId, textKey);
	}

	return rowIdToTextKey;
}

bool UMounteaDialogueSystemImportExportHelpers::CreateGraphStringTables(UMounteaDialogueGraph* Graph, IAssetTools& AssetTools, const TMap<FString, FString>& ExtractedFiles, const TMap<FString, FString>& StringTableLookup, const TMap<FString, FString>& RowIdToTextKey, UStringTable*& OutDialogueRowsStringTable, UStringTable*& OutNodesStringTable)
{
	if (!IsValid(Graph))
	{
		EditorLOG_ERROR(TEXT("[CreateGraphStringTables] Invalid Graph"));
		return false;
	}

	const FString packagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString stRowsName  = FString::Printf(TEXT("ST_%s_DialogueRows"), *Graph->GetName());
	const FString stNodesName = FString::Printf(TEXT("ST_%s_Nodes"), *Graph->GetName());

	// Parse rows and nodes JSON once for the lambdas below
	TArray<TSharedPtr<FJsonValue>> rowsArray;
	{
		TSharedRef<TJsonReader<>> r = TJsonReaderFactory<>::Create(ExtractedFiles["dialogueRows.json"]);
		FJsonSerializer::Deserialize(r, rowsArray);
	}

	TArray<TSharedPtr<FJsonValue>> nodesArray;
	{
		TSharedRef<TJsonReader<>> r = TJsonReaderFactory<>::Create(ExtractedFiles["nodes.json"]);
		FJsonSerializer::Deserialize(r, nodesArray);
	}

	OutDialogueRowsStringTable = CreateStringTable(AssetTools, packagePath, stRowsName, [&](UStringTable* table)
	{
		for (const auto& rowValue : rowsArray)
		{
			const TSharedPtr<FJsonObject> rowObj = rowValue->AsObject();
			if (!rowObj.IsValid())
				continue;

			FString rowId;
			rowObj->TryGetStringField(TEXT("id"), rowId);
			const FString tableKey = RowIdToTextKey.FindRef(rowId);

			FString rowText;
			if (!rowObj->TryGetStringField(TEXT("text"), rowText) || rowText.IsEmpty())
				rowText = StringTableLookup.FindRef(tableKey);

			if (!tableKey.IsEmpty())
				table->GetMutableStringTable()->SetSourceString(tableKey, rowText);
		}
	});

	OutNodesStringTable = CreateStringTable(AssetTools, packagePath, stNodesName, [&](UStringTable* table)
	{
		for (const auto& nodeValue : nodesArray)
		{
			const TSharedPtr<FJsonObject> nodeObj = nodeValue->AsObject();
			if (!nodeObj.IsValid())
				continue;

			FString nodeId;
			nodeObj->TryGetStringField(TEXT("id"), nodeId);

			const TSharedPtr<FJsonObject>* dataPtr;
			if (!nodeObj->TryGetObjectField(TEXT("data"), dataPtr))
				continue;

			FString displayName;
			FString displayNameKey;
			if ((*dataPtr)->TryGetStringField(TEXT("displayNameKey"), displayNameKey))
				displayName = StringTableLookup.FindRef(displayNameKey);

			if (displayName.IsEmpty())
			{
				const TSharedPtr<FJsonObject>* additionalInfoPtr;
				if ((*dataPtr)->TryGetObjectField(TEXT("additionalInfo"), additionalInfoPtr))
					(*additionalInfoPtr)->TryGetStringField(TEXT("displayName"), displayName);
			}

			if (!displayName.IsEmpty())
				table->GetMutableStringTable()->SetSourceString(nodeId, displayName);
		}
	});

	if (!OutDialogueRowsStringTable || !OutNodesStringTable)
	{
		EditorLOG_ERROR(TEXT("[CreateGraphStringTables] Failed to create one or more string tables for '%s'"), *Graph->GetName());
		return false;
	}

	SaveAsset(OutDialogueRowsStringTable);
	SaveAsset(OutNodesStringTable);
	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::CreateGraphDataTables(UMounteaDialogueGraph* Graph, IAssetTools& AssetTools, UDataTable*& OutParticipantsTable, UDataTable*& OutDialogueRowsTable)
{
	if (!IsValid(Graph))
	{
		EditorLOG_ERROR(TEXT("[CreateGraphDataTables] Invalid Graph"));
		return false;
	}

	const FString packagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());

	OutParticipantsTable = CreateDataTable<FDialogueParticipant>(
		AssetTools, packagePath,
		FString::Printf(TEXT("DT_%s_Participants"), *Graph->GetName()));

	OutDialogueRowsTable = CreateDataTable<FDialogueRow>(
		AssetTools, packagePath,
		FString::Printf(TEXT("DT_%s_DialogueRows"), *Graph->GetName()));

	if (!OutParticipantsTable || !OutDialogueRowsTable)
	{
		EditorLOG_ERROR(TEXT("[CreateGraphDataTables] Failed to create one or more data tables for '%s'"), *Graph->GetName());
		return false;
	}

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::FillDialogueRowsDataTable(UMounteaDialogueGraph* Graph, UDataTable* DialogueRowsTable, UDataTable* ParticipantsTable, UStringTable* DialogueRowsStringTable, UStringTable* NodesStringTable, const TMap<FString, FString>& ExtractedFiles, const TMap<FString, FString>& RowIdToTextKey, const TMap<FGuid, USoundWave*>& AudioMap)
{
	if (!IsValid(Graph) || !DialogueRowsTable || !ParticipantsTable || !DialogueRowsStringTable || !NodesStringTable)
	{
		EditorLOG_ERROR(TEXT("[FillDialogueRowsDataTable] One or more required assets are null"));
		return false;
	}

	const FString dialogueRowsJson = ExtractedFiles["dialogueRows.json"];
	const FString dialogueNodesJson = ExtractedFiles["nodes.json"];

	TArray<TSharedPtr<FJsonValue>> rowsArray;
	{
		TSharedRef<TJsonReader<>> r = TJsonReaderFactory<>::Create(dialogueRowsJson);
		if (!FJsonSerializer::Deserialize(r, rowsArray))
		{
			EditorLOG_ERROR(TEXT("[FillDialogueRowsDataTable] Failed to parse dialogueRows.json"));
			return false;
		}
	}

	TArray<TSharedPtr<FJsonValue>> nodesArray;
	{
		TSharedRef<TJsonReader<>> r = TJsonReaderFactory<>::Create(dialogueNodesJson);
		if (!FJsonSerializer::Deserialize(r, nodesArray))
		{
			EditorLOG_ERROR(TEXT("[FillDialogueRowsDataTable] Failed to parse nodes.json"));
			return false;
		}
	}

	// Clear stale rows so reimport starts fresh
	DialogueRowsTable->EmptyTable();

	const TMap<FString, FString> nodeParticipantMap = BuildNodeParticipantMap(nodesArray);

	// Group dialogue rows by nodeId
	TMap<FString, TArray<TSharedPtr<FJsonObject>>> groupedRows;
	for (const auto& rowValue : rowsArray)
	{
		const TSharedPtr<FJsonObject> rowObj = rowValue->AsObject();
		if (!rowObj.IsValid())
			continue;

		FString nodeId;
		rowObj->TryGetStringField(TEXT("nodeId"), nodeId);
		groupedRows.FindOrAdd(nodeId).Add(rowObj);
	}

	for (const auto& group : groupedRows)
	{
		const FString* participantNamePtr = nodeParticipantMap.Find(group.Key);
		if (!participantNamePtr || participantNamePtr->IsEmpty())
		{
			EditorLOG_WARNING(TEXT("[FillDialogueRowsDataTable] No participant for node: %s — skipped"), *group.Key);
			continue;
		}

		ProcessDialogueRowGroup(
			Graph,
			group.Key,
			*participantNamePtr,
			group.Value,
			ParticipantsTable,
			DialogueRowsTable,
			DialogueRowsStringTable,
			NodesStringTable,
			RowIdToTextKey,
			AudioMap);
	}

	const FString packagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString dtName = FString::Printf(TEXT("DT_%s_DialogueRows"), *Graph->GetName());
	UpdateGraphImportDataConfig(Graph, TEXT("dialogueRows.json"), dialogueRowsJson, packagePath, dtName);

	SaveAsset(DialogueRowsTable);
	return true;
}

TMap<FString, FString> UMounteaDialogueSystemImportExportHelpers::BuildNodeParticipantMap(const TArray<TSharedPtr<FJsonValue>>& NodesArray)
{
	TMap<FString, FString> result;

	for (const auto& nodeValue : NodesArray)
	{
		const TSharedPtr<FJsonObject> nodeObj = nodeValue->AsObject();
		if (!nodeObj.IsValid())
			continue;

		FString nodeId;
		nodeObj->TryGetStringField(TEXT("id"), nodeId);

		const TSharedPtr<FJsonObject>* dataPtr;
		if (!nodeObj->TryGetObjectField(TEXT("data"), dataPtr))
			continue;

		FString participantName;
		if (!(*dataPtr)->TryGetStringField(TEXT("participant"), participantName))
		{
			const TSharedPtr<FJsonObject>* addInfoPtr;
			if ((*dataPtr)->TryGetObjectField(TEXT("additionalInfo"), addInfoPtr))
			{
				const TSharedPtr<FJsonObject>* participantObjPtr;
				if ((*addInfoPtr)->TryGetObjectField(TEXT("participant"), participantObjPtr))
					(*participantObjPtr)->TryGetStringField(TEXT("name"), participantName);
			}
		}

		if (!participantName.IsEmpty())
			result.Add(nodeId, participantName);
		else
			EditorLOG_WARNING(TEXT("[BuildNodeParticipantMap] No participant for node: %s"), *nodeId);
	}

	return result;
}

void UMounteaDialogueSystemImportExportHelpers::ProcessDialogueRowGroup(
	UMounteaDialogueGraph* Graph,
	const FString& NodeId,
	const FString& ParticipantName,
	const TArray<TSharedPtr<FJsonObject>>& Rows,
	UDataTable* ParticipantsTable,
	UDataTable* DialogueRowsTable,
	UStringTable* DialogueRowsStringTable,
	UStringTable* NodesStringTable,
	const TMap<FString, FString>& RowIdToTextKey,
	const TMap<FGuid, USoundWave*>& AudioMap)
{
	if (Rows.Num() == 0)
	{
		EditorLOG_WARNING(TEXT("[ProcessDialogueRowGroup] No rows for node: %s"), *NodeId);
		return;
	}

	FDialogueParticipant* participant = ParticipantsTable->FindRow<FDialogueParticipant>(FName(*ParticipantName), TEXT(""));
	if (!participant)
	{
		EditorLOG_WARNING(TEXT("[ProcessDialogueRowGroup] Participant '%s' not in table — skipped"), *ParticipantName);
		return;
	}

	FDialogueRow newRow;
	newRow.RowGUID = FGuid(NodeId);
	newRow.DialogueParticipant = FText::FromString(participant->ParticipantName.ToString());
	newRow.CompatibleTags.AddTag(participant->ParticipantCategoryTag);
	newRow.RowTitle = FText::FromStringTable(NodesStringTable->GetStringTableId(), NodeId);

	for (const auto& rowObj : Rows)
	{
		FString rowId;
		rowObj->TryGetStringField(TEXT("id"), rowId);
		const FString tableKey = RowIdToTextKey.FindRef(rowId);

		FDialogueRowData rowData;
		rowData.RowGUID = FGuid(rowId);
		rowData.RowText = FText::FromStringTable(DialogueRowsStringTable->GetStringTableId(), tableKey);
		rowData.RowDuration = rowObj->GetNumberField(TEXT("duration"));

		USoundWave* const* foundSound = AudioMap.Find(rowData.RowGUID);
		if (foundSound && *foundSound)
			rowData.RowSound = *foundSound;

		newRow.DialogueRowData.Add(rowData);
	}

	TArray<FDialogueRow*> existingRows;
	FString tmp;
	DialogueRowsTable->GetAllRows(tmp, existingRows);

	FString rowName = newRow.DialogueParticipant.ToString();
	rowName.Append(TEXT("_")).Append(FString::FromInt(existingRows.Num()));
	DialogueRowsTable->AddRow(FName(*rowName), newRow);

	// Link matching dialogue nodes to this row
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

void UMounteaDialogueSystemImportExportHelpers::ExportLocalizationPoFiles(
	const FString& stringTableID,
	const FString& defaultLocale,
	const TMap<FString, FString>& defaultTexts,
	const TSharedPtr<FJsonObject>& entriesObject)
{
	if (!entriesObject.IsValid() || entriesObject->Values.IsEmpty())
		return;

	// Discover all locale codes from the first entry's child keys
	TSet<FString> allLocales;
	for (const auto& entryPair : entriesObject->Values)
	{
		const TSharedPtr<FJsonObject> localeMap = entryPair.Value->AsObject();
		if (!localeMap.IsValid())
			continue;
		for (const auto& localePair : localeMap->Values)
			allLocales.Add(localePair.Key);
		break; // one entry is enough to enumerate locales
	}

	for (const FString& locale : allLocales)
	{
		if (locale.Equals(defaultLocale, ESearchCase::IgnoreCase))
			continue; // default locale is the string table itself

		// Build the .po file content
		FString poContent;
		poContent.Append(FString::Printf(TEXT("# Mountea Dialogue localization — %s (%s)\n"), *stringTableID, *locale));
		poContent.Append(TEXT("msgid \"\"\n"));
		poContent.Append(TEXT("msgstr \"\"\n"));
		poContent.Append(TEXT("\"Content-Type: text/plain; charset=UTF-8\\n\"\n"));
		poContent.Append(TEXT("\"Content-Transfer-Encoding: 8bit\\n\"\n\n"));

		for (const auto& entryPair : entriesObject->Values)
		{
			const FString& textKey = entryPair.Key;
			const TSharedPtr<FJsonObject> localeMap = entryPair.Value->AsObject();
			if (!localeMap.IsValid())
				continue;

			const FString* defaultTextPtr = defaultTexts.Find(textKey);
			const FString defaultText = defaultTextPtr ? *defaultTextPtr : FString();

			FString translatedText;
			localeMap->TryGetStringField(*locale, translatedText);

			// Escape inner double-quotes and newlines for .po format
			auto EscapePo = [](const FString& in) -> FString
			{
				FString out = in;
				out.ReplaceInline(TEXT("\\"), TEXT("\\\\"), ESearchCase::CaseSensitive);
				out.ReplaceInline(TEXT("\""), TEXT("\\\""), ESearchCase::CaseSensitive);
				out.ReplaceInline(TEXT("\n"), TEXT("\\n"), ESearchCase::CaseSensitive);
				return out;
			};

			poContent.Append(FString::Printf(TEXT("msgctxt \"%s|%s\"\n"), *stringTableID, *textKey));
			poContent.Append(FString::Printf(TEXT("msgid \"%s\"\n"), *EscapePo(defaultText)));
			poContent.Append(FString::Printf(TEXT("msgstr \"%s\"\n\n"), *EscapePo(translatedText)));
		}

		// Write: {ProjectDir}/Content/Localization/{StringTableID}/{Locale}/{StringTableID}.po
		const FString locDir = FPaths::Combine(
			FPaths::ProjectDir(), TEXT("Content"), TEXT("Localization"),
			stringTableID, locale);
		IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*locDir);

		const FString poPath = FPaths::Combine(locDir, stringTableID + TEXT(".po"));
		const bool bSuccess = FFileHelper::SaveStringToFile(poContent, *poPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		if (!bSuccess)
			EditorLOG_WARNING(TEXT("[ExportLocalizationPoFiles] Failed to write .po file: %s"), *poPath);
		if (bSuccess)
			EditorLOG_INFO(TEXT("[ExportLocalizationPoFiles] Wrote %s"), *poPath);
	}
}

UStringTable* UMounteaDialogueSystemImportExportHelpers::CreateStringTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName, TFunction<void(UStringTable*)> PopulateFunction)
{
	UStringTable* stringTable = nullptr;

	const FSoftObjectPath assetPath = PackagePath + TEXT("/") + AssetName + TEXT(".") + AssetName;
	FAssetRegistryModule& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FAssetData assetData = assetRegistry.Get().GetAssetByObjectPath(assetPath);

	if (assetData.IsValid())
	{
		stringTable = Cast<UStringTable>(assetData.GetAsset());
	}
	else
	{
		const FString packageName = PackagePath + TEXT("/") + AssetName;
		UPackage* package = CreatePackage(*packageName);
		if (package)
		{
			package->FullyLoad();
			stringTable = NewObject<UStringTable>(package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
			if (stringTable)
			{
				stringTable->GetMutableStringTable()->SetNamespace(AssetName);
				FAssetRegistryModule::AssetCreated(stringTable);
				stringTable->MarkPackageDirty();
			}
		}
	}

	if (stringTable)
	{
		stringTable->GetMutableStringTable()->ClearSourceStrings();
		PopulateFunction(stringTable);
	}
	return stringTable;
}

template <typename RowType>
UDataTable* UMounteaDialogueSystemImportExportHelpers::CreateDataTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName)
{
	UDataTable* dataTable = nullptr;

	FSoftObjectPath DataTableAssetPath = PackagePath + TEXT("/") + AssetName + TEXT(".") + AssetName;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData DataTableAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(DataTableAssetPath);
	if (DataTableAssetData.IsValid())
		dataTable = Cast<UDataTable>(DataTableAssetData.GetAsset());
	else
	{
		dataTable = Cast<UDataTable>(
		AssetTools.CreateAsset(AssetName, PackagePath, UDataTable::StaticClass(), nullptr));
	}
	
	if (dataTable)
		dataTable->RowStruct = RowType::StaticStruct();
	
	return dataTable;
}

void UMounteaDialogueSystemImportExportHelpers::SaveAsset(UObject* Asset)
{
	if (!Asset)
		return;

	UPackage* package = Asset->GetOutermost();
	if (!package)
		return;

	// Ensure the package is fully loaded before saving.
	// Assets looked up via the asset registry can be partially loaded; saving them triggers
	// "cannot be saved as it has only been partially loaded" and a debug break.
	if (package->HasAnyPackageFlags(PKG_FilterEditorOnly) == false)
		package->FullyLoad();

	Asset->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Asset);
	const FString PackageName = package->GetName();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs saveArgs;
	{
		saveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	}
	UPackage::SavePackage(package, Asset, *PackageFileName, saveArgs);
}

bool UMounteaDialogueSystemImportExportHelpers::GatherAssetsFromGraph(const UMounteaDialogueGraph* Graph, TMap<FString, FString>& OutJsonFiles, TArray<FString>& OutAudioFiles)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[GatherAssetsFromGraph] Invalid Dialogue Graph!"));
		return false;
	}

	TArray<FDialogueNodeData> AllNodeData;
	GatherNodesFromGraph(Graph, AllNodeData);

	// Process nodes and create JSON
	OutJsonFiles.Add(TEXT("nodes.json"), CreateNodesJson(AllNodeData));
	OutJsonFiles.Add(TEXT("edges.json"), CreateEdgesJson(Graph));
	OutJsonFiles.Add(TEXT("categories.json"), CreateCategoriesJson(Graph));
	OutJsonFiles.Add(TEXT("dialogueData.json"), CreateDialogueDataJson(Graph));
	OutJsonFiles.Add(TEXT("participants.json"), CreateParticipantsJson(Graph));
	OutJsonFiles.Add(TEXT("dialogueRows.json"), CreateDialogueRowsJson(AllNodeData, Graph));
	OutJsonFiles.Add(TEXT("decorators.json"), CreateDecoratorsJson(Graph));
	OutJsonFiles.Add(TEXT("conditions.json"), CreateConditionsJson(Graph));

	const FString stringTableJson = CreateStringTableJson(Graph);
	if (!stringTableJson.IsEmpty())
		OutJsonFiles.Add(TEXT("stringTable.json"), stringTableJson);

	// Gather audio files
	GatherAudioFiles(Graph, OutAudioFiles);

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::GatherNodesFromGraph(const UMounteaDialogueGraph* Graph, TArray<FDialogueNodeData>& OutNodeData)
{
	for (UMounteaDialogueGraphNode* Node : Graph->GetAllNodes())
	{
		if (!Node) continue;

		if (Cast<UMounteaDialogueGraphNode_StartNode>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("startNode"), Node));
		else if (Cast<UMounteaDialogueGraphNode_LeadNode>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("leadNode"), Node));
		else if (Cast<UMounteaDialogueGraphNode_AnswerNode>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("answerNode"), Node));
		else if (Cast<UMounteaDialogueGraphNode_CompleteNode>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("completeNode"), Node));
		else if (Cast<UMounteaDialogueGraphNode_ReturnToNode>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("returnNode"), Node));
		else if (Cast<UMounteaDialogueGraphNode_OpenChildGraph>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("openChildGraphNode"), Node));
		else if (Cast<UMounteaDialogueGraphNode_Delay>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("delayNode"), Node));
		else
			EditorLOG_WARNING(TEXT("[GatherNodesFromGraph] Unknown node type: %s"), *Node->GetClass()->GetName());
	}
}

bool UMounteaDialogueSystemImportExportHelpers::GatherAudioFiles(const UMounteaDialogueGraph* Graph, TArray<FString>& OutAudioFiles)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[GatherAudioFiles] Invalid Graph provided"));
		return false;
	}

	for (const UMounteaDialogueGraphNode* Node : Graph->GetAllNodes())
	{
		const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		if (!DialogueNode)
			continue;

		if (!DialogueNode->GetDataTable())
			continue;

		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
			continue;

		for (const FDialogueRowData& RowData : DialogueRow->DialogueRowData)
		{
			if (RowData.RowSound)
			{
				FString AudioPath = RowData.RowSound->GetOuter()->GetName();
				OutAudioFiles.AddUnique(AudioPath);
			}
		}
	}

	return true;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateNodesJson(const TArray<FDialogueNodeData>& NodeData)
{
	TArray<TSharedPtr<FJsonValue>> NodesArray;

	for (const FDialogueNodeData& Data : NodeData)
	{
		if (!Data.Node)
		{
			EditorLOG_WARNING(TEXT("[CreateNodesJson] Skipping null node"));
			continue;
		}

		TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);
		NodeObject->SetStringField((TEXT("id")), Data.Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
		NodeObject->SetStringField("type", Data.Type);
		NodeObject->SetNumberField(TEXT("executionOrder"), Data.Node->ExecutionOrder);

		AddNodePosition(NodeObject, Data.Node);
		AddNodeData(NodeObject, Data.Node);

		NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(NodesArray, Writer);

	return OutputString;
}

void UMounteaDialogueSystemImportExportHelpers::AddNodePosition(const TSharedPtr<FJsonObject>& NodeObject, const UMounteaDialogueGraphNode* Node)
{
	const TSharedPtr<FJsonObject> PositionObject = MakeShareable(new FJsonObject);

	if (!IsValid(Node))
	{
		EditorLOG_WARNING(TEXT("[AddNodePosition] Invalid Graph or EdGraph for node!"));
		NodeObject->SetObjectField("position", PositionObject);
		return;
	}
	
	PositionObject->SetNumberField("x", Node->NodePosition.X);
	PositionObject->SetNumberField("y", Node->NodePosition.Y);
	
	NodeObject->SetObjectField("position", PositionObject);
}

void UMounteaDialogueSystemImportExportHelpers::AddNodeData(const TSharedPtr<FJsonObject>& NodeObject, const UMounteaDialogueGraphNode* Node)
{
	const TSharedPtr<FJsonObject> DataObject = MakeShareable(new FJsonObject);
	DataObject->SetStringField("title", Node->NodeTitle.ToString());

	// Export node decorators — id and name read from the CLASS CDO so they match the
	// definition GUID and human-readable Dialoguer name set during Blueprint creation.
	// (The instance constructor overwrites these fields, so the instance must not be used.)
	if (!Node->NodeDecorators.IsEmpty())
	{
		TArray<TSharedPtr<FJsonValue>> decoratorsArray;
		for (const FMounteaDialogueDecorator& decor : Node->NodeDecorators)
		{
			if (!decor.DecoratorType)
				continue;

			const UMounteaDialogueDecoratorBase* classCDO =
				decor.DecoratorType->GetClass()->GetDefaultObject<UMounteaDialogueDecoratorBase>();

			TSharedPtr<FJsonObject> decorObj = MakeShareable(new FJsonObject);
			decorObj->SetStringField(TEXT("id"),   classCDO->DecoratorGUID.ToString(EGuidFormats::DigitsWithHyphensLower));
			decorObj->SetStringField(TEXT("name"), classCDO->DecoratorName.ToString());

			TSharedPtr<FJsonObject> valuesObj = MakeShareable(new FJsonObject);
			for (TFieldIterator<FProperty> propIt(decor.DecoratorType->GetClass()); propIt; ++propIt)
			{
				FProperty* prop = *propIt;
				if (!prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
					continue;
				if (prop->GetOwnerClass() == UMounteaDialogueDecoratorBase::StaticClass())
					continue; // skip base-class properties
				FString exportStr;
				prop->ExportTextItem_InContainer(exportStr, decor.DecoratorType, decor.DecoratorType, nullptr, PPF_None);
				valuesObj->SetStringField(prop->GetName(), exportStr);
			}
			decorObj->SetObjectField(TEXT("values"), valuesObj);
			decoratorsArray.Add(MakeShareable(new FJsonValueObject(decorObj)));
		}
		if (!decoratorsArray.IsEmpty())
			DataObject->SetArrayField(TEXT("decorators"), decoratorsArray);
	}

	TSharedPtr<FJsonObject> AdditionalInfoObject = MakeShareable(new FJsonObject);

	if (const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node))
		AddDialogueNodeData(AdditionalInfoObject, DialogueNode);

	if (const UMounteaDialogueGraphNode_ReturnToNode* JumpNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(Node))
		AddJumpNodeData(AdditionalInfoObject, JumpNode);

	if (const UMounteaDialogueGraphNode_OpenChildGraph* OpenChildGraphNode = Cast<UMounteaDialogueGraphNode_OpenChildGraph>(Node))
		AddOpenChildGraphNodeData(AdditionalInfoObject, OpenChildGraphNode);

	DataObject->SetObjectField("additionalInfo", AdditionalInfoObject);
	NodeObject->SetObjectField("data", DataObject);
}

void UMounteaDialogueSystemImportExportHelpers::AddDialogueNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode)
{
	if (!DialogueNode->GetDataTable())
	{
		EditorLOG_ERROR(TEXT("[AddDialogueNodeData] Node %s has no Dialogue Table!"), *DialogueNode->GetNodeTitle().ToString());
		return;
	}

	if (!DialogueNode->GetRowName().IsValid())
	{
		EditorLOG_ERROR(TEXT("[AddDialogueNodeData] Node %s has no Dialogue Row Name!"), *DialogueNode->GetNodeTitle().ToString());
		return;
	}

	FDialogueRow* DialogueRowRef = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
	if (!DialogueRowRef)
	{
		EditorLOG_ERROR(TEXT("[AddDialogueNodeData] Node %s has empty Dialogue Row!"), *DialogueNode->GetNodeTitle().ToString());
		return;
	}

	AdditionalInfoObject->SetStringField("displayName", DialogueRowRef->RowTitle.ToString());

	TSharedPtr<FJsonObject> ParticipantObject = MakeShareable(new FJsonObject);
	ParticipantObject->SetStringField("name", DialogueRowRef->DialogueParticipant.ToString());
	ParticipantObject->SetStringField("category", DialogueRowRef->CompatibleTags.First().ToString());
	AdditionalInfoObject->SetObjectField("participant", ParticipantObject);

	const FString GraphFolder = FPaths::GetPath(DialogueNode->Graph->GetPathName());

	TArray<TSharedPtr<FJsonValue>> DialogueRowsArray;
	for (const auto& RowData : DialogueRowRef->DialogueRowData)
	{
		const TSharedPtr<FJsonObject> RowObject = MakeShareable(new FJsonObject);
		RowObject->SetStringField((TEXT("id")), RowData.RowGUID.ToString(EGuidFormats::DigitsWithHyphensLower));
		RowObject->SetStringField("text", RowData.RowText.ToString());
		RowObject->SetStringField("audio", GetRelativeAudioPath(RowData.RowSound, GraphFolder));
		DialogueRowsArray.Add(MakeShareable(new FJsonValueObject(RowObject)));
	}
	AdditionalInfoObject->SetArrayField("dialogueRows", DialogueRowsArray);
}

void UMounteaDialogueSystemImportExportHelpers::AddJumpNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_ReturnToNode* Node)
{
	if (Node && Node->SelectedNode)
		AdditionalInfoObject->SetStringField("targetNodeId", Node->SelectedNode->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
	else
		AdditionalInfoObject->SetStringField("targetNodeId", "");
}

void UMounteaDialogueSystemImportExportHelpers::AddOpenChildGraphNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_OpenChildGraph* Node)
{
	if (!Node)
	{
		AdditionalInfoObject->SetStringField("targetDialogue", "");
		return;
	}

	AdditionalInfoObject->SetStringField("targetDialogue", Node->TargetDialogue.ToSoftObjectPath().ToString());
}

FString UMounteaDialogueSystemImportExportHelpers::CreateEdgesJson(const UMounteaDialogueGraph* Graph)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[CreateEdgesJson] Invalid Graph provided"));
		return FString();
	}

	TArray<TSharedPtr<FJsonValue>> EdgesArray;

	for (const UMounteaDialogueGraphNode* Node : Graph->GetAllNodes())
	{
		if (!Node) continue;

		for (const UMounteaDialogueGraphNode* ChildNode : Node->GetChildrenNodes())
		{
			if (!ChildNode) continue;

			const TSharedPtr<FJsonObject> EdgeObject = MakeShareable(new FJsonObject);

			const FString edgeId = FString::Printf(TEXT("reactflow__edge-%s-%s"),
				*Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower),
				*ChildNode->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
			EdgeObject->SetStringField(TEXT("id"), edgeId);
			EdgeObject->SetStringField(TEXT("source"), Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
			EdgeObject->SetStringField(TEXT("target"), ChildNode->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
			EdgeObject->SetStringField(TEXT("type"), TEXT("customEdge"));

			// Write edge conditions into data.conditions
			const UMounteaDialogueGraphEdge* const* edgePtr = Node->Edges.Find(ChildNode);
			if (edgePtr && *edgePtr)
			{
				const FMounteaDialogueEdgeConditions& conds = (*edgePtr)->EdgeConditions;

				TSharedPtr<FJsonObject> condObject = MakeShareable(new FJsonObject);
				condObject->SetStringField(TEXT("mode"),
					conds.Mode == EConditionEvaluationMode::Any ? TEXT("any") : TEXT("all"));

				TArray<TSharedPtr<FJsonValue>> rulesArray;
				for (const FMounteaDialogueCondition& rule : conds.Rules)
				{
					if (!rule.ConditionClass)
						continue;
					TSharedPtr<FJsonObject> ruleObj = MakeShareable(new FJsonObject);
					ruleObj->SetStringField(TEXT("name"), rule.ConditionClass->GetConditionName());
					ruleObj->SetStringField(TEXT("id"), rule.ConditionClass->GetConditionGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
					ruleObj->SetBoolField(TEXT("negate"), rule.bNegate);

					// Export Blueprint-defined property values so they survive the round-trip
					TSharedPtr<FJsonObject> valuesObj = MakeShareable(new FJsonObject);
					UClass* condClass = rule.ConditionClass->GetClass();
					for (TFieldIterator<FProperty> propIt(condClass); propIt; ++propIt)
					{
						FProperty* prop = *propIt;
						if (!prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
							continue;
						if (prop->GetOwnerClass() == UMounteaDialogueConditionBase::StaticClass())
							continue;
						FString exportStr;
						prop->ExportTextItem_InContainer(exportStr, rule.ConditionClass, rule.ConditionClass, nullptr, PPF_None);
						valuesObj->SetStringField(prop->GetName(), exportStr);
					}
					if (valuesObj->Values.Num() > 0)
						ruleObj->SetObjectField(TEXT("values"), valuesObj);

					rulesArray.Add(MakeShareable(new FJsonValueObject(ruleObj)));
				}
				condObject->SetArrayField(TEXT("rules"), rulesArray);

				TSharedPtr<FJsonObject> dataObject = MakeShareable(new FJsonObject);
				dataObject->SetObjectField(TEXT("conditions"), condObject);
				EdgeObject->SetObjectField(TEXT("data"), dataObject);
			}

			EdgesArray.Add(MakeShareable(new FJsonValueObject(EdgeObject)));
		}
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(EdgesArray, Writer);

	return OutputString;
}

bool UMounteaDialogueSystemImportExportHelpers::ExportAudioFiles(const TArray<FString>& AudioFiles, const FString& ExportPath, TArray<FString>& OutExportedFiles)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	for (const FString& AudioFile : AudioFiles)
	{
		const USoundWave* SoundWave = LoadObject<USoundWave>(nullptr, *AudioFile);
		if (!SoundWave)
		{
			EditorLOG_ERROR(TEXT("[ExportAudioFiles] Failed to load audio file: %s"), *AudioFile);
			continue;
		}

		// Extract the relative path
		FString RelativePath = AudioFile;
		int32 AudioIndex = RelativePath.Find(TEXT("/audio/"));
		if (AudioIndex != INDEX_NONE)
			RelativePath = RelativePath.RightChop(AudioIndex + 7); // +7 to skip "/audio/"
		else // Use the full path as a fallback
			EditorLOG_WARNING(TEXT("[ExportAudioFiles] Couldn't find '/audio/' in path: %s"), *AudioFile);
		

		FString DestinationPath = FPaths::Combine(ExportPath, RelativePath);

		// Ensure the destination directory exists
		FString DestinationDir = FPaths::GetPath(DestinationPath);
		if (!PlatformFile.CreateDirectoryTree(*DestinationDir))
		{
			EditorLOG_ERROR(TEXT("[ExportAudioFiles] Failed to create directory: %s"), *DestinationDir);
			return false;
		}

		// Get the raw PCM data
		TArray<uint8> RawPCMData;
		uint32 SampleRate;
		uint16 NumChannels;

		if (SoundWave->GetImportedSoundWaveData(RawPCMData, SampleRate, NumChannels))
		{
			TArray<uint8> WAVData;
			CreateWAVFile(RawPCMData, SampleRate, NumChannels, WAVData);

			DestinationPath.Append(".wav");

			// Save the WAV data to the destination file
			if (!FFileHelper::SaveArrayToFile(WAVData, *DestinationPath))
			{
				EditorLOG_ERROR(TEXT("[ExportAudioFiles] Failed to save audio file: %s"), *DestinationPath);
				return false;
			}

			OutExportedFiles.Add(DestinationPath);
		}
		else
		{
			EditorLOG_ERROR(TEXT("[ExportAudioFiles] Failed to get imported sound wave data for: %s"), *AudioFile);
			return false;
		}
	}

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::CreateWAVFile(const TArray<uint8>& InPCMData, uint32 InSampleRate, uint16 InNumChannels, TArray<uint8>& OutWAVData)
{
	// WAV file header
	struct WAVHeader
	{
		uint8 ChunkID[4] = {'R', 'I', 'F', 'F'};
		uint32 ChunkSize;
		uint8 Format[4] = {'W', 'A', 'V', 'E'};
		uint8 Subchunk1ID[4] = {'f', 'm', 't', ' '};
		uint32 Subchunk1Size = 16;
		uint16 AudioFormat = 1;
		uint16 NumChannels;
		uint32 SampleRate;
		uint32 ByteRate;
		uint16 BlockAlign;
		uint16 BitsPerSample = 16;
		uint8 Subchunk2ID[4] = {'d', 'a', 't', 'a'};
		uint32 Subchunk2Size;
	};

	WAVHeader Header;
	Header.NumChannels = InNumChannels;
	Header.SampleRate = InSampleRate;
	Header.ByteRate = InSampleRate * InNumChannels * 2;
	Header.BlockAlign = InNumChannels * 2;
	Header.Subchunk2Size = InPCMData.Num();
	Header.ChunkSize = 36 + Header.Subchunk2Size;

	OutWAVData.SetNum(sizeof(WAVHeader) + InPCMData.Num());
	FMemory::Memcpy(OutWAVData.GetData(), &Header, sizeof(WAVHeader));
	FMemory::Memcpy(OutWAVData.GetData() + sizeof(WAVHeader), InPCMData.GetData(), InPCMData.Num());
}

bool UMounteaDialogueSystemImportExportHelpers::PackToMNTEADLG(const TMap<FString, FString>& JsonFiles, const TArray<FString>& ExportedAudioFiles, const TMap<FString, FString>& ThumbnailFiles, const FString& OutputPath)
{
	FString zipFilePath = FPaths::ChangeExtension(OutputPath, TEXT("mnteadlg"));

	FSimpleZipWriter writer;

	// JSON files (root level)
	for (const auto& jsonFile : JsonFiles)
		writer.AddFile(jsonFile.Key, jsonFile.Value);

	// Thumbnail PNG files
	for (const auto& thumb : ThumbnailFiles)
	{
		const FString zipPath = FString::Printf(TEXT("Thumbnails/%s"), *FPaths::GetCleanFilename(thumb.Key));
		TArray<uint8> thumbData;
		if (!FFileHelper::LoadFileToArray(thumbData, *thumb.Value))
		{
			EditorLOG_WARNING(TEXT("[PackToMNTEADLG] Failed to read thumbnail file: %s"), *thumb.Value);
			continue;
		}
		writer.AddFile(zipPath, thumbData);
	}

	// Audio files
	for (const FString& audioFile : ExportedAudioFiles)
	{
		TArray<uint8> audioData;
		if (!FFileHelper::LoadFileToArray(audioData, *audioFile))
		{
			EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to read audio file: %s"), *audioFile);
			continue;
		}
		writer.AddFile(FString::Printf(TEXT("audio/%s"), *FPaths::GetCleanFilename(audioFile)), audioData);
	}

	if (!writer.Save(zipFilePath))
	{
		EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to write zip file: %s"), *zipFilePath);
		return false;
	}

	return true;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateCategoriesJson(const UMounteaDialogueGraph* Graph)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[CreateCategoriesJson] Invalid Graph provided"));
		return FString();
	}

	TSet<FGameplayTag> AllCategories;
	
	for (const UMounteaDialogueGraphNode* Node : Graph->GetAllNodes())
	{
		const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		if (!DialogueNode)
			continue;

		if (!DialogueNode->GetDataTable())
			continue;
		
		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
			continue;
		
		auto CompatibleTags = DialogueRow->CompatibleTags;
		for (const auto& Itr : CompatibleTags)
		{
			AllCategories.Add(Itr);
		}
	}

	// Derive category entries from the participant tags.
	// Tag format: "Mountea_Dialogue.<FullPath>.<ParticipantLeaf>"
	// Strip the "Mountea_Dialogue." root prefix; then everything except the last
	// dot-segment is the category fullPath. The last segment of fullPath is the name.
	const FString RootPrefix = TEXT("Mountea_Dialogue.");
	TSet<FString> seenFullPaths;
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;

	for (const FGameplayTag& Tag : AllCategories)
	{
		FString tagStr = Tag.ToString();
		if (tagStr.StartsWith(RootPrefix))
			tagStr = tagStr.RightChop(RootPrefix.Len());

		// Drop the last segment (the participant/leaf name), keep the rest as fullPath
		TArray<FString> parts;
		tagStr.ParseIntoArray(parts, TEXT("."));
		if (parts.Num() < 2)
			continue; // no category above the leaf

		parts.Pop(); // remove participant leaf

		FString fullPath;
		for (int32 i = 0; i < parts.Num(); ++i)
		{
			if (i > 0) fullPath += TEXT(".");
			fullPath += parts[i];
		}

		if (seenFullPaths.Contains(fullPath))
			continue;
		seenFullPaths.Add(fullPath);

		TSharedPtr<FJsonObject> CategoryObject = MakeShareable(new FJsonObject);
		CategoryObject->SetStringField(TEXT("name"), parts.Last());
		CategoryObject->SetStringField(TEXT("fullPath"), fullPath);
		CategoriesArray.Add(MakeShareable(new FJsonValueObject(CategoryObject)));
	}
	
	CategoriesArray.Sort([](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B) {
		const TSharedPtr<FJsonObject>& ObjectA = A->AsObject();
		const TSharedPtr<FJsonObject>& ObjectB = B->AsObject();
		
		const FString NameA = ObjectA->GetStringField(TEXT("name"));
		const FString NameB = ObjectB->GetStringField(TEXT("name"));
		
		return NameA < NameB;
	});

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(CategoriesArray, Writer);

	return OutputString;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateDialogueDataJson(const UMounteaDialogueGraph* Graph)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[CreateDialogueDataJson] Invalid Graph provided"));
		return FString();
	}

	TSharedPtr<FJsonObject> DialogueDataObject = MakeShareable(new FJsonObject);

	DialogueDataObject->SetStringField("dialogueGuid", Graph->GetGraphGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
	DialogueDataObject->SetStringField("dialogueName", Graph->GetName());

	const FDateTime CurrentTime = FDateTime::UtcNow();
	const FString FormattedDate = CurrentTime.ToIso8601();
	DialogueDataObject->SetStringField("modifiedOnDate", FormattedDate);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(DialogueDataObject.ToSharedRef(), Writer);

	return OutputString;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateParticipantsJson(const UMounteaDialogueGraph* Graph)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[CreateParticipantsJson] Invalid Graph provided"));
		return FString();
	}

	TSet<TPair<FString, FString>> UniqueParticipants;

	for (const UMounteaDialogueGraphNode* Node : Graph->GetAllNodes())
	{
		const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		if (!DialogueNode)
			continue;

		if (!DialogueNode->GetDataTable())
			continue;

		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
			continue;

		// Derive fullPath: strip "Mountea_Dialogue." root prefix, then drop the last segment
		// (the participant leaf name). E.g. "Mountea_Dialogue.NPC.Merchant.Waldermar" → "NPC.Merchant".
		FString FullPath;
		if (DialogueRow->CompatibleTags.Num() > 0)
		{
			FString tagStr = DialogueRow->CompatibleTags.First().GetTagName().ToString();
			const FString RootPfx = TEXT("Mountea_Dialogue.");
			if (tagStr.StartsWith(RootPfx))
				tagStr = tagStr.RightChop(RootPfx.Len());

			TArray<FString> parts;
			tagStr.ParseIntoArray(parts, TEXT("."));
			if (parts.Num() > 1)
			{
				parts.Pop(); // drop participant leaf
				for (int32 i = 0; i < parts.Num(); ++i)
				{
					if (i > 0) FullPath += TEXT(".");
					FullPath += parts[i];
				}
			}
		}

		const FString Name = DialogueRow->DialogueParticipant.ToString();

		if (!FullPath.IsEmpty() && !Name.IsEmpty())
			UniqueParticipants.Add(TPair<FString, FString>(Name, FullPath));
	}

	// Load the participants DataTable so we can export the icon (participantImage) field
	const FString dtPackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString dtAssetPath = FString::Printf(TEXT("%s/DT_%s_Participants.DT_%s_Participants"),
		*dtPackagePath, *Graph->GetName(), *Graph->GetName());
	FAssetRegistryModule& assetReg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	UDataTable* participantsDT = Cast<UDataTable>(
		assetReg.Get().GetAssetByObjectPath(FSoftObjectPath(dtAssetPath)).GetAsset());

	TArray<TSharedPtr<FJsonValue>> ParticipantsArray;

	for (const auto& Participant : UniqueParticipants)
	{
		TSharedPtr<FJsonObject> ParticipantObject = MakeShareable(new FJsonObject);
		ParticipantObject->SetStringField(TEXT("name"), Participant.Key);
		ParticipantObject->SetStringField(TEXT("fullPath"), Participant.Value);

		if (participantsDT)
		{
			const FDialogueParticipant* row = participantsDT->FindRow<FDialogueParticipant>(FName(*Participant.Key), TEXT(""));
			if (row && !row->ParticipantIcon.IsNull())
				ParticipantObject->SetStringField(TEXT("participantImage"), FPaths::GetBaseFilename(row->ParticipantIcon.ToString()));
			else
				ParticipantObject->SetField(TEXT("participantImage"), MakeShareable(new FJsonValueNull()));
		}

		ParticipantsArray.Add(MakeShareable(new FJsonValueObject(ParticipantObject)));
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ParticipantsArray, Writer);

	return OutputString;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateDialogueRowsJson(const TArray<FDialogueNodeData>& AllNodeData, const UMounteaDialogueGraph* Graph)
{
	TArray<TSharedPtr<FJsonValue>> DialogueRowsArray;

	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[CreateDialogueRowsJson] Invalid Graph provided"));
		return FString();
	}

	const FString GraphPath = Graph->GetPathName();
	const FString GraphFolder = FPaths::GetPath(GraphPath);

	for (const FDialogueNodeData& NodeData : AllNodeData)
	{
		if (!NodeData.Node)
			continue;

		const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(NodeData.Node);
		if (!DialogueNode)
			continue;

		if (!DialogueNode->GetDataTable())
			continue;

		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
			continue;

		for (const FDialogueRowData& RowData : DialogueRow->DialogueRowData)
		{
			TSharedPtr<FJsonObject> RowObject = MakeShareable(new FJsonObject);

			RowObject->SetStringField(TEXT("id"), RowData.RowGUID.ToString(EGuidFormats::DigitsWithHyphensLower));
			RowObject->SetStringField(TEXT("text"), RowData.RowText.ToString());
			RowObject->SetNumberField(TEXT("duration"), RowData.RowDuration);
			RowObject->SetStringField(TEXT("audioPath"), GetRelativeAudioPath(RowData.RowSound, GraphFolder));
			RowObject->SetStringField(TEXT("nodeId"), NodeData.Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));

			DialogueRowsArray.Add(MakeShareable(new FJsonValueObject(RowObject)));
		}
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(DialogueRowsArray, Writer);

	return OutputString;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateStringTableJson(const UMounteaDialogueGraph* graph)
{
	if (!graph)
		return FString();

	TSharedPtr<FJsonObject> root = MakeShareable(new FJsonObject);
	root->SetNumberField(TEXT("version"), 2);
	root->SetStringField(TEXT("format"), TEXT("stringTable.v2"));
	root->SetStringField(TEXT("dialogueId"), graph->GetGraphGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
	root->SetStringField(TEXT("defaultLocale"), TEXT("en"));

	TArray<TSharedPtr<FJsonValue>> localesArray;
	localesArray.Add(MakeShareable(new FJsonValueString(TEXT("en"))));
	root->SetArrayField(TEXT("locales"), localesArray);

	TSharedPtr<FJsonObject> entries = MakeShareable(new FJsonObject);

	for (const UMounteaDialogueGraphNode* node : graph->GetAllNodes())
	{
		const UMounteaDialogueGraphNode_DialogueNodeBase* dialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(node);
		if (!dialogueNode || !dialogueNode->GetDataTable())
			continue;

		const FDialogueRow* row = dialogueNode->GetDataTable()->FindRow<FDialogueRow>(dialogueNode->GetRowName(), TEXT(""));
		if (!row)
			continue;

		for (const FDialogueRowData& rowData : row->DialogueRowData)
		{
			const FString key = rowData.RowGUID.ToString(EGuidFormats::DigitsWithHyphensLower);
			TSharedPtr<FJsonObject> localeMap = MakeShareable(new FJsonObject);
			localeMap->SetStringField(TEXT("en"), rowData.RowText.ToString());
			entries->SetObjectField(key, localeMap);
		}
	}

	root->SetObjectField(TEXT("entries"), entries);

	FString output;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&output);
	FJsonSerializer::Serialize(root.ToSharedRef(), writer);
	return output;
}

void UMounteaDialogueSystemImportExportHelpers::ShowNotification(const FText& Message, const float Duration, const FString& BrushName, const FSimpleDelegate& Hyperlink, const FText& HyperlinkText)
{
	FNotificationInfo Info(Message);
	Info.ExpireDuration = Duration;
	Info.Image = FMounteaDialogueGraphEditorStyle::GetBrush(*BrushName);
	if (Hyperlink.IsBound())
	{
		Info.Hyperlink = Hyperlink;
		Info.HyperlinkText = HyperlinkText;
	}
	FSlateNotificationManager::Get().AddNotification(Info);
}

FString UMounteaDialogueSystemImportExportHelpers::GetRelativeAudioPath(const USoundBase* Sound, const FString& GraphFolder)
{
	if (!Sound)
		return TEXT("null");

	const FString FullAudioPath = Sound->GetOuter()->GetName();
	FString RelativeAudioPath = FullAudioPath;

	// Remove the common part of the path
	if (FullAudioPath.StartsWith(GraphFolder))
	{
		RelativeAudioPath = FullAudioPath.RightChop(GraphFolder.Len() + 1);
		RelativeAudioPath.Append(TEXT(".wav"));
	}

	// Ensure the path starts with "audio/"
	if (!RelativeAudioPath.StartsWith(TEXT("audio/")))
		RelativeAudioPath = FString::Printf(TEXT("audio/%s"), *RelativeAudioPath);

	return RelativeAudioPath;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateDecoratorsJson(const UMounteaDialogueGraph* Graph)
{
	if (!Graph)
		return TEXT("[]");

	TSet<FGuid> seenGuids;
	TArray<TSharedPtr<FJsonValue>> decoratorsArray;

	for (const UMounteaDialogueGraphNode* node : Graph->GetAllNodes())
	{
		if (!node)
			continue;
		for (const FMounteaDialogueDecorator& decor : node->NodeDecorators)
		{
			if (!decor.DecoratorType)
				continue;

			const UMounteaDialogueDecoratorBase* classCDO =
				decor.DecoratorType->GetClass()->GetDefaultObject<UMounteaDialogueDecoratorBase>();
			if (!classCDO || !classCDO->DecoratorGUID.IsValid())
				continue;
			if (seenGuids.Contains(classCDO->DecoratorGUID))
				continue;
			seenGuids.Add(classCDO->DecoratorGUID);

			TSharedPtr<FJsonObject> decorObj = MakeShareable(new FJsonObject);
			decorObj->SetStringField(TEXT("id"), classCDO->DecoratorGUID.ToString(EGuidFormats::DigitsWithHyphensLower));
			decorObj->SetStringField(TEXT("name"), classCDO->DecoratorName.ToString());
			decorObj->SetStringField(TEXT("type"), TEXT(""));

			// List Blueprint-defined properties so Dialoguer can reconstruct the schema
			TArray<TSharedPtr<FJsonValue>> propsArray;
			for (TFieldIterator<FProperty> propIt(decor.DecoratorType->GetClass()); propIt; ++propIt)
			{
				FProperty* prop = *propIt;
				if (!prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
					continue;
				if (prop->GetOwnerClass() == UMounteaDialogueDecoratorBase::StaticClass())
					continue;

				TSharedPtr<FJsonObject> propObj = MakeShareable(new FJsonObject);
				propObj->SetStringField(TEXT("name"), prop->GetName());
				propObj->SetStringField(TEXT("type"), prop->GetCPPType());
				propsArray.Add(MakeShareable(new FJsonValueObject(propObj)));
			}
			decorObj->SetArrayField(TEXT("properties"), propsArray);

			decoratorsArray.Add(MakeShareable(new FJsonValueObject(decorObj)));
		}
	}

	FString outputString;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outputString);
	FJsonSerializer::Serialize(decoratorsArray, writer);
	return outputString;
}

FString UMounteaDialogueSystemImportExportHelpers::CreateConditionsJson(const UMounteaDialogueGraph* Graph)
{
	if (!Graph)
		return TEXT("[]");

	TSet<FGuid> seenGuids;
	TArray<TSharedPtr<FJsonValue>> conditionsArray;

	for (const UMounteaDialogueGraphNode* node : Graph->GetAllNodes())
	{
		if (!node)
			continue;
		for (const auto& edgePair : node->Edges)
		{
			const UMounteaDialogueGraphEdge* edge = edgePair.Value;
			if (!edge)
				continue;
			for (const FMounteaDialogueCondition& rule : edge->EdgeConditions.Rules)
			{
				if (!rule.ConditionClass)
					continue;

				const UMounteaDialogueConditionBase* classCDO =
					rule.ConditionClass->GetClass()->GetDefaultObject<UMounteaDialogueConditionBase>();
				if (!classCDO)
					continue;

				const FGuid defGuid = classCDO->GetConditionGUID();
				if (!defGuid.IsValid() || seenGuids.Contains(defGuid))
					continue;
				seenGuids.Add(defGuid);

				TSharedPtr<FJsonObject> condObj = MakeShareable(new FJsonObject);
				condObj->SetStringField(TEXT("id"), defGuid.ToString(EGuidFormats::DigitsWithHyphensLower));
				condObj->SetStringField(TEXT("name"), classCDO->GetConditionName());

				// List Blueprint-defined properties
				TArray<TSharedPtr<FJsonValue>> propsArray;
				for (TFieldIterator<FProperty> propIt(rule.ConditionClass->GetClass()); propIt; ++propIt)
				{
					FProperty* prop = *propIt;
					if (!prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
						continue;
					if (prop->GetOwnerClass() == UMounteaDialogueConditionBase::StaticClass())
						continue;

					TSharedPtr<FJsonObject> propObj = MakeShareable(new FJsonObject);
					propObj->SetStringField(TEXT("name"), prop->GetName());
					propObj->SetStringField(TEXT("type"), prop->GetCPPType());
					propsArray.Add(MakeShareable(new FJsonValueObject(propObj)));
				}
				condObj->SetArrayField(TEXT("properties"), propsArray);

				conditionsArray.Add(MakeShareable(new FJsonValueObject(condObj)));
			}
		}
	}

	FString outputString;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outputString);
	FJsonSerializer::Serialize(conditionsArray, writer);
	return outputString;
}

bool UMounteaDialogueSystemImportExportHelpers::GatherThumbnailFiles(const UMounteaDialogueGraph* Graph, TMap<FString, FString>& OutThumbnailFiles)
{
	if (!Graph)
		return false;

	const FString dtPackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString dtAssetPath = FString::Printf(TEXT("%s/DT_%s_Participants.DT_%s_Participants"),
		*dtPackagePath, *Graph->GetName(), *Graph->GetName());

	FAssetRegistryModule& assetReg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	UDataTable* participantsDT = Cast<UDataTable>(
		assetReg.Get().GetAssetByObjectPath(FSoftObjectPath(dtAssetPath)).GetAsset());
	if (!participantsDT)
		return false;

	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

	for (const TPair<FName, uint8*>& rowPair : participantsDT->GetRowMap())
	{
		const FDialogueParticipant* row = reinterpret_cast<const FDialogueParticipant*>(rowPair.Value);
		if (!row || row->ParticipantIcon.IsNull())
			continue;

		UTexture2D* texture = row->ParticipantIcon.LoadSynchronous();
		if (!texture)
			continue;

		// Read the raw source PNG data from the texture's bulk data
		const FTextureSource& texSource = texture->Source;
		if (!texSource.IsValid())
			continue;

		TArray64<uint8> sourceData;
		if (!const_cast<FTextureSource&>(texSource).GetMipData(sourceData, 0))
			continue;

		// Map texture source format to the image wrapper's RGB format and bit depth
		ERGBFormat rgbFormat = ERGBFormat::BGRA;
		int32 bitDepth = 8;
		const ETextureSourceFormat srcFmt = texSource.GetFormat();
		switch (srcFmt)
		{
			case TSF_RGBA8_DEPRECATED:  rgbFormat = ERGBFormat::RGBA; bitDepth = 8;  break;
			case TSF_RGBA16:			rgbFormat = ERGBFormat::RGBA; bitDepth = 16; break;
			case TSF_RGBA16F:			rgbFormat = ERGBFormat::RGBA; bitDepth = 16; break;
			case TSF_G8:				rgbFormat = ERGBFormat::Gray; bitDepth = 8;  break;
			case TSF_G16:				rgbFormat = ERGBFormat::Gray; bitDepth = 16; break;
			default:					rgbFormat = ERGBFormat::BGRA; bitDepth = 8;  break; // TSF_BGRA8, TSF_BGRE8, etc.
		}

		// Re-encode as PNG
		TSharedPtr<IImageWrapper> wrapper = imageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		const int32 sizeX = texSource.GetSizeX();
		const int32 sizeY = texSource.GetSizeY();
		wrapper->SetRaw(sourceData.GetData(), sourceData.Num(), sizeX, sizeY, rgbFormat, bitDepth);
		const TArray64<uint8>& pngData = wrapper->GetCompressed();

		// Write to temp file
		const FString assetName = FPaths::GetBaseFilename(texture->GetPathName());
		const FString tempPath = FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), *assetName, TEXT(".png"));
		if (FFileHelper::SaveArrayToFile(pngData, *tempPath))
			OutThumbnailFiles.Add(FString::Printf(TEXT("%s.png"), *assetName), tempPath);
	}

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::ExportProject(const TArray<UMounteaDialogueGraph*>& Graphs, const FString& FilePath)
{
	if (Graphs.IsEmpty())
	{
		EditorLOG_ERROR(TEXT("[ExportProject] No graphs provided"));
		return false;
	}

	// Temp dir for intermediate .mnteadlg archives
	const FString tempDir = FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("MNTEAProjExport"), TEXT(""));
	IFileManager::Get().MakeDirectory(*tempDir, true);

	// Project-level aggregated JSON arrays (deduplicated by first-seen entry)
	TSet<FString> seenCategoryNames;
	TSet<FString> seenParticipantNames;
	TSet<FString> seenDecoratorGuids;
	TSet<FString> seenConditionGuids;
	TArray<TSharedPtr<FJsonValue>> allCategories;
	TArray<TSharedPtr<FJsonValue>> allParticipants;
	TArray<TSharedPtr<FJsonValue>> allDecorators;
	TArray<TSharedPtr<FJsonValue>> allConditions;
	TMap<FString, FString> dialogueArchives; // "dialogues/<name>.mnteadlg" → temp path
	TMap<FString, FString> allThumbnails;

	for (UMounteaDialogueGraph* graph : Graphs)
	{
		if (!graph)
			continue;

		TMap<FString, FString> jsonFiles;
		TArray<FString> audioFiles;
		if (!GatherAssetsFromGraph(graph, jsonFiles, audioFiles))
		{
			EditorLOG_WARNING(TEXT("[ExportProject] Failed to gather assets for graph '%s' — skipped"), *graph->GetName());
			continue;
		}

		TMap<FString, FString> thumbFiles;
		GatherThumbnailFiles(graph, thumbFiles);
		for (const auto& t : thumbFiles)
			if (!allThumbnails.Contains(t.Key))
				allThumbnails.Add(t.Key, t.Value);

		// Merge project-level JSON arrays (deduplicate across graphs)
		if (jsonFiles.Contains(TEXT("categories.json")))
			MergeJsonArrayInto(jsonFiles[TEXT("categories.json")], allCategories, seenCategoryNames, TEXT("name"));
		if (jsonFiles.Contains(TEXT("participants.json")))
			MergeJsonArrayInto(jsonFiles[TEXT("participants.json")], allParticipants, seenParticipantNames, TEXT("name"));
		if (jsonFiles.Contains(TEXT("decorators.json")))
			MergeJsonArrayInto(jsonFiles[TEXT("decorators.json")], allDecorators, seenDecoratorGuids, TEXT("id"));
		if (jsonFiles.Contains(TEXT("conditions.json")))
			MergeJsonArrayInto(jsonFiles[TEXT("conditions.json")], allConditions, seenConditionGuids, TEXT("id"));

		// Pack this graph as a temp .mnteadlg
		const FString tempArchivePath = FString::Printf(TEXT("%s/%s.mnteadlg"), *tempDir, *graph->GetName());
		TArray<FString> exportedAudio;
		ExportAudioFiles(audioFiles, tempDir, exportedAudio);
		if (PackToMNTEADLG(jsonFiles, exportedAudio, thumbFiles, tempArchivePath))
			dialogueArchives.Add(FString::Printf(TEXT("dialogues/%s.mnteadlg"), *graph->GetName()), tempArchivePath);
		else
			EditorLOG_WARNING(TEXT("[ExportProject] Failed to pack dialogue '%s'"), *graph->GetName());

		for (const FString& f : exportedAudio)
			IFileManager::Get().Delete(*f);
	}

	// Infer project name from file path and build projectData.json
	const FString projectName = FPaths::GetBaseFilename(FilePath);
	TSharedPtr<FJsonObject> projectDataObj = MakeShareable(new FJsonObject);
	projectDataObj->SetStringField(TEXT("projectName"), projectName);
	projectDataObj->SetStringField(TEXT("projectGuid"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
	FString projectDataStr;
	TSharedRef<TJsonWriter<>> pdWriter = TJsonWriterFactory<>::Create(&projectDataStr);
	FJsonSerializer::Serialize(projectDataObj.ToSharedRef(), pdWriter);

	TMap<FString, FString> projectJsonFiles;
	projectJsonFiles.Add(TEXT("projectData.json"), projectDataStr);
	projectJsonFiles.Add(TEXT("categories.json"),   SerializeJsonArray(allCategories));
	projectJsonFiles.Add(TEXT("participants.json"), SerializeJsonArray(allParticipants));
	projectJsonFiles.Add(TEXT("decorators.json"),   SerializeJsonArray(allDecorators));
	projectJsonFiles.Add(TEXT("conditions.json"),   SerializeJsonArray(allConditions));

	const bool bSuccess = PackToMNTEADLGPROJ(projectJsonFiles, dialogueArchives, allThumbnails, FilePath);

	// Clean up temp files
	for (const auto& arch : dialogueArchives)
		IFileManager::Get().Delete(*arch.Value);
	for (const auto& t : allThumbnails)
		IFileManager::Get().Delete(*t.Value);
	IFileManager::Get().DeleteDirectory(*tempDir, false, true);

	return bSuccess;
}

bool UMounteaDialogueSystemImportExportHelpers::PackToMNTEADLGPROJ(
	const TMap<FString, FString>& ProjectJsonFiles,
	const TMap<FString, FString>& DialogueArchives,
	const TMap<FString, FString>& ThumbnailFiles,
	const FString& OutputPath)
{
	FString zipPath = FPaths::ChangeExtension(OutputPath, TEXT("mnteadlgproj"));

	FSimpleZipWriter writer;

	// Root-level JSON files
	for (const auto& jsonFile : ProjectJsonFiles)
		writer.AddFile(jsonFile.Key, jsonFile.Value);

	// Project-level thumbnails
	for (const auto& thumb : ThumbnailFiles)
	{
		const FString zipThumbPath = FString::Printf(TEXT("Thumbnails/%s"), *FPaths::GetCleanFilename(thumb.Key));
		TArray<uint8> thumbData;
		if (!FFileHelper::LoadFileToArray(thumbData, *thumb.Value))
		{
			EditorLOG_WARNING(TEXT("[PackToMNTEADLGPROJ] Failed to read thumbnail: %s"), *thumb.Value);
			continue;
		}
		writer.AddFile(zipThumbPath, thumbData);
	}

	// Dialogue archives under dialogues/
	for (const auto& arch : DialogueArchives)
	{
		TArray<uint8> archData;
		if (!FFileHelper::LoadFileToArray(archData, *arch.Value))
		{
			EditorLOG_WARNING(TEXT("[PackToMNTEADLGPROJ] Failed to read dialogue archive: %s"), *arch.Value);
			continue;
		}
		writer.AddFile(arch.Key, archData);
	}

	if (!writer.Save(zipPath))
	{
		EditorLOG_ERROR(TEXT("[PackToMNTEADLGPROJ] Failed to write zip file: %s"), *zipPath);
		return false;
	}

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::MergeJsonArrayInto(const FString& JsonStr, TArray<TSharedPtr<FJsonValue>>& Target, TSet<FString>& SeenKeys, const FString& KeyField)
{
	TArray<TSharedPtr<FJsonValue>> parsed;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(reader, parsed))
		return;

	for (const TSharedPtr<FJsonValue>& val : parsed)
	{
		const TSharedPtr<FJsonObject>* obj;
		if (!val->TryGetObject(obj))
			continue;

		FString key;
		if (!(*obj)->TryGetStringField(KeyField, key))
			continue;

		if (SeenKeys.Contains(key))
			continue;

		SeenKeys.Add(key);
		Target.Add(val);
	}
}

FString UMounteaDialogueSystemImportExportHelpers::SerializeJsonArray(const TArray<TSharedPtr<FJsonValue>>& Array)
{
	FString out;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&out);
	FJsonSerializer::Serialize(Array, writer);
	return out;
}

#undef LOCTEXT_NAMESPACE
