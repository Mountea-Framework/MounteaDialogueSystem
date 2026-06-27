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
#include "MounteaDialogueContextStatics.generated.h"

class UMounteaDialogueContext;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;

UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueContextStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Validate"))
	static bool IsContextValid(const UMounteaDialogueContext* Context);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraphNode* FindNodeByGUID(const UMounteaDialogueGraph* FromGraph, FGuid ByGUID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static TArray<UMounteaDialogueGraphNode*> FindNodesByGUID(const UMounteaDialogueGraph* FromGraph, const TArray<FGuid>& Guids);
};
