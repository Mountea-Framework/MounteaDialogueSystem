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
class UStringTable;
class UDataTable;

namespace MounteaArchive
{
	struct FDialogueArchive;
	struct FDialogue;
	struct FDefinition;
	struct FNode;
	struct FRow;
}

/** Where and how one archive is imported. */
struct FMounteaArchiveImportRequest
{
	/** File the archive came from (stored on the graph for Reimport). */
	FString SourceFilePath;

	/** Long package path that receives Decorators, Conditions and thumbnails. */
	FString TargetFolder;

	/** Long package path that receives dialogue graphs created next to the root. Defaults to TargetFolder. */
	FString DialoguesFolder;

	/** True for .mnteadlgproj imports. */
	bool bProjectImport = false;

	/** Outer, name and flags used when the root dialogue asset has to be created (single-file import). */
	UObject* RootParent = nullptr;
	FName RootName = NAME_None;
	EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;

	/** Reimport target for the root dialogue; when null the graph is looked up by its dialogue id. */
	UMounteaDialogueGraph* ExistingRoot = nullptr;
};

/** Data shared by every dialogue of one archive import. */
struct FMounteaArchiveImportContext
{
	const MounteaArchive::FDialogueArchive* Archive = nullptr;
	FMounteaArchiveImportRequest Request;

	/** participant id -> full gameplay tag */
	TMap<FString, FString> ParticipantTags;
	TMap<FGuid, UClass*> DecoratorClasses;
	TMap<FGuid, UClass*> ConditionClasses;
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

	// ── Archive import (mountea.archive.v3, see Docs/ArchiveContract-v3.md) ─────────────────────

	/** Reads, unzips and validates a .mnteadlg / .mnteadlgproj. OutMessage lists the problems on failure. */
	static bool ReadArchiveFile(const FString& FilePath, MounteaArchive::FDialogueArchive& OutArchive, FString& OutMessage);

	/** True when the file's root dialogue was imported before (import config history). */
	static bool IsReimport(const FString& Filename);

	static bool ReimportDialogueGraph(const FString& FilePath, UObject* ObjectRedirector, UMounteaDialogueGraph*& OutGraph, FString& OutMessage);
	static bool CanReimport(UObject* ObjectRedirector, TArray<FString>& OutFilenames);

	/**
	 * Resolves pending OpenChildGraph targets after all graphs have been imported.
	 *
	 * During import, OpenChildGraph nodes store the target dialogue GUID in
	 * PendingTargetDialogueGUID instead of resolving immediately (because the target
	 * graph may not exist yet). Call this once all graphs are available.
	 *
	 * @param Graph       The graph whose OpenChildGraph nodes should be resolved.
	 * @param KnownGraphs Optional map of GUID -> graph for graphs imported in the same batch.
	 *                    Checked before falling back to a full asset-registry scan.
	 */
	static void ResolveOpenChildGraphTargets(UMounteaDialogueGraph* Graph, const TMap<FGuid, UMounteaDialogueGraph*>& KnownGraphs);

	/** Single .mnteadlg import; creates the root graph (and its child dialogues next to it). */
	static bool ImportDialogueGraph(const FString& FilePath, UObject* InParent, FName Name, EObjectFlags Flags, UMounteaDialogueGraph*& OutGraph, FString& OutMessage);

	/** Core: creates or reimports one graph per dialogue of the archive. */
	static bool ImportArchiveModel(const MounteaArchive::FDialogueArchive& Archive, const FMounteaArchiveImportRequest& Request,
		TMap<FGuid, UMounteaDialogueGraph*>& OutGraphs, UMounteaDialogueGraph*& OutRoot, FString& OutMessage);

	/** Existing graphs that importing Archive would overwrite. */
	static TArray<UMounteaDialogueGraph*> FindGraphsToBeReimported(const MounteaArchive::FDialogueArchive& Archive);

	// Searches import config history then asset registry for an existing graph with ImportedGuid.
	// Returns nullptr if not found.
	static UMounteaDialogueGraph* LookupExistingGraphByGuid(const FGuid& ImportedGuid);

	// Main export functions
	static bool ExportDialogueGraph(const UMounteaDialogueGraph* Graph, const FString& FilePath);
	static bool ExportProject(const TArray<UMounteaDialogueGraph*>& Graphs, const FString& FilePath);

private:

	// ── Definitions (decorator / condition Blueprint classes) ────────────────────

	// Finds or creates one Blueprint subclass per definition under PackagePath (Decorators/ or Conditions/).
	// Returns definition GUID -> generated class.
	static TMap<FGuid, UClass*> CreateDefinitionClasses(const TArray<MounteaArchive::FDefinition>& Definitions, const FString& PackagePath, bool bDecorators);

	// Finds an already-created Blueprint class by CDO GUID (pass 1: loaded classes,
	// pass 2: asset registry at PackagePath/Name). Returns nullptr if not found.
	static UClass* FindExistingDecoratorClass(const FGuid& DefinitionGUID, const FString& Name, const FString& PackagePath);
	static UClass* FindExistingConditionClass(const FGuid& DefinitionGUID, const FString& Name, const FString& PackagePath);

	// Class-resolution helpers for import: search by class name, display name, and GUID string
	static UClass* FindConditionClassByName(const FString& NameOrGUID);
	static UClass* FindDecoratorClassByName(const FString& NameOrGUID);

	// ── Participants and gameplay tags ───────────────────────────────────────────

	// participant id -> "Mountea_Dialogue.<category chain>.<participant name>"
	static TMap<FString, FString> BuildParticipantTags(const MounteaArchive::FDialogueArchive& Archive);

	// Appends missing tags to <Project>/Config/Tags/MounteaDialogueParticipants.ini and refreshes the tag tree.
	static void RegisterParticipantTags(const TArray<FString>& FullTags);

	// Imports participant thumbnails as <TargetFolder>/thumbnails/<ParticipantName>.
	static void ImportParticipantThumbnails(const MounteaArchive::FDialogueArchive& Archive, const FString& TargetFolder);

	// ── One dialogue ─────────────────────────────────────────────────────────────

	static bool PopulateGraphFromArchive(UMounteaDialogueGraph* Graph, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue);

	static bool CreateGraphStringTables(UMounteaDialogueGraph* Graph, IAssetTools& AssetTools, const FMounteaArchiveImportContext& Context,
		const MounteaArchive::FDialogue& Dialogue, UStringTable*& OutDialogueRowsStringTable, UStringTable*& OutNodesStringTable);

	// Creates (or loads existing) DT_*_Participants and DT_*_DialogueRows data tables (empty).
	static bool CreateGraphDataTables(UMounteaDialogueGraph* Graph, IAssetTools& AssetTools, UDataTable*& OutParticipantsTable, UDataTable*& OutDialogueRowsTable);

	static bool PopulateNodes(UMounteaDialogueGraph* Graph, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue,
		TMap<FGuid, UMounteaDialogueGraphNode*>& OutSpawnedNodes, const UStringTable* NodesStringTable);

	static void PopulateNodeData(UMounteaDialogueGraphNode* Node, const MounteaArchive::FNode& NodeRecord, const FMounteaArchiveImportContext& Context, const UStringTable* NodesStringTable);

	static bool PopulateEdges(UMounteaDialogueGraph* Graph, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue);

	// Resolves ReturnToNode.SelectedNode after all nodes have been created.
	static void ResolveReturnToNodeTargets(UMounteaDialogueGraph* Graph, const TMap<FGuid, UMounteaDialogueGraphNode*>& SpawnedNodesByGuid);

	static bool FillParticipantsDataTable(UMounteaDialogueGraph* Graph, UDataTable* ParticipantsTable, const FMounteaArchiveImportContext& Context, const MounteaArchive::FDialogue& Dialogue);

	static bool FillDialogueRowsDataTable(UMounteaDialogueGraph* Graph, UDataTable* DialogueRowsTable, UDataTable* ParticipantsTable,
		UStringTable* DialogueRowsStringTable, UStringTable* NodesStringTable, const FMounteaArchiveImportContext& Context,
		const MounteaArchive::FDialogue& Dialogue, const TMap<FGuid, USoundWave*>& AudioMap);

	static void ProcessDialogueRowGroup(UMounteaDialogueGraph* Graph, const MounteaArchive::FNode& NodeRecord, const FString& ParticipantName,
		UDataTable* ParticipantsTable, UDataTable* DialogueRowsTable, UStringTable* DialogueRowsStringTable, UStringTable* NodesStringTable,
		const TMap<FGuid, USoundWave*>& AudioMap);

	// Imports each row's audio bytes as a USoundWave under <GraphPackage>/audio/<RowGuid>/. Returns row GUID -> sound.
	static TMap<FGuid, USoundWave*> ImportRowAudio(const MounteaArchive::FDialogueArchive& Archive, const MounteaArchive::FDialogue& Dialogue, UObject* InParent);

	// Imports or re-imports one audio file at TempPath into the UE content system.
	static USoundWave* ImportSingleAudioAsset(
		const FString& TempPath,
		const FString& FullPackagePath,
		const FString& AssetName,
		const FString& SearchPackagePath,
		IAudioEditorModule& AudioEditorModule,
		FAssetRegistryModule& AssetRegistryModule);

	// ── Node titles (Nodes string table) ─────────────────────────────────────────
	// A node has two player-facing titles: its display name and its Selection Title (the text of the
	// choice button). Both live in the Nodes string table under the keys named by the archive.

	// Nodes table key a node's RowTitle (the choice button text) should read: the Selection Title key when
	// the table has text for it, otherwise the display name key. Empty when the node has neither.
	static FString GetRowTitleEntryKey(const UStringTable* NodesStringTable, const MounteaArchive::FNode& NodeRecord);
	static FText MakeRowTitle(const UStringTable* NodesStringTable, const MounteaArchive::FNode& NodeRecord);

	// Writes a .po file per non-default locale so UE's localization pipeline can produce .locres files.
	// The string table itself holds the default locale; this carries every other one.
	static void WriteLocalizationPoFiles(const FString& StringTableID, const FString& DefaultLocale, const TMap<FString, TMap<FString, FString>>& TextsByKeyAndLocale);

#if WITH_DEV_AUTOMATION_TESTS
	friend struct FMounteaDialogueImportTestAccess;
#endif

	// ── Utilities ─────────────────────────────────────────────────────────────────

	static UStringTable* CreateStringTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName, TFunction<void(UStringTable*)> PopulateFunction);

	template <typename RowType>
	static UDataTable* CreateDataTable(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName);
	static void SaveAsset(UObject* Asset);

	// ── Archive export ───────────────────────────────────────────────────────────

	// Walks the graphs (plus every dialogue they open) into the archive model. Collects every problem into OutErrors.
	static bool BuildArchiveFromGraphs(const TArray<const UMounteaDialogueGraph*>& Graphs, const UMounteaDialogueGraph* Root, bool bProject,
		const FString& ProjectName, MounteaArchive::FDialogueArchive& OutArchive, TArray<FString>& OutErrors);

	static bool SaveArchiveToFile(const MounteaArchive::FDialogueArchive& Archive, const FString& FilePath, FString& OutMessage);
};
