// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Core/MounteaDialogueConditionContextInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueConditionsStatics.generated.h"

class UMounteaDialogueConditionBase;
class UMounteaDialogueGraphEdge;

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueConditionsStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	/**
	 * Evaluates whether this condition is satisfied.
	 *
	 * @param Condition Condition to evaluate.
	 * @param Context  Condition context exposing traversal history, active participant, and session GUID.
	 * @return True if the condition passes; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mountea|Inventory & Equipment|Attachments|Helpers",
		meta=(CustomTag="MounteaK2Validate"),
		DisplayName="Evaluate Condition")
	static bool EvaluateCondition(UMounteaDialogueConditionBase* Condition, const TScriptInterface<IMounteaDialogueConditionContextInterface>& Context);
	
	/**
	 * Returns the human-readable name of this condition.
	 * Defaults to the class name. Override for a friendlier display string.
	 * 
	 * @param Condition Conditio to get name from.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mountea|Inventory & Equipment|Attachments|Helpers",
		meta=(CustomTag="MounteaK2Getter"),
		DisplayName="Get Condition Name")
	static FString GetConditionName(UMounteaDialogueConditionBase* Condition);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mountea|Inventory & Equipment|Attachments|Helpers",
		meta=(CustomTag="MounteaK2Getter"),
		DisplayName="Get Condition Documentation Link")
	static FString GetConditionDocumentationLink(UMounteaDialogueConditionBase* Condition);

	/**
	 * Evaluates all conditions on a dialogue graph edge.
	 * Returns true if the edge is traversable: no conditions, all pass (All mode), or any passes (Any mode).
	 *
	 * @param Edge     The edge whose conditions are evaluated. A null edge is treated as unconditional (returns true).
	 * @param Context  Condition context exposing traversal history, active participant, and session GUID.
	 * @return True if the edge may be traversed; false if blocked by conditions.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Validate"),
		DisplayName="Evaluate Edge Conditions")
	static bool EvaluateEdgeConditions(
		const UMounteaDialogueGraphEdge* Edge,
		const TScriptInterface<IMounteaDialogueConditionContextInterface>& Context);
};
