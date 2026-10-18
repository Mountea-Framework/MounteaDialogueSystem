// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "Helpers/MounteaDialogueEditorDetailsTypes.h"
#include "Layout/Layered/MounteaDialogueLayoutTypes.h"
#include "EdGraph_MounteaDialogueGraph.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphEdge;
class UEdNode_MounteaDialogueGraphNode;
class UEdNode_MounteaDialogueGraphEdge;
class FAssetEditor_MounteaDialogueGraph;

/** Maps layout snapshot ids back to editor objects. Game thread only. */
struct FDialogueLayoutBinding
{
	TArray<TWeakObjectPtr<UEdGraphNode>> Nodes; // UEdNode_MounteaDialogueGraphNode, parallel to snapshot nodes
	TArray<TWeakObjectPtr<UEdGraphNode>> Edges; // UEdNode_MounteaDialogueGraphEdge, parallel to snapshot edges
};

/** A wire route calculated by the layered layout, valid while the nodes it was calculated for stay where they are. */
struct FPreparedDialogueRoute
{
	TArray<FVector2D> Points; // graph space
	FIntPoint SourcePosition = FIntPoint::ZeroValue;
	FIntPoint TargetPosition = FIntPoint::ZeroValue;
	FVector2D SourceSize = FVector2D::ZeroVector;
	FVector2D TargetSize = FVector2D::ZeroVector;
	bool bUsable = false;
};

UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UEdGraph_MounteaDialogueGraph : public UEdGraph
{
	GENERATED_BODY()

public:

	UEdGraph_MounteaDialogueGraph();
	virtual ~UEdGraph_MounteaDialogueGraph() override;

	virtual void RebuildMounteaDialogueGraph();
	bool NormalizeEdgeNodes(bool BMigrateDirectLinks, int32& OutMigratedLinks, int32& OutRemovedDuplicateEdges);
	UEdNode_MounteaDialogueGraphNode* CreateEdNode(UMounteaDialogueGraphNode* DialogueNode);
	UEdNode_MounteaDialogueGraphEdge* CreateEdgeNode(UEdNode_MounteaDialogueGraphNode* StartNode, UEdNode_MounteaDialogueGraphNode* EndNode);

	UMounteaDialogueGraph* GetMounteaDialogueGraph() const;

	virtual bool Modify(bool bAlwaysMarkDirty) override;
	virtual void PostEditUndo() override;
	virtual void NotifyGraphChanged(const FEdGraphEditAction& Action) override;

	// Delta operations — update only what changed, no full rebuild
	void RegisterNode(UEdNode_MounteaDialogueGraphNode* EdNode);
	void UnregisterNode(UMounteaDialogueGraphNode* Node);
	void RegisterEdge(UEdNode_MounteaDialogueGraphEdge* EdEdge);
	void UnregisterEdge(UMounteaDialogueGraphEdge* Edge);

	// Bulk sync for undo/redo — clear and rebuild without NormalizeEdgeNodes
	void SyncTopology();

	TWeakPtr<FAssetEditor_MounteaDialogueGraph> GetDialogueEditorPtr() const
	{ return DialogueEditorPtr; };
	void SetDialogueEditorPtr(TWeakPtr<FAssetEditor_MounteaDialogueGraph> NewPtr);
	void ResetDialogueEditorPtr()
	{ DialogueEditorPtr.Reset(); };
	
	bool JumpToNode(const UMounteaDialogueGraphNode* Node);

public:

	UPROPERTY(Transient)
	TMap<UMounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*> NodeMap;

	UPROPERTY(Transient)
	TMap<UMounteaDialogueGraphEdge*, UEdNode_MounteaDialogueGraphEdge*> EdgeMap;

public:

	/** Captures a value-only layout input (game thread). Does not mutate the graph. */
	bool BuildLayoutSnapshot(FDialogueLayoutSnapshot& OutSnapshot, FDialogueLayoutBinding& OutBinding, FString& OutError) const;

	/**
	 * Writes a validated checkpoint: node and control positions (editor and runtime NodePosition) and prepared routes.
	 * Fails without touching anything when the graph no longer matches the snapshot. The caller owns the transaction.
	 */
	bool ApplyLayoutCheckpoint(const FDialogueLayoutSnapshot& Expected, const FDialogueLayoutBinding& Binding, const FDialogueLayoutCheckpoint& Checkpoint, FString& OutError);

	/** Route prepared for this connection, or nullptr when none exists or the diagram changed since it was calculated. */
	const FPreparedDialogueRoute* FindPreparedRoute(const UEdNode_MounteaDialogueGraphEdge* Edge) const;
	void ClearPreparedRoutes() { PreparedRoutes.Reset(); }
	bool HasPreparedRoutes() const { return !PreparedRoutes.IsEmpty(); }

	/** Positive cached size, then desired size, then the documented fallback. */
	static FVector2D MeasureNode(const UEdNode_MounteaDialogueGraphNode* Node);

	void UpdateFocusedInstance(const FPIEInstanceData& InstanceId);
	/** Explicit, position-derived option ordering. Call only for intentional reordering (manual node movement). */
	bool ReorderFromPositions();
	/** Initialises missing orders, keeping every existing one. Safe on any topology change. */
	void EnsureExecutionOrders();

protected:

	void Clear();
	void SortNodes(UMounteaDialogueGraphNode* RootNode);	
	
	static UMounteaDialogueGraphNode* GetParentNode(const UMounteaDialogueGraphNode& Node);
	
private:

	void RefreshPreparedRoutes() const;
	uint32 ComputeNodeSignature() const;

	mutable TMap<TWeakObjectPtr<const UEdGraphNode>, FPreparedDialogueRoute> PreparedRoutes;
	mutable uint32 PreparedSignature = 0;
	mutable uint64 PreparedFrame = TNumericLimits<uint64>::Max();

	UPROPERTY()
	TArray<UMounteaDialogueGraphNode*> CachedGraphData;

	/** Pointer back to the Dialogue editor that owns us */
	TWeakPtr<FAssetEditor_MounteaDialogueGraph> DialogueEditorPtr;

	/** Suppresses delta-sync callbacks during full RebuildMounteaDialogueGraph / SyncTopology */
	bool bSuppressDeltaSync = false;

public:
	
	FPIEInstanceData FocusedInstance;
};
