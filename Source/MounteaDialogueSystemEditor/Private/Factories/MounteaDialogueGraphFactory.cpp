// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraphFactory.h"

#include "Sound/SoundWave.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"

#include "Data/MounteaDialogueGraphDataTypes.h"

#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsSettings.h"

#include "Engine/DataTable.h"

#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "zip.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphFactory"

UMounteaDialogueGraphFactory::UMounteaDialogueGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UMounteaDialogueGraph::StaticClass();
	bEditorImport = true;
	Formats.Add("mnteadlg;Mountea Dialogue File");
}

UObject* UMounteaDialogueGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
														EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UObject>(InParent, Class, Name, Flags | RF_Transactional);
}

UObject* UMounteaDialogueGraphFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName,
														EObjectFlags Flags, const FString& Filename, const TCHAR* Parms,
														FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	if (!FactoryCanImport(Filename)) return nullptr;

	// 1. Load the file
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *Filename))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *Filename);
		bOutOperationCanceled = true;
		return nullptr;
	}

	// 2. Make sure it's a zip file
	if (!IsZipFile(FileData))
	{
		UE_LOG(LogTemp, Error, TEXT("File is not a valid zip: %s"), *Filename);
		bOutOperationCanceled = true;
		return nullptr;
	}

	// 3. Extract and read content
	TMap<FString, FString> ExtractedFiles;
	if (!ExtractFilesFromZip(FileData, ExtractedFiles))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract files from zip: %s"), *Filename);
		bOutOperationCanceled = true;
		return nullptr;
	}

	// 4. Validate content
	if (!ValidateExtractedContent(ExtractedFiles))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid content in file: %s"), *Filename);
		bOutOperationCanceled = true;
		return nullptr;
	}

	// 5. Read dialogue name from dialogueData.json
	FString DialogueName;
	if (ExtractedFiles.Contains("dialogueData.json"))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ExtractedFiles["dialogueData.json"]);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->HasField("dialogueName"))
			{
				DialogueName = JsonObject->GetStringField("dialogueName");
				UE_LOG(LogTemp, Log, TEXT("Found dialogueName in JSON: %s"), *DialogueName);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("dialogueName field not found in dialogueData.json"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to parse dialogueData.json"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("dialogueData.json not found in extracted files"));
	}

	// Use DialogueName if it's not empty, otherwise use InName
	FName AssetName = !DialogueName.IsEmpty() ? FName(*DialogueName) : InName;
	UE_LOG(LogTemp, Log, TEXT("Using asset name: %s"), *AssetName.ToString());

	// 6. Create UMounteaDialogueGraph if all is good
	UMounteaDialogueGraph* NewGraph = NewObject<UMounteaDialogueGraph>(InParent, InClass, AssetName, Flags);
	if (NewGraph)
	{
		UE_LOG(LogTemp, Log, TEXT("Created new graph with name: %s"), *NewGraph->GetName());
		if (PopulateGraphFromExtractedFiles(NewGraph, ExtractedFiles))
		{
			// 7. Import audio files if present
			ImportAudioFiles(ExtractedFiles, InParent, Flags);

			NewGraph->CreateGraph();
			if (NewGraph->EdGraph)
			{
				if (UEdGraph_MounteaDialogueGraph* edGraph = Cast<UEdGraph_MounteaDialogueGraph>(NewGraph->EdGraph))
					edGraph->RebuildMounteaDialogueGraph();
			}
			
			return NewGraph;
		}
		UE_LOG(LogTemp, Error, TEXT("Failed to populate graph from extracted files: %s"), *Filename);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create new graph object"));
	}

	bOutOperationCanceled = true;

	// Cleanup
	for (const auto& File : ExtractedFiles)
	{
		if (File.Key.StartsWith("audio/"))
		{
			IFileManager::Get().Delete(*File.Value);
		}
	}

	return nullptr;
}

UObject* UMounteaDialogueGraphFactory::ImportObject(UClass* InClass, UObject* InOuter, FName InName,
													EObjectFlags Flags, const FString& Filename, const TCHAR* Parms,
													bool& OutCanceled)
{
	if (!FactoryCanImport(Filename)) return nullptr;
	return FactoryCreateFile(InClass, InOuter, InName, Flags, Filename, Parms, nullptr, OutCanceled);
}

bool UMounteaDialogueGraphFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);
	return (Extension == TEXT("mnteadlg") || Extension == TEXT("zip"));
}

bool UMounteaDialogueGraphFactory::IsZipFile(const TArray<uint8>& FileData)
{
	// ZIP files can start with several different signatures
	const uint32 ZIP_SIGNATURE_1 = 0x04034b50; // Regular ZIP file
	const uint32 ZIP_SIGNATURE_2 = 0x08074b50; // Spanned ZIP file
	const uint32 ZIP_SIGNATURE_3 = 0x02014b50; // Central directory header
	const uint32 ZIP_SIGNATURE_4 = 0x06054b50; // End of central directory record

	if (FileData.Num() < 4)
	{
		UE_LOG(LogTemp, Warning, TEXT("File is too small to be a valid ZIP file"));
		return false;
	}

	// Read the first 4 bytes as a uint32
	uint32 Signature = (FileData[3] << 24) | (FileData[2] << 16) | (FileData[1] << 8) | FileData[0];

	// Check against known ZIP signatures
	if (Signature == ZIP_SIGNATURE_1 ||
		Signature == ZIP_SIGNATURE_2 ||
		Signature == ZIP_SIGNATURE_3 ||
		Signature == ZIP_SIGNATURE_4)
	{
		return true;
	}

	// Additional checks could be performed here, such as looking for the end of central directory record

	UE_LOG(LogTemp, Warning, TEXT("File does not have a valid ZIP signature"));
	return false;
}

bool UMounteaDialogueGraphFactory::ExtractFilesFromZip(const TArray<uint8>& ZipData,
														TMap<FString, FString>& OutExtractedFiles)
{
	// Create a temporary file to store the zip data
	FString TempFilePath = FPaths::CreateTempFilename(FPlatformProcess::UserTempDir(), TEXT("MounteaDialogue"),
													TEXT(".zip"));
	if (!FFileHelper::SaveArrayToFile(ZipData, *TempFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save zip data to temporary file"));
		return false;
	}

	// Open the zip archive
	struct zip_t* zip = zip_open(TCHAR_TO_UTF8(*TempFilePath), 0, 'r');
	if (!zip)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to open zip file"));
		IFileManager::Get().Delete(*TempFilePath);
		return false;
	}

	int i, n = zip_entries_total(zip);
	for (i = 0; i < n; ++i)
	{
		zip_entry_openbyindex(zip, i);
		{
			const char* name = zip_entry_name(zip);
			int size = zip_entry_size(zip);

			// Allocate buffer for file content
			TArray<uint8> buffer;
			buffer.SetNum(size);

			// Read file content
			if (zip_entry_noallocread(zip, buffer.GetData(), size) != -1)
			{
				// Convert to FString and add to output map
				FString FileName = UTF8_TO_TCHAR(name);
				if (FileName.StartsWith(TEXT("audio/")) && FileName.EndsWith(TEXT(".wav")))
				{
					// For audio files, store the file path
					OutExtractedFiles.Add(FileName, TempFilePath);
				}
				else
				{
					// For other files, store the content as string
					FString FileContent = BytesToString(buffer.GetData(), size);
					OutExtractedFiles.Add(FileName, FileContent);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to read file content: %hs"), UTF8_TO_TCHAR(name));
			}
		}
		zip_entry_close(zip);
	}

	zip_close(zip);

	return true;
}

FString UMounteaDialogueGraphFactory::BytesToString(const uint8* Bytes, int32 Count)
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

bool UMounteaDialogueGraphFactory::ValidateExtractedContent(const TMap<FString, FString>& ExtractedFiles)
{
	const TArray<FString> RequiredFiles = {
		"categories.json",
		"participants.json",
		"nodes.json",
		"edges.json",
		"dialogueData.json",
		"dialogueRows.json"
	};

	for (const auto& File : RequiredFiles)
	{
		if (!ExtractedFiles.Contains(File))
		{
			UE_LOG(LogTemp, Error, TEXT("Missing required file: %s"), *File);
			return false;
		}
	}

	// TODO: Add more specific validation

	return true;
}

bool UMounteaDialogueGraphFactory::PopulateGraphFromExtractedFiles(UMounteaDialogueGraph* Graph,
																	const TMap<FString, FString>& ExtractedFiles)
{
	if (!PopulateDialogueData(Graph, ExtractedFiles["dialogueData.json"]))
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

	if (!PopulateDialogueRows(Graph, ExtractedFiles["dialogueRows.json"]))
	{
		return false;
	}

	return true;
}

void UMounteaDialogueGraphFactory::ImportAudioFiles(const TMap<FString, FString>& ExtractedFiles, UObject* InParent,
													EObjectFlags Flags)
{
	for (const auto& File : ExtractedFiles)
	{
		if (File.Key.StartsWith("audio/") && File.Key.EndsWith(".wav"))
		{
			FString SourceFilePath = File.Value;
			FString DestinationPath = FPaths::Combine(FPaths::GetPath(InParent->GetPathName()),
													FPaths::GetBaseFilename(File.Key) + TEXT(".wav"));

			// Copy the audio file to the project's content directory
			if (IFileManager::Get().Copy(*DestinationPath, *SourceFilePath) == COPY_OK)
			{
				// Import the audio file
				UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
				ImportData->Filenames.Add(DestinationPath);
				ImportData->DestinationPath = FPaths::GetPath(InParent->GetPathName());
				ImportData->bReplaceExisting = true;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);

				// Link the imported audio to your dialogue system as needed
				if (ImportedAssets.Num() > 0)
				{
					USoundWave* SoundWave = Cast<USoundWave>(ImportedAssets[0]);
					if (SoundWave)
					{
						// TODO: Link this SoundWave to your dialogue system
					}
				}
			}
		}
	}
}

bool UMounteaDialogueGraphFactory::PopulateDialogueData(UMounteaDialogueGraph* Graph, const FString& Json)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		Graph->SetGraphGUID(FGuid(JsonObject->GetStringField("dialogueGuid")));
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to parse dialogueData.json during population"));
	return false;
}

bool UMounteaDialogueGraphFactory::PopulateCategories(UMounteaDialogueGraph* Graph, const FString& Json)
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse categories.json"));
		return false;
	}

	// Get the GameplayTagsManager
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	// Get the mutable config for GameplayTags
	UGameplayTagsSettings* MutableTagsSettings = GetMutableDefault<UGameplayTagsSettings>();

	for (const auto& CategoryValue : JsonArray)
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

		// Check if the tag already exists
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

		// For demonstration purposes, let's log each tag we're creating
		UE_LOG(LogTemp, Log, TEXT("Creating tag: %s"), *FullTag);
	}

	// Save the updated config
	MutableTagsSettings->SaveConfig();

	// Notify the GameplayTagsManager that we've added new tags
	TagsManager.EditorRefreshGameplayTagTree();

	// Notify the user
	FNotificationInfo Info(FText(LOCTEXT("DialogueCategoriesCreated", "Created Dialogue Categories")));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	return true;
}

bool UMounteaDialogueGraphFactory::PopulateParticipants(UMounteaDialogueGraph* Graph, const FString& Json)
{
	if (!Graph)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Graph object provided to PopulateParticipants"));
		return false;
	}

	// Parse JSON
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse participants.json"));
		return false;
	}

	// Create a new DataTable asset
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	// Determine the package path for the new asset
	FString PackagePath = FPackageName::GetLongPackagePath(Graph->GetPathName());
	FString AssetName = FString::Printf(TEXT("%s_Participants"), *Graph->GetName());

	UDataTable* ParticipantsDataTable = Cast<UDataTable>(
		AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UDataTable::StaticClass(), nullptr));

	if (!ParticipantsDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create DataTable asset"));
		return false;
	}

	ParticipantsDataTable->RowStruct = FDialogueParticipant::StaticStruct();

	// Get the GameplayTagsManager
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	// Get all existing tags
	FGameplayTagContainer AllTags;
	TagsManager.RequestAllGameplayTags(AllTags, true);

	for (const auto& ParticipantValue : JsonArray)
	{
		TSharedPtr<FJsonObject> Participant = ParticipantValue->AsObject();
		if (!Participant.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid participant object in JSON"));
			continue;
		}

		FString Name = Participant->GetStringField("name");
		FString Category = Participant->GetStringField("category");

		FDialogueParticipant* NewRow = new FDialogueParticipant();
		NewRow->ParticipantName = FName(*Name);

		// Find the correct GameplayTag
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
			UE_LOG(LogTemp, Log, TEXT("Found tag for participant %s: %s"), *Name, *FoundTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find GameplayTag for category: %s"), *Category);
		}

		// Add the new row to the DataTable
		ParticipantsDataTable->AddRow(FName(*Name), *NewRow);
	}

	// Save the DataTable asset
	FAssetRegistryModule::AssetCreated(ParticipantsDataTable);
	ParticipantsDataTable->MarkPackageDirty();

	// Notify the user
	FNotificationInfo Info(FText::Format(
		LOCTEXT("ParticipantsDataTableCreated", "Created Participants DataTable: {0}"), FText::FromString(AssetName)));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	return true;
}

bool UMounteaDialogueGraphFactory::PopulateNodes(UMounteaDialogueGraph* Graph, const FString& Json)
{
	if (!Graph || !Graph->GetOutermost()->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Graph object or package provided to PopulateNodes"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse nodes.json"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> LeadNodes, AnswerNodes, CloseDialogueNodes, JumpToNodes, StartNodes;
	TArray<UMounteaDialogueGraphNode*> SpawnedNodes;
	
	// Categorize nodes
	for (const auto& NodeValue : JsonArray)
	{
		TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
		if (!NodeObject.IsValid())
		{
			continue;
		}

		FString NodeType = NodeObject->GetStringField("type");
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
			UMounteaDialogueGraphNode_StartNode* NewStartNode = Graph->ConstructDialogueNode<
				UMounteaDialogueGraphNode_StartNode>();
			if (NewStartNode)
			{
				Graph->StartNode = NewStartNode;
				PopulateNodeData(NewStartNode, StartNodes[0]->AsObject());
				SpawnedNodes.Add(NewStartNode);
			}
		}
	}

	auto CreateNodes = [&](const TArray<TSharedPtr<FJsonValue>>& Nodes,
							TSubclassOf<UMounteaDialogueGraphNode> NodeClass)
	{
		for (const auto& Node : Nodes)
		{
			UMounteaDialogueGraphNode* NewNode = Graph->ConstructDialogueNode(NodeClass);
			if (NewNode)
			{
				PopulateNodeData(NewNode, Node->AsObject());
				Graph->AllNodes.Add(NewNode);
				SpawnedNodes.Add(NewNode);
			}
		}
	};
	
	CreateNodes(LeadNodes, UMounteaDialogueGraphNode_LeadNode::StaticClass());
	CreateNodes(AnswerNodes, UMounteaDialogueGraphNode_AnswerNode::StaticClass());
	CreateNodes(CloseDialogueNodes, UMounteaDialogueGraphNode_CompleteNode::StaticClass());
	CreateNodes(JumpToNodes, UMounteaDialogueGraphNode_ReturnToNode::StaticClass());

	// Notify the user
	FNotificationInfo Info(FText::Format(
		LOCTEXT("DialogueNodesCreated", "Populated nodes\n{0} Lead\n{1} Answer\n{2} Close\n{3} Jump"),
		LeadNodes.Num(), AnswerNodes.Num(), CloseDialogueNodes.Num(), JumpToNodes.Num()));
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	return true;
}

void UMounteaDialogueGraphFactory::PopulateNodeData(UMounteaDialogueGraphNode* Node,
													const TSharedPtr<FJsonObject>& JsonObject)
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
			returnToNode->SelectedNode = returnToNode->Graph->FindNodeByGuid(
				FGuid(AdditionalInfoObject->GetStringField("targetNodeId")));
		}
	}
}

bool UMounteaDialogueGraphFactory::PopulateEdges(UMounteaDialogueGraph* Graph, const FString& Json)
{
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Graph object provided to PopulateEdges"));
        return false;
    }

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, JsonArray))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse edges.json"));
        return false;
    }

    int32 EdgesCreated = 0;

    for (const auto& EdgeValue : JsonArray)
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
            UE_LOG(LogTemp, Warning, TEXT("Could not find source or target node for edge: %s -> %s"), *SourceID, *TargetID);
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
            UE_LOG(LogTemp, Error, TEXT("Failed to create edge object for: %s -> %s"), *SourceID, *TargetID);
        }
    }

    // Notify the user
    FNotificationInfo Info(FText::Format(
        LOCTEXT("DialogueEdgesCreated", "Created {0} edges"),
        EdgesCreated));
    Info.ExpireDuration = 5.0f;
    FSlateNotificationManager::Get().AddNotification(Info);

    return true;
}

bool UMounteaDialogueGraphFactory::PopulateDialogueRows(UMounteaDialogueGraph* Graph, const FString& Json)
{
	// TODO: Implement JSON parsing and population of dialogue rows
	return true;
}

#undef LOCTEXT_NAMESPACE
