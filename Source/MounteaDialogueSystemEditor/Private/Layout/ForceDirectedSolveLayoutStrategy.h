// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphLayoutStrategy.h"
#include "UObject/Object.h"
#include "ForceDirectedSolveLayoutStrategy.generated.h"

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UForceDirectedSolveLayoutStrategy : public UMounteaDialogueGraphLayoutStrategy
{
	GENERATED_BODY()

	UForceDirectedSolveLayoutStrategy();
	
	virtual void Layout(UEdGraph* EdGraph) override;

protected:
	virtual FBox2D LayoutOneTree(UMounteaDialogueGraphNode* RootNode, const FBox2D& PreTreeBound);

protected:
	bool bRandomInit;
	float InitTemperature;
	float CoolDownRate;
};
