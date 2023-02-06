// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphLayoutStrategy.generated.h"

class UEdGraph_MounteaDialogueGraph;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class UEdNode_MounteaDialogueGraphNode;
class UMounteaDialogueGraphEditorSettings;

/**
 * 
 */
UCLASS(Abstract)
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueGraphLayoutStrategy : public UObject
{
	GENERATED_BODY()
	
public:
	UMounteaDialogueGraphLayoutStrategy();
	virtual ~UMounteaDialogueGraphLayoutStrategy() override;

	virtual void Layout(UEdGraph* G) {};

	UMounteaDialogueGraphEditorSettings* Settings;

protected:
	int32 GetNodeWidth(UEdNode_MounteaDialogueGraphNode* EdNode);

	int32 GetNodeHeight(UEdNode_MounteaDialogueGraphNode* EdNode);

	FBox2D GetNodeBound(UEdGraphNode* EdNode);

	FBox2D GetActualBounds(UMounteaDialogueGraphNode* RootNode);

	virtual void RandomLayoutOneTree(UMounteaDialogueGraphNode* RootNode, const FBox2D& Bound);

protected:
	UMounteaDialogueGraph* Graph;
	UEdGraph_MounteaDialogueGraph* EdGraph;
	int32 MaxIteration;
	int32 OptimalDistance;
};
