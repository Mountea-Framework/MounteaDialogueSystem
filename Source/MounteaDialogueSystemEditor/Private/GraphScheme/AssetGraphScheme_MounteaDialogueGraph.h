// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetGraphScheme_MounteaDialogueGraph.generated.h"

class UEdNode_MounteaDialogueGraphNode;
class UEdNode_MounteaDialogueGraphEdge;

#pragma region HelperStructs

/** Action to add a node to the graph */
USTRUCT()
struct FAssetSchemaAction_MounteaDialogueGraph_NewNode : public FEdGraphSchemaAction
{
	GENERATED_BODY();

public:
	FAssetSchemaAction_MounteaDialogueGraph_NewNode(): NodeTemplate(nullptr) {}

	FAssetSchemaAction_MounteaDialogueGraph_NewNode(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping), NodeTemplate(nullptr) {}

	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	UEdNode_MounteaDialogueGraphNode* NodeTemplate;
};

USTRUCT()
struct FAssetSchemaAction_MounteaDialogueGraph_NewEdge : public FEdGraphSchemaAction
{
	GENERATED_BODY();

public:
	FAssetSchemaAction_MounteaDialogueGraph_NewEdge(): NodeTemplate(nullptr){}

	FAssetSchemaAction_MounteaDialogueGraph_NewEdge(const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping), NodeTemplate(nullptr) {}

	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	UEdNode_MounteaDialogueGraphEdge* NodeTemplate;
};

#pragma endregion 

UCLASS(MinimalAPI)
class UAssetGraphScheme_MounteaDialogueGraph : public UEdGraphSchema
{

	GENERATED_BODY()
	
public:
	
	void GetBreakLinkToSubMenuActions(class UToolMenu* Menu, class UEdGraphPin* InGraphPin);

	virtual EGraphType GetGraphType(const UEdGraph* TestEdGraph) const override;

	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

	virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;

	virtual bool CreateAutomaticConversionNodeAndConnections(UEdGraphPin* A, UEdGraphPin* B) const override;

	virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;

	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;

	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;

	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const override;

	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;

	virtual UEdGraphPin* DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const override;

	virtual bool SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const override;

	virtual bool IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const override;

	virtual int32 GetCurrentVisualizationCacheID() const override;

	virtual void ForceVisualizationCacheClear() const override;

	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;

private:
	
	static int32 CurrentCacheRefreshID;
};
