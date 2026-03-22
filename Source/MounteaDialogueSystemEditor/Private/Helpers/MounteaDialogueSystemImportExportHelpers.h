// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueSystemImportExportHelpers.generated.h"


class USoundWave;
class UMounteaDialogueGraphNode_ReturnToNode;
class UMounteaDialogueGraphNode_OpenChildGraph;
class UMounteaDialogueGraphNode_DialogueNodeBase;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class UMounteaDialogueConditionBase;
class UMounteaDialogueDecoratorBase;
class UMounteaDialogueGraphEdge;
class IAssetTools;
class IAudioEditorModule;
class FAssetRegistryModule;

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

	static void ShowNotification(const FText& Message, const float Duration, const FString& BrushName = TEXT("MDSStyleSet.Icon.Success"), const FSimpleDelegate& Hyperlink = FSimpleDelegate(), const FText& HyperlinkText = FText());

	// Main reimport logic
	static bool IsReimport(const FString& Filename);
	static bool ReimportDialogueGraph(const FString& FilePath, UObject* ObjectRedirector, UMounteaDialogueGraph*& OutGraph, FString& OutMessage);
	static bool CanReimport(UObject* ObjectRedirector, TArray<FString>& OutFilenames);

	/**
	 * Resolves pending OpenChildGraph targets after all graphs have been imported.
	 *
	 * During import, OpenChildGraph nodes store the Dialoguer target GUID in
	 * PendingTargetDialogueGUID instead of resolving immediately (because the target
	 * graph may not exist yet). Call this once all graphs are available.
	 *
	 * @param Graph       The graph whose OpenChildGraph nodes should be resolved.
	 * @param KnownGraphs Optional map of GUID → graph for graphs imported in the same batch.
	 *                    Checked before falling back to a full asset-registry scan.
	 */
	static void ResolveOpenChildGraphTargets(UMounteaDialogueGraph* Graph, const TMap<FGuid, UMounteaDialogueGraph*>& KnownGraphs);

	// Main import function — loads/extracts then delegates to ImportDialogueGraphFromFiles
	static bool ImportDialogueGraph(const FString& FilePath, UObject* InParent, FName Name, EObjectFlags Flags, UMounteaDialogueGraph*& OutGraph, FString& OutMessage);

	// Core import logic — accepts already-extracted files so callers can inject project-level data
	static bool ImportDialogueGraphFromFiles(TMap<FString, FString>& ExtractedFiles, const FString& SourceFilePath, UObject* InParent, FName Name, EObjectFlags Flags, UMounteaDialogueGraph*& OutGraph, FString& OutMessage);

	// Main export functions
	static bool ExportDialogueGraph(const UMounteaDialogueGraph* Graph, const FString& FilePath);
	static bool ExportProject(const TArray<UMounteaDialogueGraph*>& Graphs, const FString& FilePath);

	// Helper functions for import process
	static bool IsZipFile(const TArray<uint8>& FileData);
	static bool ExtractFilesFromZip(const TArray<uint8>& ZipData, TMap<FString, FString>& OutExtractedFiles);
	static bool ValidateExtractedContent(const TMap<FString, FString>& ExtractedFiles);
	static bool PopulateGraphFromExtractedFiles(UMounteaDialogueGraph* Graph, const TMap<FString, FString>& ExtractedFiles, const FString& SourceFilePath);

	// Pre-import analysis: scans all nested .mnteadlg entries in an already-extracted project
	// archive and returns every existing graph that would be overwritten by the import.
	// Temp files created during the inner extraction are cleaned up before returning.
	// Call this before starting the import loop to check for open editors.
	static TArray<UMounteaDialogueGraph*> FindGraphsToBeReimported(const TMap<FString, FString>& ExtractedProjectFiles);

	// Imports PNG files from the Thumbnails/ folder of the archive into UTexture2D assets.
	// DestPackagePath is the UE content path under which a "thumbnails" subfolder is created.
	// Called both from PopulateGraphFromExtractedFiles (per-dialogue) and from the project factory (project-level).
	static void CreateTextureAssets(
		const TMap<FString, FString>& ExtractedFiles,
		const FString& DestPackagePath);

private:

	// ── Phase 0: Decorator / Condition Blueprint class creation ──────────────────

	// Parses decorators.json definitions (each entry has id, name, type, properties[]).
	// For each unique definition GUID, finds or creates a Blueprint subclass of
	// UMounteaDialogueDecoratorBase under <ProjectPackagePath>/Decorators/.
	// Returns a definition-GUID → generated UClass* map for use during node population.
	static TMap<FGuid, UClass*> CreateDecoratorBlueprints(
		const FString& Json,
		const FString& ProjectPackagePath);

	// Same for conditions.json → UMounteaDialogueConditionBase subclasses.
	// Assets land under <ProjectPackagePath>/Conditions/.
	static TMap<FGuid, UClass*> CreateConditionBlueprints(
		const FString& Json,
		const FString& ProjectPackagePath);

	// Finds an already-created Blueprint class by CDO GUID (pass 1: loaded classes,
	// pass 2: asset registry at PackagePath/Name). Returns nullptr if not found.
	static UClass* FindExistingDecoratorClass(const FGuid& DefinitionGUID, const FString& Name, const FString& PackagePath);
	static UClass* FindExistingConditionClass(const FGuid& DefinitionGUID, const FString& Name, const FString& PackagePath);

	// ── Phase 1: Pre-graph asset creation ───────────────────────────────────────

	// Parses participants.json, writes gameplay tags to the plugin INI, and returns
	// a name→fullTag map for use during DataTable population.
	static TMap<FString, FString> RegisterGameplayTags(
		const UMounteaDialogueGraph* Graph,
		const FString& ParticipantsJson);

	// Parses stringTable.json and builds the default-locale text lookup used by string tables.
	static void BuildStringTableLookup(
		const TMap<FString, FString>& ExtractedFiles,
		FString& OutDefaultLocale,
		TMap<FString, FString>& OutLookup,
		TSharedPtr<FJsonObject>& OutEntriesObject);

	// Builds the row-UUID → textKey map from dialogueRows.json.
	static TMap<FString, FString> BuildRowIdToTextKeyMap(const FString& DialogueRowsJson);

	// Creates (or loads existing) ST_*_DialogueRows and ST_*_Nodes string tables,
	// fills them with content from lookup maps, and saves them.
	static bool CreateGraphStringTables(
		UMounteaDialogueGraph* Graph,
		IAssetTools& AssetTools,
		const TMap<FString, FString>& ExtractedFiles,
		const TMap<FString, FString>& StringTableLookup,
		const TMap<FString, FString>& RowIdToTextKey,
		UStringTable*& OutDialogueRowsStringTable,
		UStringTable*& OutNodesStringTable);

	// Creates (or loads existing) DT_*_Participants and DT_*_DialogueRows data tables (empty).
	static bool CreateGraphDataTables(
		UMounteaDialogueGraph* Graph,
		IAssetTools& AssetTools,
		UDataTable*& OutParticipantsTable,
		UDataTable*& OutDialogueRowsTable);

	// Imports audio WAV files from the archive into USoundWave assets.
	// Returns a row-GUID → USoundWave map for use during DataTable row filling.
	static TMap<FGuid, USoundWave*> CreateAudioAssets(
		const TMap<FString, FString>& ExtractedFiles,
		UObject* InParent);

	// Imports or re-imports one audio file at TempPath into the UE content system.
	// Returns the imported USoundWave, or nullptr on failure.
	static USoundWave* ImportSingleAudioAsset(
		const FString& TempPath,
		const FString& FullPackagePath,
		const FString& AssetName,
		const FString& SearchPackagePath,
		IAudioEditorModule& AudioEditorModule,
		FAssetRegistryModule& AssetRegistryModule);

	// ── Phase 2: Graph population ────────────────────────────────────────────────

	static bool PopulateDialogueData(UMounteaDialogueGraph* Graph, const FString& SourceFilePath, const TMap<FString, FString>& ExtractedFiles);
	static bool PopulateCategories(UMounteaDialogueGraph* Graph, const FString& Json);

	// Creates all node objects and fills their properties. Builds OutSpawnedNodes for the
	// subsequent ReturnToNode resolution pass.
	static bool PopulateNodes(UMounteaDialogueGraph* Graph, const FString& Json,
		TMap<FGuid, UMounteaDialogueGraphNode*>& OutSpawnedNodes,
		const TMap<FGuid, UClass*>& DecoratorClasses);

	static bool PopulateEdges(UMounteaDialogueGraph* Graph, const FString& Json,
		const TMap<FGuid, UClass*>& ConditionClasses);

	// Resolves ReturnToNode.SelectedNode after all nodes have been created.
	static void ResolveReturnToNodeTargets(
		UMounteaDialogueGraph* Graph,
		const TMap<FGuid, UMounteaDialogueGraphNode*>& SpawnedNodesByGuid);

	// Searches import config history then asset registry for an existing graph with ImportedGuid.
	// Returns nullptr if not found.
	static UMounteaDialogueGraph* LookupExistingGraphByGuid(const FGuid& ImportedGuid);

	// Validates, populates, rebuilds and saves Graph from ExtractedFiles.
	// Returns false and sets OutMessage on any failure.
	static bool PopulateAndSaveGraph(
		UMounteaDialogueGraph* Graph,
		TMap<FString, FString>& ExtractedFiles,
		const FString& SourceFilePath,
		FString& OutMessage);

	// Parses "data.conditions" from EdgeObject and fills Edge->EdgeConditions.
	static void ParseEdgeConditions(
		UMounteaDialogueGraphEdge* Edge,
		const TSharedPtr<FJsonObject>& EdgeObject,
		const TMap<FGuid, UClass*>& ConditionClasses);

	// ── Phase 3: Fill data tables ─────────────────────────────────────────────────

	// Empties and repopulates DT_*_Participants from participants.json.
	static bool FillParticipantsDataTable(
		const UMounteaDialogueGraph* Graph,
		UDataTable* ParticipantsTable,
		const FString& ParticipantsJson,
		const TMap<FString, FString>& ParticipantTagMap);

	// Empties and repopulates DT_*_DialogueRows; links dialogue nodes to their rows;
	// sets RowSound references from the audio map.
	static bool FillDialogueRowsDataTable(
		UMounteaDialogueGraph* Graph,
		UDataTable* DialogueRowsTable,
		UDataTable* ParticipantsTable,
		UStringTable* DialogueRowsStringTable,
		UStringTable* NodesStringTable,
		const TMap<FString, FString>& ExtractedFiles,
		const TMap<FString, FString>& RowIdToTextKey,
		const TMap<FGuid, USoundWave*>& AudioMap);

	// Builds a nodeId → participantName map from the parsed nodes JSON array.
	static TMap<FString, FString> BuildNodeParticipantMap(
		const TArray<TSharedPtr<FJsonValue>>& NodesArray);

	// Creates FDialogueRow entries for one node's rows, links matching graph nodes, and adds them to DialogueRowsTable.
	static void ProcessDialogueRowGroup(
		UMounteaDialogueGraph* Graph,
		const FString& NodeId,
		const FString& ParticipantName,
		const TArray<TSharedPtr<FJsonObject>>& Rows,
		UDataTable* ParticipantsTable,
		UDataTable* DialogueRowsTable,
		UStringTable* DialogueRowsStringTable,
		UStringTable* NodesStringTable,
		const TMap<FString, FString>& RowIdToTextKey,
		const TMap<FGuid, USoundWave*>& AudioMap);

	// ── Utilities ─────────────────────────────────────────────────────────────────

	static FString BytesToString(const uint8* Bytes, int32 Count);
	static void PopulateNodeData(UMounteaDialogueGraphNode* Node, const TSharedPtr<FJsonObject>& JsonObject, const TMap<FGuid, UClass*>& DecoratorClasses);

	// Parses the "decorators" array from DataObject and appends resolved instances to Node->NodeDecorators.
	static void PopulateNodeDecorators(
		UMounteaDialogueGraphNode* Node,
		const TSharedPtr<FJsonObject>& DataObject,
		const TMap<FGuid, UClass*>& DecoratorClasses);

	// Deserialises JsonStr as a JSON array and appends entries (deduplicated by KeyField) to Target.
	static void MergeJsonArrayInto(
		const FString& JsonStr,
		TArray<TSharedPtr<FJsonValue>>& Target,
		TSet<FString>& SeenKeys,
		const FString& KeyField);

	// Serialises a JSON array to a compact string.
	static FString SerializeJsonArray(const TArray<TSharedPtr<FJsonValue>>& Array);
	static UStringTable* CreateStringTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName, TFunction<void(UStringTable*)> PopulateFunction);

	template <typename RowType>
	static UDataTable* CreateDataTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName);
	static void SaveAsset(UObject* Asset);

	// Helper functions for export process
	static bool GatherAssetsFromGraph(const UMounteaDialogueGraph* Graph, TMap<FString, FString>& OutJsonFiles, TArray<FString>& OutAudioFiles);
	static bool ExportAudioFiles(const TArray<FString>& AudioFiles, const FString& ExportPath, TArray<FString>& OutExportedFiles);
	static bool GatherThumbnailFiles(const UMounteaDialogueGraph* Graph, TMap<FString, FString>& OutThumbnailFiles);
	static bool PackToMNTEADLG(const TMap<FString, FString>& JsonFiles, const TArray<FString>& ExportedAudioFiles, const TMap<FString, FString>& ThumbnailFiles, const FString& OutputPath);
	static bool PackToMNTEADLGPROJ(const TMap<FString, FString>& ProjectJsonFiles, const TMap<FString, FString>& DialogueArchives, const TMap<FString, FString>& ThumbnailFiles, const FString& OutputPath);

	// Helper functions for gathering specific parts of the graph
	static void GatherNodesFromGraph(const UMounteaDialogueGraph* Graph, TArray<FDialogueNodeData>& OutNodeData);
	static bool GatherAudioFiles(const UMounteaDialogueGraph* Graph, TArray<FString>& OutAudioFiles);

	// Class-resolution helpers for import — search by class name, display name, and GUID string
	static UClass* FindConditionClassByName(const FString& NameOrGUID);
	static UClass* FindDecoratorClassByName(const FString& NameOrGUID);

	// Export helper for localized string table (v2)
	static FString CreateStringTableJson(const UMounteaDialogueGraph* Graph);

	// Writes a .po file per non-default locale so UE's localization pipeline can produce .locres files.
	// StringTableID  — asset name of the UStringTable (e.g. "ST_Foo_DialogueRows")
	// DefaultLocale  — locale whose text was written into the UStringTable (e.g. "en")
	// DefaultTexts   — textKey → default-locale text (already-built stringTableLookup map)
	// EntriesObject  — the "entries" object from stringTable.json
	static void ExportLocalizationPoFiles(
		const FString& StringTableID,
		const FString& DefaultLocale,
		const TMap<FString, FString>& DefaultTexts,
		const TSharedPtr<FJsonObject>& EntriesObject);

	// Helper function to get relative audio paths
	static FString GetRelativeAudioPath(const USoundBase* Sound, const FString& GraphFolder);
	static void CreateWAVFile(const TArray<uint8>& InPCMData, uint32 InSampleRate, uint16 InNumChannels, TArray<uint8>& OutWAVData);

	// Helper functions to generate JSON files
	static FString CreateNodesJson(const TArray<FDialogueNodeData>& NodeData);
	static void AddNodePosition(const TSharedPtr<FJsonObject>& NodeObject, const UMounteaDialogueGraphNode* Node);
	static void AddNodeData(const TSharedPtr<FJsonObject>& NodeObject, const UMounteaDialogueGraphNode* Node);
	static void AddDialogueNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode);
	static void AddJumpNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_ReturnToNode* Node);
	static void AddOpenChildGraphNodeData(const TSharedPtr<FJsonObject>& AdditionalInfoObject, const UMounteaDialogueGraphNode_OpenChildGraph* Node);
	static FString CreateEdgesJson(const UMounteaDialogueGraph* Graph);
	static FString CreateCategoriesJson(const UMounteaDialogueGraph* Graph);
	static FString CreateDialogueDataJson(const UMounteaDialogueGraph* Graph);
	static FString CreateParticipantsJson(const UMounteaDialogueGraph* Graph);
	static FString CreateDialogueRowsJson(const TArray<FDialogueNodeData>& AllNodeData, const UMounteaDialogueGraph* Graph);
	static FString CreateDecoratorsJson(const UMounteaDialogueGraph* Graph);
	static FString CreateConditionsJson(const UMounteaDialogueGraph* Graph);
};
