// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
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
#include "MounteaMonologueStatics.generated.h"

class UMounteaDialogueGraph;
/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaMonologueStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	/**
	 * Validates whether given Graph is Monologue or not.
	 * If invalid graph is provided or settings are not setup, false is returned
	 * @param Graph Graph to be validated
	 * @return True if graph has `Monologue` tag from Config
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Monologue|Helpers", 
		meta=(CustomTag="MounteaK2Validate"))
	static bool IsGraphMonologue(UMounteaDialogueGraph* Graph);
};
