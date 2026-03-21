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
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueConditionsStatics.generated.h"

class UMounteaDialogueConditionBase;

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
	 * Override in subclasses to implement custom logic.
	 *
	 * @param Condition Condition to evaluate.
	 * @param Context  Arbitrary context object (e.g. dialogue manager or participant).
	 * @return True if the condition passes; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mountea|Inventory & Equipment|Attachments|Helpers",
		meta=(CustomTag="MounteaK2Validate"),
		DisplayName="Evaluate Condition")
	static bool EvaluateCondition(UMounteaDialogueConditionBase* Condition, UObject* Context);
	
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
};
