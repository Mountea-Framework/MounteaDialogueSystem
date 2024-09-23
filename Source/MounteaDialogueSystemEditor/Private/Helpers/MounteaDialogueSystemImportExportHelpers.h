// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueSystemImportExportHelpers.generated.h"


class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class IAssetTools;

/**
 *
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueSystemImportExportHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	// TODO:
	// 1. Export logic
	// 1.1 Gather assets from Graph
	// 1.2 Translate to JSON files
	// 1.3 Export audio
	// 1.4 Pack to custom .mnteadlg file (zip)

public:
	// Main import function
	static bool ImportDialogueGraph(const FString& FilePath, UObject* InParent, FName Name, EObjectFlags Flags, UMounteaDialogueGraph*& OutGraph);

	// Helper functions for import process
	static bool IsZipFile(const TArray<uint8>& FileData);
	static bool ExtractFilesFromZip(const TArray<uint8>& ZipData, TMap<FString, FString>& OutExtractedFiles);
	static bool ValidateExtractedContent(const TMap<FString, FString>& ExtractedFiles);
	static bool PopulateGraphFromExtractedFiles(UMounteaDialogueGraph* Graph, const TMap<FString, FString>& ExtractedFiles, const FString& SourceFilePath);
	static void ImportAudioFiles(const TMap<FString, FString>& ExtractedFiles, UObject* InParent, UMounteaDialogueGraph* Graph, EObjectFlags Flags);

private:
	// Helper functions for populating specific parts of the graph
	static bool PopulateDialogueData(UMounteaDialogueGraph* Graph, const FString& SourceFilePath, const TMap<FString, FString>& ExtractedFiles);
	static bool PopulateCategories(UMounteaDialogueGraph* Graph, const FString& Json);
	static bool PopulateParticipants(UMounteaDialogueGraph* Graph, const FString& Json);
	static bool PopulateNodes(UMounteaDialogueGraph* Graph, const FString& Json);
	static bool PopulateEdges(UMounteaDialogueGraph* Graph, const FString& Json);
	static bool PopulateDialogueRows(UMounteaDialogueGraph* Graph, const TMap<FString, FString>& ExtractedFiles);

	// Utility functions
	static FString BytesToString(const uint8* Bytes, int32 Count);
	static void PopulateNodeData(UMounteaDialogueGraphNode* Node, const TSharedPtr<FJsonObject>& JsonObject);
	static UStringTable* CreateStringTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName, TFunction<void(UStringTable*)> PopulateFunction);
	template <typename RowType>
	static UDataTable* CreateDataTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName);
	static void SaveAsset(UObject* Asset);

	// 2. Import logic
	// Move all that is inside the Factory Class
};
