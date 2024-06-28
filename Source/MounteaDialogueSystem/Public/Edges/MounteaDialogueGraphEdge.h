// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphEdge.generated.h"

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;

/**
 * Mountea Dialogue Edges.
 *
 * Currently those Edges are just connecting objects with no advanced logic.
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), NotBlueprintType)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphEdge : public UObject
{
	GENERATED_BODY()

	UMounteaDialogueGraphEdge();

#pragma region Variables
public:
	
	UPROPERTY(VisibleAnywhere, Category = "Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraph> Graph = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraphNode> StartNode = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraphNode> EndNode = nullptr;

#pragma endregion

#pragma region Functions
public:
	
	//UFUNCTION(BlueprintPure, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* GetGraph() const;

#pragma endregion 
};
