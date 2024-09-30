// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueSystemImportExportHelpers.generated.h"


class UMounteaDialogueGraphNode_ReturnToNode;
class UMounteaDialogueGraphNode_DialogueNodeBase;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class IAssetTools;

// Define a struct to hold node data
struct FDialogueNodeData
{
	FString Type;
	UMounteaDialogueGraphNode* Node;
	
	FDialogueNodeData(const FString &InType, UMounteaDialogueGraphNode *InNode) : Type(InType), Node(InNode) {}
};

/**
 *
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueSystemImportExportHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	// Main reimport logic
	static bool IsReimport(const FString& Filename);
	static bool ReimportDialogueGraph(const FString& FilePath, UObject* ObjectRedirector, UMounteaDialogueGraph*& OutGraph);
	static bool CanReimport(UObject* ObjectRedirector, TArray<FString>& OutFilenames);

	static void UpdateGraphImportDataConfig(const UMounteaDialogueGraph* Graph, const FString& JsonName, const FString& Json, const FString& PackagePath, const FString& AssetName);
	
	// Main import function
	static bool ImportDialogueGraph(const FString& FilePath, UObject* InParent, FName Name, EObjectFlags Flags, UMounteaDialogueGraph*& OutGraph);
	
	// Main export function
	static bool ExportDialogueGraph(const UMounteaDialogueGraph* Graph, const FString& FilePath);
	
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
	static bool PopulateParticipants(const UMounteaDialogueGraph* Graph, const FString& Json);
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
	
	// Helper functions for export process
	static bool GatherAssetsFromGraph(const UMounteaDialogueGraph* Graph, TMap<FString, FString>& OutJsonFiles, TArray<FString>& OutAudioFiles);
	static bool ExportAudioFiles(const TArray<FString>& AudioFiles, const FString& ExportPath, TArray<FString>& OutExportedFiles);
	static bool PackToMNTEADLG(const TMap<FString, FString>& JsonFiles, const TArray<FString>& ExportedAudioFiles, const FString& OutputPath);
	
	// Helper functions for gathering specific parts of the graph
	static void GatherNodesFromGraph(const UMounteaDialogueGraph* Graph, TArray<FDialogueNodeData>& OutNodeData);
	static bool GatherAudioFiles(const UMounteaDialogueGraph* Graph, TArray<FString>& OutAudioFiles);

	// Helper function to get relative audio paths
	static FString GetRelativeAudioPath(const USoundBase* Sound, const FString& GraphFolder);
	static void CreateWAVFile(const TArray<uint8>& InPCMData, uint32 InSampleRate, uint16 InNumChannels, TArray<uint8>& OutWAVData);
	
	// Helper functions to generate JSON files
	static FString CreateNodesJson(const TArray<FDialogueNodeData>& NodeData);
	static void AddNodePosition(const TSharedPtr<FJsonObject>& NodeObject, const UMounteaDialogueGraphNode* Node);
	static void AddNodeData(const TSharedPtr<FJsonObject>& NodeObject, const UMounteaDialogueGraphNode* Node);
	static void AddDialogueNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode);
	static void AddJumpNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_ReturnToNode* Node);
	static FString CreateEdgesJson(const UMounteaDialogueGraph* Graph);
	static FString CreateCategoriesJson(const UMounteaDialogueGraph* Graph);
	static FString CreateDialogueDataJson(const UMounteaDialogueGraph* Graph);
	static FString CreateParticipantsJson(const UMounteaDialogueGraph* Graph);
	static FString CreateDialogueRowsJson(const TArray<FDialogueNodeData>& AllNodeData, const UMounteaDialogueGraph* Graph);
};
