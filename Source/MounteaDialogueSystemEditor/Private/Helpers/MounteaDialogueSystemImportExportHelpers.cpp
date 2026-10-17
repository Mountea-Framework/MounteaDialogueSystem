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

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemImportExportHelpers"

bool UMounteaDialogueSystemImportExportHelpers::CanReimport(UObject* ObjectRedirector, TArray<FString>& OutFilenames)
{
	if (ObjectRedirector == nullptr) 
		return false;

	UMounteaDialogueGraph* dialogueGraph = Cast<UMounteaDialogueGraph>(ObjectRedirector);
	if (dialogueGraph == nullptr) 
		return false;
	
	return true;
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
		UMounteaDialogueImportConfig* importConfig = GetMutableDefault<UMounteaDialogueImportConfig>();

		FString assetPath;
		if (importConfig && importConfig->LookupDialogueAssetPath(ImportedGuid, assetPath))
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

	return nullptr;
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

#undef LOCTEXT_NAMESPACE
