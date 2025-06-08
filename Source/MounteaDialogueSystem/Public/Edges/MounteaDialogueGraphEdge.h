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
	
	/**
	 * The dialogue graph this edge belongs to.
	 * This represents the graph that the edge connects nodes within.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraph> Graph = nullptr;

	/**
	 * The starting node for this edge.
	 * This node represents where the dialogue transition begins.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraphNode> StartNode = nullptr;

	/**
	 * The ending node for this edge.
	 * This node represents where the dialogue transition ends.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraphNode> EndNode = nullptr;


#pragma endregion

#pragma region Functions
public:

	/**
	 * Returns the dialogue graph this edge is part of.
	 * 
	 * @return The dialogue graph that the edge belongs to.
	 */
	UFUNCTION(BlueprintPure, Category = "Mountea|Dialogue|Edge", meta=(CustomTag="MounteaK2Getter"))
	UMounteaDialogueGraph* GetGraph() const;

#pragma endregion 
};
