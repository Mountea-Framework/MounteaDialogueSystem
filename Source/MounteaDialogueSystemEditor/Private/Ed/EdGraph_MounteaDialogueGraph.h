// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
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
	UEdNode_MounteaDialogueGraphNode* CreateEdNode(UMounteaDialogueGraphNode* DialogueNode);
	UEdNode_MounteaDialogueGraphEdge* CreateEdgeNode(UEdNode_MounteaDialogueGraphNode* StartNode, UEdNode_MounteaDialogueGraphNode* EndNode);

	UMounteaDialogueGraph* GetMounteaDialogueGraph() const;

	virtual bool Modify(bool bAlwaysMarkDirty) override;
	virtual void PostEditUndo() override;

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

	void UpdateFocusedInstance(const int32 InstanceId);
	void AssignExecutionOrder();

protected:

	void Clear();
	void SortNodes(UMounteaDialogueGraphNode* RootNode);
	
	void ResetExecutionOrders() const;
	static UMounteaDialogueGraphNode* GetParentNode(const UMounteaDialogueGraphNode& Node);
	
	static void AssignNodeToLayer(UMounteaDialogueGraphNode* Node, int32 LayerIndex, TMap<int32, TArray<UMounteaDialogueGraphNode*>>& LayeredNodes);

private:

	TArray<UMounteaDialogueGraphNode*> CachedGraphData;

	/** Pointer back to the Dialogue editor that owns us */
	TWeakPtr<FAssetEditor_MounteaDialogueGraph> DialogueEditorPtr;
};
