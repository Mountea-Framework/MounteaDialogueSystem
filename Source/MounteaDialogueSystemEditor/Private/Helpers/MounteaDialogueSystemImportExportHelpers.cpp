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

#include "UObject/SavePackage.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemImportExportHelpers"

bool UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraph(const FString &FilePath, UObject *InParent, FName Name, EObjectFlags Flags, UMounteaDialogueGraph *&OutGraph)
{
	// 1. Load the file
	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *FilePath))
	{
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] Failed to load file: %s"), *FilePath);
		return nullptr;
	}

	// 2. Make sure it's a zip file
	if (!IsZipFile(fileData))
	{
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] File is not a valid mnteadlg/zip: %s"), *FilePath);
		return nullptr;
	}

	// 3. Extract and read content
	TMap<FString, FString> extractedFiles;
	if (!ExtractFilesFromZip(fileData, extractedFiles))
	{
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] Failed to extract files from archive: %s"), *FilePath);
		return nullptr;
	}

	// 4. Validate content
	if (!ValidateExtractedContent(extractedFiles))
	{
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] Invalid content in file: %s"), *FilePath);
		return nullptr;
	}

	// 5. Read dialogue name from dialogueData.json
	FString dialogueName;
	if (extractedFiles.Contains("dialogueData.json"))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(extractedFiles["dialogueData.json"]);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasField("dialogueName"))
			{
				dialogueName = JsonObject->GetStringField("dialogueName");
			}
			else
			{
				EditorLOG_WARNING(TEXT("dialogueName field not found in dialogueData.json"));
			}
		}
		else
		{
			EditorLOG_ERROR(TEXT("Failed to parse dialogueData.json"));
		}
	}
	else
	{
		EditorLOG_WARNING(TEXT("dialogueData.json not found in extracted files"));
	}

	const FName assetName = !dialogueName.IsEmpty() ? FName(*dialogueName) : Name;

	// 6. Process UMounteaDialogueGraph if all is good
	if (OutGraph)
	{
		OutGraph->Rename(*assetName.ToString());
		
		if (PopulateGraphFromExtractedFiles(OutGraph, extractedFiles, FilePath))
		{
			// 7. Import audio files if present
			ImportAudioFiles(extractedFiles, InParent, OutGraph, Flags);

			OutGraph->CreateGraph();
			if (OutGraph->EdGraph)
			{
				if (UEdGraph_MounteaDialogueGraph* edGraph = Cast<UEdGraph_MounteaDialogueGraph>(OutGraph->EdGraph))
					edGraph->RebuildMounteaDialogueGraph();
			}

			SaveAsset(OutGraph);

			return true;
		}
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] Failed to populate graph from extracted files: %s"), *FilePath);
	}
	else
	{
		EditorLOG_ERROR(TEXT("[FactoryCreateFile] Failed to create new graph object"));
	}

	// Cleanup
	for (const auto& File : extractedFiles)
	{
		if (File.Key.StartsWith("audio/"))
		{
			IFileManager::Get().Delete(*File.Value);
		}
	}

	return nullptr;
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

FString UMounteaDialogueSystemImportExportHelpers::BytesToString(const uint8* Bytes, int32 Count)
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

void UMounteaDialogueSystemImportExportHelpers::ImportAudioFiles(const TMap<FString, FString>& ExtractedFiles, UObject* InParent, UMounteaDialogueGraph* Graph, EObjectFlags Flags)
{
	FModuleManager::LoadModuleChecked<IAudioEditorModule>("AudioEditor");
	IAudioEditorModule& AudioEditorModule = FModuleManager::GetModuleChecked<IAudioEditorModule>("AudioEditor");

	// Get the Dialogue Rows DataTable
	FString PackagePath = FPackageName::GetLongPackagePath(InParent->GetPathName());
    
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

			FString DestinationPath = FPaths::Combine(FPaths::GetPath(InParent->GetPathName()), TEXT("Audio"), SubfolderPath);
            
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*DestinationPath);
            
			FString FullDestinationPath = FPaths::Combine(DestinationPath, FPaths::GetCleanFilename(File.Key));
            
			FString RelativePackagePath = FString::Printf(TEXT("/Audio/%s"), *SubfolderPath);
			FString FullPackagePath = PackagePath / RelativePackagePath / FPaths::GetBaseFilename(File.Key);
			UPackage* SoundWavePackage = CreatePackage(*FullPackagePath);
            
			SoundWavePackage->FullyLoad();
            
			USoundWave* ImportedSoundWave = AudioEditorModule.ImportSoundWave(SoundWavePackage, FPaths::GetBaseFilename(File.Key), TempAudioPath);

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

	Graph->SourceFile = SourceFilePath;
	for (const auto& Itr : ExtractedFiles)
	{
		FDialogueImportData NewSourceData;
		NewSourceData.JsonFile = Itr.Key;
		NewSourceData.JsonData = Itr.Value;
		Graph->SourceData.Add(NewSourceData);
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DialogueDataJson);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		Graph->SetGraphGUID(FGuid(JsonObject->GetStringField("dialogueGuid")));
		return true;
	}

	EditorLOG_ERROR(TEXT("[PopulateDialogueData] Failed to parse dialogueData.json during population"));
	return false;
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

	// Notify the user
	FNotificationInfo Info(FText(LOCTEXT("DialogueCategoriesCreated", "Created Dialogue Categories")));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::PopulateParticipants(UMounteaDialogueGraph* Graph, const FString& Json)
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

	const FString PackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	const FString AssetName = FString::Printf(TEXT("DT_%s_Participants"), *Graph->GetName());

	UDataTable* ParticipantsDataTable = Cast<UDataTable>(
		AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UDataTable::StaticClass(), nullptr));

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

	SaveAsset(ParticipantsDataTable);

	// Notify the user
	FNotificationInfo Info(FText::Format(
		LOCTEXT("ParticipantsDataTableCreated", "Created Participants DataTable: {0}"), FText::FromString(AssetName)));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

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

	// Notify the user
	FNotificationInfo Info(
		FText::Format(
			LOCTEXT("DialogueNodesCreated", "Populated nodes\n{0} Lead\n{1} Answer\n{2} Close\n{3} Jump"),
			LeadNodes.Num(), AnswerNodes.Num(), CloseDialogueNodes.Num(), JumpToNodes.Num()));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

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

	// Notify the user
	FNotificationInfo Info(FText::Format(LOCTEXT("DialogueEdgesCreated", "Created {0} edges"), EdgesCreated));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

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
			NewRow.DialogueRowData.Add(RowData);
		}

		TArray<FDialogueRow*> dialogueRows;
		FString outString;
		DialogueRowsDataTable->GetAllRows(outString, dialogueRows);
		FString rowName = NewRow.DialogueParticipant.ToString();
		rowName.Append(TEXT("_")).Append(FString::FromInt(dialogueRows.Num()));
		DialogueRowsDataTable->AddRow(FName(*rowName), NewRow);

		// Update nodes
		if (Graph && Graph->GetAllNodes().Num() > 0)
		{
			for (const auto& Node : Graph->GetAllNodes())
			{
				if (!Node || Node->GetNodeGUID() != newRowGuid) continue;
				
				if (UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node))
				{
					DialogueNode->SetDataTable(DialogueRowsDataTable);
					DialogueNode->SetRowName(FName(*rowName));
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
	UStringTable* StringTable = Cast<UStringTable>(AssetTools.CreateAsset(AssetName, PackagePath, UStringTable::StaticClass(), nullptr));
	if (StringTable)
	{
		PopulateFunction(StringTable);
	}
	return StringTable;
}

template <typename RowType>
UDataTable* UMounteaDialogueSystemImportExportHelpers::CreateDataTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName)
{
	UDataTable* DataTable = Cast<UDataTable>(
		AssetTools.CreateAsset(AssetName, PackagePath, UDataTable::StaticClass(), nullptr));
	if (DataTable)
	{
		DataTable->RowStruct = RowType::StaticStruct();
	}
	return DataTable;
}

void UMounteaDialogueSystemImportExportHelpers::SaveAsset(UObject* Asset)
{
	if (!Asset) return;
	
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

#undef LOCTEXT_NAMESPACE