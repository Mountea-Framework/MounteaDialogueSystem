// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphEdge.generated.h"

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;

/**
 * 
 */
UCLASS(Blueprintable, ClassGroup=("Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphEdge : public UObject
{
	GENERATED_BODY()

	UMounteaDialogueGraphEdge();

#pragma region Variables
public:
	
	UPROPERTY(VisibleAnywhere, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* Graph = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	UMounteaDialogueGraphNode* StartNode = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	UMounteaDialogueGraphNode* EndNode = nullptr;

#pragma endregion

#pragma region Functions
public:
	
	UFUNCTION(BlueprintPure, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* GetGraph() const;

#pragma endregion 
};
