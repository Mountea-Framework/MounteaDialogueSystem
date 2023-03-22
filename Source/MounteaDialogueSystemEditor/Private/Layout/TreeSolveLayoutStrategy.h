// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphLayoutStrategy.h"
#include "TreeSolveLayoutStrategy.generated.h"

class UMounteaDialogueGraphNode;
//class UEdNode_MounteaDialogueGraphNode;

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UTreeSolveLayoutStrategy : public UMounteaDialogueGraphLayoutStrategy
{
	GENERATED_BODY()

	virtual void Layout(UEdGraph* InEdGraph) override;

protected:
	void InitPass(UMounteaDialogueGraphNode* RootNode, const FVector2D& Anchor);
	bool ResolveConflictPass(UMounteaDialogueGraphNode* Node);

	bool ResolveConflict(UMounteaDialogueGraphNode* LRoot, UMounteaDialogueGraphNode* RRoot);

	void GetLeftContour(UMounteaDialogueGraphNode* RootNode, int32 Level, TArray<UEdNode_MounteaDialogueGraphNode*>& Contour);
	void GetRightContour(UMounteaDialogueGraphNode* RootNode, int32 Level, TArray<UEdNode_MounteaDialogueGraphNode*>& Contour);
	
	void ShiftSubTree(UMounteaDialogueGraphNode* RootNode, const FVector2D& Offset);

	void UpdateParentNodePosition(UMounteaDialogueGraphNode* RootNode);
};
