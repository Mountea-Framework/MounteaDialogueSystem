// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueSystemImportExportHelpers.h"
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
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Framework/Notifications/NotificationManager.h"

#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"

#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#include "ImportConfig/MounteaDialogueImportConfig.h"
#include "Interfaces/IPluginManager.h"

#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemImportExportHelpers"

bool UMounteaDialogueSystemImportExportHelpers::IsReimport(const FString& Filename)
{
	if (Filename.IsEmpty())
	{
		return false;
	}
	
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *Filename))
	{
		return false;
	}

	if (!IsZipFile(fileData))
	{
		return false;
	}

	TMap<FString, FString> extractedFiles;
	if (!ExtractFilesFromZip(fileData, extractedFiles))
	{
		return false;
	}

	if (!ValidateExtractedContent(extractedFiles))
	{
		return false;
	}

	FGuid dialogueGuid;
	if (extractedFiles.Contains("dialogueData.json"))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(extractedFiles["dialogueData.json"]);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasField("dialogueGuid"))
			{
				dialogueGuid = FGuid(JsonObject->GetStringField("dialogueGuid"));
			}
			
		}
	}
	else
	{
		return false;
	}

	const UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
	if (!importConfig)
		return false;

	return importConfig->ImportHistory.Contains(dialogueGuid);
}

bool UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(const FString& FilePath, UObject* ObjectRedirector, UMounteaDialogueGraph*& OutGraph, FString& OutMessage)
{
	if (FilePath.IsEmpty())
	{
		OutMessage = TEXT("Source File is empty.");
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	// TODO: with recursive calls FilePath is wrong so maybe do not call with different path?
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *FilePath))
	{
		OutMessage = FString::Printf( TEXT("Failed to load file: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	if (!IsZipFile(fileData))
	{
		OutMessage = FString::Printf( TEXT("File is not a valid mnteadlg/zip: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	TMap<FString, FString> extractedFiles;
	if (!ExtractFilesFromZip(fileData, extractedFiles))
	{
		OutMessage = FString::Printf( TEXT("Failed to extract files from archive: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}
	
	if (!ValidateExtractedContent(extractedFiles))
	{
		OutMessage = FString::Printf( TEXT("Invalid content in file: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	FGuid dialogueGuid;
	if (extractedFiles.Contains("dialogueData.json"))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(extractedFiles["dialogueData.json"]);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasField("dialogueGuid"))
			{
				dialogueGuid = FGuid(JsonObject->GetStringField("dialogueGuid"));
			}
			else
			{
				OutMessage = FString::Printf( TEXT("`dialogueGuid` field not found in dialogueData.json"));
				EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
				return false;
			}
		}
		else
		{
			OutMessage = FString::Printf( TEXT("Failed to parse dialogueData.json"));
			EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
			return false;
		}
	}
	else
	{
		OutMessage = FString::Printf( TEXT("Invalid Dialogue source, missing dialogueData.json"));
		EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
		return false;
	}

	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();
	if (!importConfig)
		return false;

	if (!OutGraph)
	{		
		if (importConfig->ImportHistory.Contains(dialogueGuid))
		{
			const FString dialoguePath = importConfig->ImportHistory.Find(dialogueGuid)->DialogueAssetPath;

			const FString Directory = FPaths::GetPath(dialoguePath);
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

			FARFilter Filter;
			Filter.bRecursivePaths = false;
			Filter.PackagePaths.Add(FName(*Directory));
			Filter.ClassPaths.Add(UMounteaDialogueGraph::StaticClass()->GetClassPathName());

			TArray<FAssetData> AssetDataList;
			AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

			for (const FAssetData& AssetData : AssetDataList)
			{
				UMounteaDialogueGraph* DialogueGraph = Cast<UMounteaDialogueGraph>(AssetData.GetAsset());
				if (DialogueGraph && DialogueGraph->GetGraphGUID() == dialogueGuid)
				{
					OutGraph = DialogueGraph;
					break;
				}
			}
		}
	}
	
	if (OutGraph)
	{
		if (dialogueGuid != OutGraph->GetGraphGUID())
		{
			// TODO: Rather than return false process creating new dialogue? Maybe expose this option to settings?
			OutMessage = FString::Printf( TEXT("Attempting to reimport different Graph.\nUnreal Graph: %s\nSource Graph: %s"), *OutGraph->GetGraphGUID().ToString(EGuidFormats::DigitsWithHyphensLower), *dialogueGuid.ToString(EGuidFormats::DigitsWithHyphensLower));
			EditorLOG_ERROR(TEXT("[ReimportDialogueGraph] %s"), *OutMessage);
			return false;
		}

		OutGraph->ClearGraph();
		
		if (PopulateGraphFromExtractedFiles(OutGraph, extractedFiles, FilePath))
		{
			ImportAudioFiles(extractedFiles, OutGraph, OutGraph);
			
			if (OutGraph->EdGraph)
			{
				if (UEdGraph_MounteaDialogueGraph *edGraph = Cast<UEdGraph_MounteaDialogueGraph>(OutGraph->EdGraph))
					edGraph->RebuildMounteaDialogueGraph();
			}
		}

		for (const auto &File : extractedFiles)
		{
			if (File.Key.StartsWith("audio/"))
			{
				IFileManager::Get().Delete(*File.Value);
			}
		}

		OutMessage = FString::Printf(TEXT("Graph `%s` has been refreshed."), *OutGraph->GetName());

		return true;
	}
	
	// Create new, because:
	// - we dont have OutGraph
	// - config doesnt know this guid yet
	EObjectFlags flags = RF_Public | RF_Standalone | RF_Transactional;
	UMounteaDialogueGraph* NewGraph = NewObject<UMounteaDialogueGraph>(ObjectRedirector, UMounteaDialogueGraph::StaticClass(), FName(ObjectRedirector->GetName()), flags);
	OutGraph = NewGraph;
	FString outMessage;
	return ImportDialogueGraph(FilePath, ObjectRedirector, FName(ObjectRedirector->GetName()), flags, NewGraph, outMessage);
}

bool UMounteaDialogueSystemImportExportHelpers::CanReimport(UObject* ObjectRedirector, TArray<FString>& OutFilenames)
{
	return true;
}

void UMounteaDialogueSystemImportExportHelpers::UpdateGraphImportDataConfig(const UMounteaDialogueGraph* Graph, const FString& JsonName, const FString& Json, const FString& PackagePath, const FString& AssetName)
{
	const FString GameDirectory = FPaths::ProjectDir();
	const FString UpdatedConfigFile = GameDirectory + "/Config/MounteaDialogueImportConfig.ini";

	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();

	if (FPaths::FileExists(UpdatedConfigFile))
	{
		importConfig->LoadConfig(nullptr, *UpdatedConfigFile);
	}
	else
	{
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
	// 1. Load the file
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *FilePath))
	{
		OutMessage = FString::Printf(TEXT("Failed to extract files from archive: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] %s"), *OutMessage);
		return false;
	}

	// 2. Make sure it's a zip file
	if (!IsZipFile(fileData))
	{
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] File is not a valid mnteadlg/zip: %s"), *FilePath);
		return false;
	}

	// 3. Extract and read content
	TMap<FString, FString> extractedFiles;
	if (!ExtractFilesFromZip(fileData, extractedFiles))
	{
		OutMessage = FString::Printf(TEXT("Failed to extract files from archive: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] %s"), *OutMessage);
		return false;
	}

	// 3. Read the GUID from dialogueData.json
	FGuid importedGuid;
	FString dialogueName;
	if (extractedFiles.Contains("dialogueData.json"))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(extractedFiles["dialogueData.json"]);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasField("dialogueGuid"))
			{
				FString guidString = JsonObject->GetStringField("dialogueGuid");
				FGuid::Parse(guidString, importedGuid);
			}
			
			if (JsonObject->HasField("dialogueName"))
			{
				dialogueName = JsonObject->GetStringField("dialogueName");
			}
			else
			{
				OutMessage = TEXT("dialogueName field not found in dialogueData.json");
				EditorLOG_ERROR(TEXT("[FactoryCreateFile] %s"), *OutMessage);
				return false;
			}
		}
		else
		{
			OutMessage = TEXT("Failed to parse dialogueData.json");
			EditorLOG_ERROR(TEXT("[FactoryCreateFile] %s"), *OutMessage);
			return false;
		}
	}
	else
	{
		OutMessage = TEXT("dialogueData.json not found in extracted files");
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] %s"), *OutMessage);
		return false;
	}

	if (!importedGuid.IsValid())
	{
		OutMessage = TEXT("Failed to read GUID from imported file");
		EditorLOG_ERROR(TEXT("[ImportDialogueGraph] %s"), *OutMessage);
		return false;
	}
	
	// 4. Check for existing Dialogue Graphs in the folder
	const FString Directory = FPaths::GetPath(InParent->GetPackage()->GetName());
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.bRecursivePaths = false;
	Filter.PackagePaths.Add(FName(*Directory));
	Filter.ClassPaths.Add(UMounteaDialogueGraph::StaticClass()->GetClassPathName());

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	// 5. Compare GUIDs and handle reimport or new import
	bool bFoundGraphOk = false;
	UMounteaDialogueGraph* ExistingGraph = nullptr;
	for (const FAssetData& AssetData : AssetDataList)
	{
		UMounteaDialogueGraph* DialogueGraph = Cast<UMounteaDialogueGraph>(AssetData.GetAsset());
		if (DialogueGraph && DialogueGraph->GetGraphGUID() == importedGuid)
		{
			ExistingGraph = DialogueGraph;
			break;
		}
	}
	
	if (ExistingGraph)
	{
		// Reimport
		OutGraph = ExistingGraph;
		return ReimportDialogueGraph(FilePath, InParent, OutGraph, OutMessage);
	}

	if (AssetDataList.Num() > 0)
	{
		for (const FAssetData& AssetData : AssetDataList)
		{
			UMounteaDialogueGraph* DialogueGraph = Cast<UMounteaDialogueGraph>(AssetData.GetAsset());
			if (DialogueGraph == OutGraph)
			{
				bFoundGraphOk = true;
				break;
			}
		}

		if (!bFoundGraphOk)
		{
			OutMessage = TEXT("A different Dialogue Graph already exists in this folder. Import canceled.");
			EditorLOG_ERROR(TEXT("[ImportDialogueGraph] %s"), *OutMessage);
			return false;
		}
	}

	// 4. Validate content
	if (!ValidateExtractedContent(extractedFiles))
	{
		OutMessage = FString::Printf(TEXT("Invalid content in file: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] %s"), *OutMessage);
		return false;
	}
	const FName assetName = !dialogueName.IsEmpty() ? FName(*dialogueName) : Name;

	// 6. Process UMounteaDialogueGraph if all is good
	if (OutGraph)
	{
		OutGraph->Rename(*assetName.ToString());

		if (PopulateGraphFromExtractedFiles(OutGraph, extractedFiles, FilePath))
		{
			// 7. Import audio files if present
			ImportAudioFiles(extractedFiles, InParent, OutGraph);

			OutGraph->CreateGraph();
			if (OutGraph->EdGraph)
			{
				if (UEdGraph_MounteaDialogueGraph *edGraph = Cast<UEdGraph_MounteaDialogueGraph>(OutGraph->EdGraph))
					edGraph->RebuildMounteaDialogueGraph();
			}

			SaveAsset(OutGraph);

			OutMessage = FString::Printf(TEXT("New Dialogue Graph created!"));
			return true;
		}
		OutMessage = FString::Printf(TEXT("Failed to populate graph from extracted files: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] "), *OutMessage);
	}
	else
	{
		OutMessage = FString::Printf(TEXT("Failed to create new graph object"));
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] %s"), *OutMessage);
	}

	// Cleanup
	for (const auto &File : extractedFiles)
	{
		if (File.Key.StartsWith("audio/"))
		{
			IFileManager::Get().Delete(*File.Value);
		}
	}

	return false;
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
	bool bSuccess = false;

	if (ExportAudioFiles(AudioFiles, ExportDirectory, ExportedAudioFiles))
	{
		bSuccess = PackToMNTEADLG(JsonFiles, ExportedAudioFiles, FilePath);
		
		if (!bSuccess)
		{
			EditorLOG_ERROR(TEXT("[ExportDialogueGraph] Failed to pack files into MNTEADLG"));
		}
	}
	else
	{
		EditorLOG_ERROR(TEXT("[ExportDialogueGraph] Failed to export audio files"));
	}
	
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
	{
		return true;
	}

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
			if (FileName.StartsWith(TEXT("audio/")) && FileName.EndsWith(TEXT(".wav")))
			{
				// For audio files, extract to a temporary file
				FString TempAudioPath = FPaths::CreateTempFilename(FPlatformProcess::UserTempDir(), TEXT("TempAudio"), TEXT(".wav"));
				
				TArray<uint8> Buffer;
				Buffer.SetNum(size);
				
				if (zip_entry_noallocread(zip, Buffer.GetData(), size) != -1)
				{
					if (FFileHelper::SaveArrayToFile(Buffer, *TempAudioPath))
					{
						OutExtractedFiles.Add(FileName, TempAudioPath);
					}
					else
					{
						EditorLOG_ERROR(TEXT("Failed to save temporary audio file: %s"), *TempAudioPath);
					}
				}
				else
				{
					EditorLOG_ERROR(TEXT("Failed to read audio file content: %hs"), name);
				}
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
				{
					EditorLOG_ERROR(TEXT("Failed to read file content: %hs"), name);
				}
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
	TArray<TCHAR> CharArray;
	CharArray.SetNum(Count + 1);
	for (int32 i = 0; i < Count; ++i)
	{
		CharArray[i] = static_cast<TCHAR>(Bytes[i]);
	}
	CharArray[Count] = '\0';
	return FString(CharArray.GetData());
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

bool UMounteaDialogueSystemImportExportHelpers::PopulateGraphFromExtractedFiles(UMounteaDialogueGraph* Graph, const TMap<FString, FString>& ExtractedFiles, const FString& SourceFilePath)
{
	if (!PopulateDialogueData(Graph, SourceFilePath, ExtractedFiles))
	{
		return false;
	}

	if (!PopulateCategories(Graph, ExtractedFiles["categories.json"]))
	{
		return false;
	}

	if (!PopulateParticipants(Graph, ExtractedFiles["participants.json"]))
	{
		return false;
	}

	if (!PopulateNodes(Graph, ExtractedFiles["nodes.json"]))
	{
		return false;
	}

	if (!PopulateEdges(Graph, ExtractedFiles["edges.json"]))
	{
		return false;
	}

	if (!PopulateDialogueRows(Graph, ExtractedFiles))
	{
		return false;
	}

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::ImportAudioFiles(const TMap<FString, FString>& ExtractedFiles, UObject* InParent, UMounteaDialogueGraph* Graph)
{
	FModuleManager::LoadModuleChecked<IAudioEditorModule>("AudioEditor");
	IAudioEditorModule& AudioEditorModule = FModuleManager::GetModuleChecked<IAudioEditorModule>("AudioEditor");

	const FString Directory = FPaths::GetPath(InParent->GetPackage()->GetName());
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FString PackagePath = FPackageName::GetLongPackagePath(InParent->GetPathName());

	// Create `audio` folder if one doesn't exist yet
	{
		FString AudioFolderPath = FPaths::Combine(
			FPaths::ProjectContentDir(),
			PackagePath.RightChop(6),
			TEXT("audio")
		);

		FPaths::MakeStandardFilename(AudioFolderPath);

		if (!FPaths::DirectoryExists(AudioFolderPath))
		{
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*AudioFolderPath);
		}
	}
	
	// Extract just the asset name from InParent
	FString AssetName = FPaths::GetBaseFilename(Graph->GetName());
	
	FString DialogueRowsDataTableName = FString::Printf(TEXT("DT_%s_DialogueRows"), *AssetName);
	FString FullAssetPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *DialogueRowsDataTableName);
	FSoftObjectPath AssetReference(FullAssetPath);
	UDataTable* DialogueRowsDataTable = Cast<UDataTable>(AssetReference.TryLoad());

	if (!DialogueRowsDataTable)
	{
		EditorLOG_ERROR(TEXT("[ImportAudioFiles] Failed to load Dialogue Rows DataTable: %s"), *FullAssetPath);
	}

	TMap<FGuid, USoundWave*> ImportedAudioMap;
	
	for (const auto& File : ExtractedFiles)
	{
		if (File.Key.StartsWith("audio/") && File.Key.EndsWith(".wav"))
		{
			FString TempAudioPath = File.Value;
			
			FString RelativePath = File.Key;
			RelativePath.RemoveFromStart(TEXT("audio/"));
			FString SubfolderPath = FPaths::GetPath(RelativePath);
			
			FGuid RowDataGuid = FGuid(FPaths::GetBaseFilename(SubfolderPath));

			FString DestinationPath = FPaths::Combine(FPaths::GetPath(InParent->GetPathName()), TEXT("audio"), SubfolderPath);
			
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*DestinationPath);
			
			FString FullDestinationPath = FPaths::Combine(DestinationPath, FPaths::GetCleanFilename(File.Key));
			
			FString RelativePackagePath = FString::Printf(TEXT("/audio/%s"), *SubfolderPath);
			FString FullPackagePath = PackagePath / RelativePackagePath / FPaths::GetBaseFilename(File.Key);

			FARFilter Filter;
			const FString FullAssetPathComposite = FString::Printf(TEXT("%s%s"), *PackagePath, *RelativePackagePath);
			Filter.PackagePaths.Add(FName(*FullAssetPathComposite));
			Filter.ClassPaths.Add(USoundWave::StaticClass()->GetClassPathName());

			TArray<FAssetData> AssetDataList;
			AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

			USoundWave* ExistingSoundWave = nullptr;
			if (AssetDataList.Num() > 0)
			{
				ExistingSoundWave = Cast<USoundWave>(AssetDataList[0].GetAsset());
			}

			UPackage* SoundWavePackage = nullptr;
			USoundWave* ImportedSoundWave = nullptr;

			if (ExistingSoundWave)
			{
				// Update existing asset
				SoundWavePackage = ExistingSoundWave->GetOutermost();
				SoundWavePackage->FullyLoad();
				ImportedSoundWave = AudioEditorModule.ImportSoundWave(SoundWavePackage, ExistingSoundWave->GetName(), TempAudioPath);
			}
			else
			{
				// Create new asset
				SoundWavePackage = CreatePackage(*FullPackagePath);
				SoundWavePackage->FullyLoad();
				ImportedSoundWave = AudioEditorModule.ImportSoundWave(SoundWavePackage, FPaths::GetBaseFilename(File.Key), TempAudioPath);
			}
			
			SoundWavePackage->FullyLoad();

			if (ImportedSoundWave)
			{
				FAssetRegistryModule::AssetCreated(ImportedSoundWave);
				SaveAsset(ImportedSoundWave);
				ImportedAudioMap.Add(RowDataGuid, ImportedSoundWave);
			}
			else
			{
				EditorLOG_WARNING(TEXT("[ImportAudioFiles] Failed to import audio file: %s"), *TempAudioPath);
			}

			// Clean up the temporary file
			IFileManager::Get().Delete(*TempAudioPath);
		}
	}

	if (!DialogueRowsDataTable)
	{
		EditorLOG_WARNING(TEXT("[ImportAudioFiles] Failed to find Dialogue Rows table: %s"), *FullAssetPath);
		return;
	}

	TMap<FName, uint8 *> rowMap = DialogueRowsDataTable->GetRowMap();
	for (auto It = rowMap.CreateIterator(); It; ++It)
	{
		FName RowName = It.Key();
		FDialogueRow* DialogueRow = reinterpret_cast<FDialogueRow*>(It.Value());

		if(!DialogueRow) continue;
		
		for (FDialogueRowData& RowData : DialogueRow->DialogueRowData)
		{
			if (USoundWave** FoundSound = ImportedAudioMap.Find(RowData.RowGUID))
			{
				RowData.RowSound = *FoundSound;
			}
		}
	}

	// Save the updated DataTable
	SaveAsset(DialogueRowsDataTable);
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
	{
		Graph->SetGraphGUID(FGuid(JsonObject->GetStringField("dialogueGuid")));
	}
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

	const FString GameDirectory = FPaths::ProjectDir();
	const FString UpdatedConfigFile = GameDirectory + "/Config/MounteaDialogueImportConfig.ini";

	UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();

	if (FPaths::FileExists(UpdatedConfigFile))
	{
		importConfig->LoadConfig(nullptr, *UpdatedConfigFile);
	}
	else
	{
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
		importConfig->WriteToConfig(Graph->GetGraphGUID(), importSourceData);

		importConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);
	}

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateCategories(UMounteaDialogueGraph* Graph, const FString& Json)
{
	TArray<TSharedPtr<FJsonValue>> dialogueRowsJsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, dialogueRowsJsonArray))
	{
		EditorLOG_ERROR(TEXT("[PopulateCategories] Failed to parse categories.json"));
		return false;
	}

	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	UGameplayTagsSettings* MutableTagsSettings = GetMutableDefault<UGameplayTagsSettings>();

	for (const auto& CategoryValue : dialogueRowsJsonArray)
	{
		TSharedPtr<FJsonObject> Category = CategoryValue->AsObject();
		if (!Category.IsValid())
		{
			continue;
		}

		FString Name = Category->GetStringField("name");
		FString Parent = Category->GetStringField("parent");

		FString FullTag;
		if (Parent.IsEmpty())
		{
			FullTag = FString::Printf(TEXT("MounteaDialogue.Categories.%s"), *Name);
		}
		else
		{
			FullTag = FString::Printf(TEXT("MounteaDialogue.Categories.%s.%s"), *Parent, *Name);
		}

		FGameplayTag ExistingTag = TagsManager.RequestGameplayTag(FName(*FullTag), false);
		if (!ExistingTag.IsValid())
		{
			// If the tag doesn't exist, create a new FGameplayTagTableRow and add it to the config
			FGameplayTagTableRow NewTagRow;
			NewTagRow.Tag = FName(*FullTag);
			NewTagRow.DevComment = FString::Printf(TEXT("Mountea Dialogue Category: %s"), *Name);

			// Check if the tag already exists in the list
			bool bTagExists = false;
			for (const FGameplayTagTableRow& ExistingRow : MutableTagsSettings->GameplayTagList)
			{
				if (ExistingRow.Tag == NewTagRow.Tag)
				{
					bTagExists = true;
					break;
				}
			}

			if (!bTagExists)
			{
				MutableTagsSettings->GameplayTagList.Add(NewTagRow);
			}
		}
	}

	MutableTagsSettings->SaveConfig();

	TagsManager.EditorRefreshGameplayTagTree();

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateParticipants(const UMounteaDialogueGraph* Graph, const FString& Json)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[PopulateParticipants] Invalid Graph object provided to PopulateParticipants"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> dialogueRowsJsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, dialogueRowsJsonArray))
	{
		EditorLOG_ERROR(TEXT("[PopulateParticipants] Failed to parse participants.json"));
		return false;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();

	const FString PackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString AssetName = FString::Printf(TEXT("DT_%s_Participants"), *Graph->GetName());

	UDataTable* ParticipantsDataTable =ParticipantsDataTable = CreateDataTable<FDialogueRow>(AssetTools, PackagePath, AssetName);;

	if (!ParticipantsDataTable)
	{
		EditorLOG_ERROR(TEXT("[PopulateParticipants] Failed to create DataTable asset"));
		return false;
	}

	ParticipantsDataTable->RowStruct = FDialogueParticipant::StaticStruct();

	const UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	FGameplayTagContainer AllTags;
	TagsManager.RequestAllGameplayTags(AllTags, true);

	for (const auto& ParticipantValue : dialogueRowsJsonArray)
	{
		TSharedPtr<FJsonObject> Participant = ParticipantValue->AsObject();
		if (!Participant.IsValid())
		{
			EditorLOG_WARNING(TEXT("[PopulateParticipants] Invalid participant object in JSON"));
			continue;
		}

		FString Name = Participant->GetStringField("name");
		FString Category = Participant->GetStringField("category");

		FDialogueParticipant* NewRow = new FDialogueParticipant();
		NewRow->ParticipantName = FName(*Name);

		FGameplayTag FoundTag;
		for (const FGameplayTag& Tag : AllTags)
		{
			if (Tag.ToString().EndsWith(Category))
			{
				FoundTag = Tag;
				break;
			}
		}

		if (FoundTag.IsValid())
		{
			NewRow->ParticipantCategoryTag = FoundTag;
		}

		ParticipantsDataTable->AddRow(FName(*Name), *NewRow);
	}

	// Process Config Update
	UpdateGraphImportDataConfig(Graph, TEXT("participants.json"), Json, PackagePath, AssetName);
	
	SaveAsset(ParticipantsDataTable);

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateNodes(UMounteaDialogueGraph* Graph, const FString& Json)
{
	if (!Graph || !Graph->GetOutermost()->IsValidLowLevel())
	{
		EditorLOG_ERROR(TEXT("[PopulateNodes] Invalid Graph object or package provided to PopulateNodes"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> dialogueRowsJsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, dialogueRowsJsonArray))
	{
		EditorLOG_ERROR(TEXT("[PopulateNodes] Failed to parse nodes.json"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> LeadNodes, AnswerNodes, CloseDialogueNodes, JumpToNodes, StartNodes;
	TArray<UMounteaDialogueGraphNode*> SpawnedNodes;

	// Categorize nodes
	for (const auto& NodeValue : dialogueRowsJsonArray)
	{
		TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
		if (!NodeObject.IsValid())
		{
			continue;
		}

		const FString NodeType = NodeObject->GetStringField("type");
		if (NodeType == "leadNode")
		{
			LeadNodes.Add(NodeValue);
		}
		else if (NodeType == "answerNode")
		{
			AnswerNodes.Add(NodeValue);
		}
		else if (NodeType == "closeDialogueNode")
		{
			CloseDialogueNodes.Add(NodeValue);
		}
		else if (NodeType == "jumpToNode")
		{
			JumpToNodes.Add(NodeValue);
		}
		else if (NodeType == "startNode")
		{
			StartNodes.Add(NodeValue);
		}
	}

	if (StartNodes.Num() > 0)
	{
		if (Graph && Graph->GetStartNode())
		{
			PopulateNodeData(Graph->GetStartNode(), StartNodes[0]->AsObject());
			SpawnedNodes.Add(Graph->GetStartNode());
		}
		else
		{
			if (UMounteaDialogueGraphNode_StartNode* NewStartNode = Graph->ConstructDialogueNode<UMounteaDialogueGraphNode_StartNode>())
			{
				Graph->StartNode = NewStartNode;
				PopulateNodeData(NewStartNode, StartNodes[0]->AsObject());
				SpawnedNodes.Add(NewStartNode);
			}
		}
	}

	auto CreateNodes = [&](const TArray<TSharedPtr<FJsonValue>>& Nodes, const TSubclassOf<UMounteaDialogueGraphNode> NodeClass)
	{
		for (const auto& Node : Nodes)
		{
			UMounteaDialogueGraphNode* NewNode = Graph->ConstructDialogueNode(NodeClass);
			if (!NewNode) continue;
			
			PopulateNodeData(NewNode, Node->AsObject());
			Graph->AllNodes.Add(NewNode);
			SpawnedNodes.Add(NewNode);
		}
	};

	CreateNodes(LeadNodes, UMounteaDialogueGraphNode_LeadNode::StaticClass());
	CreateNodes(AnswerNodes, UMounteaDialogueGraphNode_AnswerNode::StaticClass());
	CreateNodes(CloseDialogueNodes, UMounteaDialogueGraphNode_CompleteNode::StaticClass());
	CreateNodes(JumpToNodes, UMounteaDialogueGraphNode_ReturnToNode::StaticClass());

	return true;
}

void UMounteaDialogueSystemImportExportHelpers::PopulateNodeData(UMounteaDialogueGraphNode* Node, const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!Node || !JsonObject.IsValid())
	{
		return;
	}

	Node->SetNodeGUID(FGuid(JsonObject->GetStringField("id")));
	Node->NodeTitle = FText::FromString(JsonObject->GetObjectField("data")->GetStringField("title"));
	Node->ExecutionOrder = JsonObject->GetIntegerField(TEXT("executionOrder"));

	TSharedPtr<FJsonObject> AdditionalInfoObject = JsonObject->GetObjectField("data")->GetObjectField("additionalInfo");
	if (AdditionalInfoObject->HasField("targetNodeId") && Node->Graph)
	{
		if (UMounteaDialogueGraphNode_ReturnToNode* returnToNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(Node))
		{
			UMounteaDialogueGraphNode* targetNode = returnToNode->Graph->FindNodeByGuid(FGuid(AdditionalInfoObject->GetStringField("targetNodeId")));
			const int32 targetNodeIndex = returnToNode->Graph->AllNodes.Find(targetNode);
			returnToNode->SelectedNodeIndex = FString::FromInt(targetNodeIndex);
			returnToNode->SelectedNode = returnToNode->Graph->FindNodeByGuid(FGuid(AdditionalInfoObject->GetStringField("targetNodeId")));

			returnToNode->ReturnNodeUpdated.ExecuteIfBound();
		}
	}
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateEdges(UMounteaDialogueGraph* Graph, const FString& Json)
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
		{
			continue;
		}

		FString SourceID = EdgeObject->GetStringField("source");
		FString TargetID = EdgeObject->GetStringField("target");

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

			EdgesCreated++;
		}
		else
		{
			EditorLOG_ERROR(TEXT("[PopulateEdges] Failed to create edge object for: %s -> %s"), *SourceID, *TargetID);
		}
	}

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateDialogueRows(UMounteaDialogueGraph* Graph, const TMap<FString, FString>& ExtractedFiles)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("[PopulateDialogueRows] Invalid Graph object provided to PopulateDialogueRows"));
		return false;
	}

	const FString dialogueRowsJson = ExtractedFiles["dialogueRows.json"];
	const FString dialogueNodesJson = ExtractedFiles["nodes.json"];

	TArray<TSharedPtr<FJsonValue>> dialogueRowsJsonArray;
	TSharedRef<TJsonReader<>> dialogueRowsJsonReader = TJsonReaderFactory<>::Create(dialogueRowsJson);
	if (!FJsonSerializer::Deserialize(dialogueRowsJsonReader, dialogueRowsJsonArray))
	{
		EditorLOG_ERROR(TEXT("[PopulateDialogueRows] Failed to parse dialogueRows.json"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> dialogueNodesJsonArray;
	TSharedRef<TJsonReader<>> dialogueNodesJsonReader = TJsonReaderFactory<>::Create(dialogueNodesJson);
	if (!FJsonSerializer::Deserialize(dialogueNodesJsonReader, dialogueNodesJsonArray))
	{
		EditorLOG_ERROR(TEXT("[PopulateDialogueRows] Failed to parse nodes.json"));
		return false;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();

	FString PackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());

	const FString ParticipantsAssetName = FString::Printf(TEXT("DT_%s_Participants"), *Graph->GetName());
	FString DialogueRowsStringTableName = FString::Printf(TEXT("ST_%s_DialogueRows"), *Graph->GetName());
	FString DialogueRowsDataTableName = FString::Printf(TEXT("DT_%s_DialogueRows"), *Graph->GetName());
	FString DialogueNodesStringTableName = FString::Printf(TEXT("ST_%s_Nodes"), *Graph->GetName());

	// Create StringTables and DataTable
	UStringTable* DialogueRowsStringTable = CreateStringTable(AssetTools, PackagePath, DialogueRowsStringTableName, [&](UStringTable* Table)
	{
		if (!Table) return;
		
		for (const auto& Row : dialogueRowsJsonArray)
		{
			const TSharedPtr<FJsonObject>& RowObject = Row->AsObject();
			FString Id = RowObject->GetStringField("id");
			FString Text = RowObject->GetStringField("text");
			Table->GetMutableStringTable()->SetSourceString(Id, Text);
		}
	});

	UStringTable* NodesStringTable = CreateStringTable(AssetTools, PackagePath, DialogueNodesStringTableName,[&dialogueNodesJsonArray](UStringTable* Table)
	{
		for (const auto& Node : dialogueNodesJsonArray)
		{
			const TSharedPtr<FJsonObject>& NodeObject = Node->AsObject();
			if (!NodeObject.IsValid()) continue;
			
			FString Id = NodeObject->GetStringField("id");
			const TSharedPtr<FJsonObject>& DataObject = NodeObject->GetObjectField("data");
			if (!DataObject.IsValid()) continue;
			
			const TSharedPtr<FJsonObject>& AdditionalInfoObject = DataObject->GetObjectField("additionalInfo");
			if (!AdditionalInfoObject.IsValid()) continue;
			
			FString DisplayName = AdditionalInfoObject->GetStringField("displayName");
			Table->GetMutableStringTable()->SetSourceString(Id, DisplayName);
		}
	});

	UDataTable* DialogueRowsDataTable = CreateDataTable<FDialogueRow>(AssetTools, PackagePath, DialogueRowsDataTableName);

	// Load ParticipantsDataTable
	FString FullAssetPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *ParticipantsAssetName);
	FSoftObjectPath AssetReference(FullAssetPath);
	UDataTable* ParticipantsDataTable = Cast<UDataTable>(AssetReference.TryLoad());

	if (!ParticipantsDataTable)
	{
		EditorLOG_ERROR(TEXT("Failed to load Participants DataTable: %s"), *FullAssetPath);
		return false;
	}

	// Process Config Update
	UpdateGraphImportDataConfig(Graph, TEXT("dialogueRows.json"), dialogueRowsJson, PackagePath, DialogueRowsDataTableName);

	DialogueRowsDataTable->EmptyTable();

	// Create a mapping of node IDs to participant information
	TMap<FString, FString> NodeParticipantMap;
	for (const auto& Node : dialogueNodesJsonArray)
	{
		const TSharedPtr<FJsonObject>& NodeObject = Node->AsObject();
		if (!NodeObject.IsValid()) continue;
		
		FString NodeId = NodeObject->GetStringField("id");
		const TSharedPtr<FJsonObject>& DataObject = NodeObject->GetObjectField("data");
		if (!DataObject.IsValid()) continue;
		
		const TSharedPtr<FJsonObject>& AdditionalInfoObject = DataObject->GetObjectField("additionalInfo");
		if (!AdditionalInfoObject.IsValid()) continue;
		
		const TSharedPtr<FJsonObject>* ParticipantObject = nullptr;
		if (AdditionalInfoObject->TryGetObjectField("participant", ParticipantObject) && ParticipantObject != nullptr)
		{
			FString ParticipantName;
			if ((*ParticipantObject)->TryGetStringField("name", ParticipantName))
			{
				NodeParticipantMap.Add(NodeId, ParticipantName);
			}
			else
			{
				EditorLOG_WARNING(TEXT("[PopulateDialogueRows] Participant name not found for NodeId: %s"), *NodeId);
			}
		}
		else
		{
			EditorLOG_WARNING(TEXT("[PopulateDialogueRows] Participant object not found or invalid for NodeId: %s"), *NodeId);
		}
	}
	
	// Group dialogue rows by nodeId
	TMap<FString, TArray<TSharedPtr<FJsonObject>>> GroupedDialogueRows;
	for (const auto& Row : dialogueRowsJsonArray)
	{
		const TSharedPtr<FJsonObject>& RowObject = Row->AsObject();
		if (RowObject.IsValid())
		{
			FString NodeId = RowObject->GetStringField("nodeId");
			GroupedDialogueRows.FindOrAdd(NodeId).Add(RowObject);
		}
	}
	
	// Populate the DialogueRowsDataTable
	for (const auto& GroupedRow : GroupedDialogueRows)
	{
		const FString& NodeId = GroupedRow.Key;
		const TArray<TSharedPtr<FJsonObject>>& Rows = GroupedRow.Value;

		if (Rows.Num() == 0)
		{
			EditorLOG_WARNING(TEXT("[PopulateDialogueRows] No dialogue Rows associated with NodeId: %s"), *NodeId);
			
			continue;
		}
		
		FDialogueRow NewRow;
		const FGuid newRowGuid = FGuid(GroupedRow.Key);
		NewRow.RowGUID = newRowGuid;

		// Set DialogueParticipant using the NodeParticipantMap
		const FString* ParticipantName = NodeParticipantMap.Find(NodeId);
		if (!ParticipantName || ParticipantName->IsEmpty())
		{
			EditorLOG_WARNING(TEXT("[PopulateDialogueRows] Participant name not valid not found for NodeId %s"), *NodeId);
		
			continue;
		}
		
		FDialogueParticipant* Participant = ParticipantsDataTable->FindRow<FDialogueParticipant>(FName(**ParticipantName), TEXT(""));
		if (!Participant)
		{
			EditorLOG_WARNING(TEXT("[PopulateDialogueRows] Participant not found for NodeId: %s"), *NodeId);
			
			continue;
		}
		
		NewRow.DialogueParticipant = FText::FromString(Participant->ParticipantName.ToString());
		NewRow.CompatibleTags.AddTag(Participant->ParticipantCategoryTag);
		NewRow.RowTitle = FText::FromStringTable(NodesStringTable->GetStringTableId(), NodeId);

		// Add FDialogueRowData for each row in the group
		for (const auto& RowObject : Rows)
		{
			FString Id = RowObject->GetStringField("id");
			FDialogueRowData RowData;
			RowData.RowText = FText::FromStringTable(DialogueRowsStringTable->GetStringTableId(), Id);
			RowData.RowGUID = FGuid(Id);
			RowData.RowDuration = RowObject->GetNumberField("duration");;
			NewRow.DialogueRowData.Add(RowData);
		}		

		TArray<FDialogueRow*> dialogueRows;
		FString outString;
		DialogueRowsDataTable->GetAllRows(outString, dialogueRows);
		
		TArray<FName> rowNames = DialogueRowsDataTable->GetRowNames();
		FString newRowName = NewRow.DialogueParticipant.ToString();
		newRowName.Append(TEXT("_")).Append(FString::FromInt(dialogueRows.Num()));
		DialogueRowsDataTable->AddRow(FName(*newRowName), NewRow);

		// Update nodes
		if (Graph && Graph->GetAllNodes().Num() > 0)
		{
			for (const auto& Node : Graph->GetAllNodes())
			{
				if (!Node || Node->GetNodeGUID() != newRowGuid) continue;
        
				if (UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node))
				{
					DialogueNode->SetDataTable(DialogueRowsDataTable);
					DialogueNode->SetRowName(FName(*newRowName));
				}
			}
		}
	}

	// Save all created assets
	SaveAsset(DialogueRowsStringTable);
	SaveAsset(NodesStringTable);
	SaveAsset(DialogueRowsDataTable);

	return true;
}

UStringTable* UMounteaDialogueSystemImportExportHelpers::CreateStringTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName, TFunction<void(UStringTable*)> PopulateFunction)
{
	UStringTable* StringTable = nullptr;
	
	FSoftObjectPath StringTableAssetPath = PackagePath + TEXT("/") + AssetName + TEXT(".") + AssetName;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData StringTableAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(StringTableAssetPath);
	if (StringTableAssetData.IsValid())
	{
		StringTable = Cast<UStringTable>(StringTableAssetData.GetAsset());
	}
	else
	{
		StringTable = Cast<UStringTable>(AssetTools.CreateAsset(AssetName, PackagePath, UStringTable::StaticClass(), nullptr));
	}
	
	if (StringTable)
	{
		PopulateFunction(StringTable);
	}
	return StringTable;
}

template <typename RowType>
UDataTable* UMounteaDialogueSystemImportExportHelpers::CreateDataTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName)
{
	UDataTable* DataTable = nullptr;

	FSoftObjectPath DataTableAssetPath = PackagePath + TEXT("/") + AssetName + TEXT(".") + AssetName;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData DataTableAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(DataTableAssetPath);
	if (DataTableAssetData.IsValid())
	{
		DataTable = Cast<UDataTable>(DataTableAssetData.GetAsset());
	}
	else
	{
		DataTable = Cast<UDataTable>(
		AssetTools.CreateAsset(AssetName, PackagePath, UDataTable::StaticClass(), nullptr));
	}
	
	if (DataTable)
	{
		DataTable->RowStruct = RowType::StaticStruct();
	}
	
	return DataTable;
}

void UMounteaDialogueSystemImportExportHelpers::SaveAsset(UObject* Asset)
{
	if (!Asset)
		return;

	Asset->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Asset);
	const FString PackageName = Asset->GetOutermost()->GetName();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs saveArgs;
	{
		saveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	}
	UPackage::SavePackage(Asset->GetOutermost(), Asset, *PackageFileName, saveArgs);
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
			OutNodeData.Add(FDialogueNodeData(TEXT("closeDialogueNode"), Node));
		else if (Cast<UMounteaDialogueGraphNode_ReturnToNode>(Node))
			OutNodeData.Add(FDialogueNodeData(TEXT("jumpToNode"), Node));
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
		{
			continue;
		}

		if (!DialogueNode->GetDataTable())
		{
			continue;
		}

		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
		{
			continue;
		}

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
		NodeObject->SetStringField("id", Data.Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
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

	if (!Node)
	{
		EditorLOG_WARNING(TEXT("[AddNodePosition] Invalid Graph or EdGraph for node %s"), *Node->GetName());
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

	TSharedPtr<FJsonObject> AdditionalInfoObject = MakeShareable(new FJsonObject);

	if (const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node))
	{
		AddDialogueNodeData(AdditionalInfoObject, DialogueNode);
	}

	if (const UMounteaDialogueGraphNode_ReturnToNode* JumpNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(Node))
	{
		AddJumpNodeData(AdditionalInfoObject, JumpNode);
	}

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
		RowObject->SetStringField("id", RowData.RowGUID.ToString(EGuidFormats::DigitsWithHyphensLower));
		RowObject->SetStringField("text", RowData.RowText.ToString());
		RowObject->SetStringField("audio", GetRelativeAudioPath(RowData.RowSound, GraphFolder));
		DialogueRowsArray.Add(MakeShareable(new FJsonValueObject(RowObject)));
	}
	AdditionalInfoObject->SetArrayField("dialogueRows", DialogueRowsArray);
}

void UMounteaDialogueSystemImportExportHelpers::AddJumpNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_ReturnToNode* Node)
{
	if (Node && Node->SelectedNode)
	{
		AdditionalInfoObject->SetStringField("targetNodeId", Node->SelectedNode->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
	}
	else
	{
		AdditionalInfoObject->SetStringField("targetNodeId", "");
	}
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

			FString EdgeId = FString::Printf(TEXT("reactflow__edge-%s-%s"), *Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower), *ChildNode->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
			EdgeObject->SetStringField("id", EdgeId);
			EdgeObject->SetStringField("source", Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
			EdgeObject->SetStringField("target", ChildNode->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));
			EdgeObject->SetStringField("type", "customEdge");

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
		{
			RelativePath = RelativePath.RightChop(AudioIndex + 7); // +7 to skip "/audio/"
		}
		else
		{
			EditorLOG_WARNING(TEXT("[ExportAudioFiles] Couldn't find '/audio/' in path: %s"), *AudioFile);
			// Use the full path as a fallback
		}

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

bool UMounteaDialogueSystemImportExportHelpers::PackToMNTEADLG(const TMap<FString, FString>& JsonFiles, const TArray<FString>& ExportedAudioFiles, const FString& OutputPath)
{
	FString ZipFilePath = FPaths::ChangeExtension(OutputPath, TEXT("mnteadlg"));
	
	struct zip_t* zip = zip_open(TCHAR_TO_UTF8(*ZipFilePath), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
	if (!zip)
	{
		EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to create zip file: %s"), *ZipFilePath);
		return false;
	}
	
	for (const auto& JsonFile : JsonFiles)
	{
		if (zip_entry_open(zip, TCHAR_TO_UTF8(*JsonFile.Key)) < 0)
		{
			EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to open zip entry for JSON file: %s"), *JsonFile.Key);
			zip_close(zip);
			return false;
		}

		if (zip_entry_write(zip, TCHAR_TO_UTF8(*JsonFile.Value), JsonFile.Value.Len()) < 0)
		{
			EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to write JSON file to zip: %s"), *JsonFile.Key);
			zip_entry_close(zip);
			zip_close(zip);
			return false;
		}

		zip_entry_close(zip);
	}

	// Always add an empty 'audio' directory
	if (zip_entry_open(zip, "audio/") < 0)
	{
		EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to add 'audio' directory to zip"));
		zip_close(zip);
		return false;
	}
	zip_entry_close(zip);
	
	for (const FString& AudioFile : ExportedAudioFiles)
	{
		FString AudioFileName = FPaths::GetCleanFilename(AudioFile);
		FString ZipAudioPath = FString::Printf(TEXT("audio/%s"), *AudioFileName);

		if (zip_entry_open(zip, TCHAR_TO_UTF8(*ZipAudioPath)) < 0)
		{
			EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to open zip entry for audio file: %s"), *AudioFileName);
			zip_close(zip);
			return false;
		}

		TArray<uint8> AudioFileData;
		if (!FFileHelper::LoadFileToArray(AudioFileData, *AudioFile))
		{
			EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to read audio file: %s"), *AudioFile);
			zip_entry_close(zip);
			zip_close(zip);
			return false;
		}

		if (zip_entry_write(zip, AudioFileData.GetData(), AudioFileData.Num()) < 0)
		{
			EditorLOG_ERROR(TEXT("[PackToMNTEADLG] Failed to write audio file to zip: %s"), *AudioFileName);
			zip_entry_close(zip);
			zip_close(zip);
			return false;
		}

		zip_entry_close(zip);
	}

	zip_close(zip);
	
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
		{
			continue;
		}

		if (!DialogueNode->GetDataTable())
		{
			continue;
		}
		
		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
		{
			continue;
		}
		
		auto CompatibleTags = DialogueRow->CompatibleTags;
		for (const auto& Itr : CompatibleTags)
		{
			AllCategories.Add(Itr);
		}
	}

	TArray<TSharedPtr<FJsonValue>> CategoriesArray;

	for (const FGameplayTag& Tag : AllCategories)
	{
		TSharedPtr<FJsonObject> CategoryObject = MakeShareable(new FJsonObject);

		FString TagString = Tag.ToString();
		const FString Prefix = TEXT("MounteaDialogue.Categories.");
		
		if (TagString.StartsWith(Prefix))
		{
			FString CategoryPath = TagString.RightChop(Prefix.Len());
			TArray<FString> CategoryParts;
			CategoryPath.ParseIntoArray(CategoryParts, TEXT("."));

			if (CategoryParts.Num() > 0)
			{
				FString Name = CategoryParts.Last();
				
				FString Parent;
				for (int32 i = 0; i < CategoryParts.Num() - 1; ++i)
				{
					if (!Parent.IsEmpty())
					{
						Parent += TEXT(".");
					}
					Parent += CategoryParts[i];
				}

				CategoryObject->SetStringField("name", Name);
				CategoryObject->SetStringField("parent", Parent);

				CategoriesArray.Add(MakeShareable(new FJsonValueObject(CategoryObject)));
			}
		}
	}
	
	CategoriesArray.Sort([](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B) {
		const TSharedPtr<FJsonObject>& ObjectA = A->AsObject();
		const TSharedPtr<FJsonObject>& ObjectB = B->AsObject();
		
		const FString NameA = ObjectA->GetStringField("name");
		const FString NameB = ObjectB->GetStringField("name");
		
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
		{
			continue;
		}

		if (!DialogueNode->GetDataTable())
		{
			continue;
		}

		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
		{
			continue;
		}

		FString Category;
		if (DialogueRow->CompatibleTags.Num() > 0)
		{
			Category = DialogueRow->CompatibleTags.First().GetTagName().ToString();
			const FString Prefix = TEXT("MounteaDialogue.Categories.");
			if (Category.StartsWith(Prefix))
			{
				Category = Category.RightChop(Prefix.Len());
			}
		}

		const FString Name = DialogueRow->DialogueParticipant.ToString();

		if (!Category.IsEmpty() && !Name.IsEmpty())
		{
			UniqueParticipants.Add(TPair<FString, FString>(Name, Category));
		}
	}

	TArray<TSharedPtr<FJsonValue>> ParticipantsArray;

	for (const auto& Participant : UniqueParticipants)
	{
		TSharedPtr<FJsonObject> ParticipantObject = MakeShareable(new FJsonObject);
		ParticipantObject->SetStringField("name", Participant.Key);
		ParticipantObject->SetStringField("category", Participant.Value);
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
		{
			continue;
		}

		const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(NodeData.Node);
		if (!DialogueNode)
		{
			continue;
		}

		if (!DialogueNode->GetDataTable())
		{
			continue;
		}

		const FDialogueRow* DialogueRow = DialogueNode->GetDataTable()->FindRow<FDialogueRow>(DialogueNode->GetRowName(), TEXT(""));
		if (!DialogueRow)
		{
			continue;
		}

		for (const FDialogueRowData& RowData : DialogueRow->DialogueRowData)
		{
			TSharedPtr<FJsonObject> RowObject = MakeShareable(new FJsonObject);

			RowObject->SetStringField("id", RowData.RowGUID.ToString(EGuidFormats::DigitsWithHyphensLower));
			RowObject->SetStringField("text", RowData.RowText.ToString());
			RowObject->SetStringField("audioPath", GetRelativeAudioPath(RowData.RowSound, GraphFolder));
			RowObject->SetStringField("nodeId", NodeData.Node->GetNodeGUID().ToString(EGuidFormats::DigitsWithHyphensLower));

			DialogueRowsArray.Add(MakeShareable(new FJsonValueObject(RowObject)));
		}
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(DialogueRowsArray, Writer);

	return OutputString;
}

void UMounteaDialogueSystemImportExportHelpers::ShowNotification(const FText& Message, const float Duration, const FString& BrushName, const FSimpleDelegate& Hyperlink, const FText& HyperlinkText)
{
	FNotificationInfo Info(Message);
	Info.ExpireDuration = Duration;
	Info.Image = FAppStyle::GetBrush(*BrushName);
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
	{
		return TEXT("null");
	}

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
	{
		RelativeAudioPath = FString::Printf(TEXT("audio/%s"), *RelativeAudioPath);
	}

	return RelativeAudioPath;
}

#undef LOCTEXT_NAMESPACE