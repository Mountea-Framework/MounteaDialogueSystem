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

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UEdGraph_MounteaDialogueGraph : public UEdGraph
{
	GENERATED_BODY()

public:

	UEdGraph_MounteaDialogueGraph();
	virtual ~UEdGraph_MounteaDialogueGraph() override;

	virtual void RebuildMounteaDialogueGraph();

	UMounteaDialogueGraph* GetMounteaDialogueGraph() const;

	virtual bool Modify(bool bAlwaysMarkDirty) override;
	virtual void PostEditUndo() override;

public:

	UPROPERTY(Transient)
	TMap<UMounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*> NodeMap;

	UPROPERTY(Transient)
	TMap<UMounteaDialogueGraphEdge*, UEdNode_MounteaDialogueGraphEdge*> EdgeMap;

protected:

	void Clear();
	void SortNodes(UMounteaDialogueGraphNode* RootNode);
};
