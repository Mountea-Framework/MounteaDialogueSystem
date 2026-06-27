// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Conditions/MounteaDialogueConditionBase.h"
#include "MounteaDialogueGraphEdge.generated.h"

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;

/**
 * Mountea Dialogue Edges.
 *
 * Edges connect two nodes in the dialogue graph and carry an optional set of
 * runtime conditions that gate traversal.
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), BlueprintType)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphEdge : public UObject
{
	GENERATED_BODY()

	UMounteaDialogueGraphEdge();

public:
	
	/**
	 * The dialogue graph this edge belongs to.
	 * This represents the graph that the edge connects nodes within.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mountea|Dialogue",
		meta=(NoResetToDefault),
		AdvancedDisplay)
	TObjectPtr<UMounteaDialogueGraph> Graph = nullptr;

	/**
	 * The starting node for this edge.
	 * This node represents where the dialogue transition begins.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mountea|Dialogue",
		meta=(NoResetToDefault),
		AdvancedDisplay)
	TObjectPtr<UMounteaDialogueGraphNode> StartNode = nullptr;

	/**
	 * The ending node for this edge.
	 * This node represents where the dialogue transition ends.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mountea|Dialogue",
		meta=(NoResetToDefault),
		AdvancedDisplay)
	TObjectPtr<UMounteaDialogueGraphNode> EndNode = nullptr;

	/**
	 * Optional conditions that gate traversal of this edge.
	 * An empty Rules array means the edge is always traversable.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions",
		meta=(NoResetToDefault),
		meta=(ShowOnlyInnerProperties))
	FMounteaDialogueEdgeConditions EdgeConditions;

public:

	/**
	 * Returns the dialogue graph this edge is part of.
	 *
	 * @return The dialogue graph that the edge belongs to.
	 */
	UFUNCTION(BlueprintPure, Category = "Mountea|Dialogue|Edge",
		meta=(CustomTag="MounteaK2Getter"))
	UMounteaDialogueGraph* GetGraph() const;

	/**
	 * Returns the edge conditions (rules + evaluation mode).
	 *
	 * @return The full condition set for this edge.
	 */
	UFUNCTION(BlueprintPure, Category = "Mountea|Dialogue|Edge",
		meta=(CustomTag="MounteaK2Getter"))
	FMounteaDialogueEdgeConditions GetEdgeConditions() const;
};
