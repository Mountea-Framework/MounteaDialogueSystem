// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "Helpers/MounteaDialogueEditorDetailsTypes.h"
#include "EdGraph_MounteaDialogueGraph.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphEdge;
class UEdNode_MounteaDialogueGraphNode;
class UEdNode_MounteaDialogueGraphEdge;
class FAssetEditor_MounteaDialogueGraph;

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

	void UpdateFocusedInstance(const FPIEInstanceData& InstanceId);
	void AssignExecutionOrder();
	void ResetExecutionOrders() const;

protected:

	void Clear();
	void SortNodes(UMounteaDialogueGraphNode* RootNode);	
	
	static UMounteaDialogueGraphNode* GetParentNode(const UMounteaDialogueGraphNode& Node);
	
	static void AssignNodeToLayer(UMounteaDialogueGraphNode* Node, int32 LayerIndex, TMap<int32, TArray<UMounteaDialogueGraphNode*>>& LayeredNodes);

private:

	UPROPERTY()
	TArray<UMounteaDialogueGraphNode*> CachedGraphData;

	/** Pointer back to the Dialogue editor that owns us */
	TWeakPtr<FAssetEditor_MounteaDialogueGraph> DialogueEditorPtr;

	/** Suppresses delta-sync callbacks during full RebuildMounteaDialogueGraph / SyncTopology */
	bool bSuppressDeltaSync = false;

public:
	
	FPIEInstanceData FocusedInstance;
};
