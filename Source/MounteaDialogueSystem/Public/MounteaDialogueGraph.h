// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"

#include "MounteaDialogueGraph.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraphEdge;

/**
 * 
 */
UCLASS(Blueprintable, ClassGroup=("Mountea|Dialogue"), DisplayName="Mountea Dialogue Tree")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraph : public UObject
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraph();

#pragma region Variables
	
public: 
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TSubclassOf<UMounteaDialogueGraphNode> NodeType;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TSubclassOf<UMounteaDialogueGraphEdge> EdgeType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mountea|Dialogue")
	FGameplayTagContainer GraphTags;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> RootNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> AllNodes;

	UPROPERTY(BlueprintReadOnly, BlueprintReadOnly, Category = "Mountea|Dialogue")
	bool bEdgeEnabled;

#pragma endregion

#pragma region Functions

public:
	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	void Print(bool ToConsole = true, bool ToScreen = true);

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	int GetLevelNum() const;

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	void GetNodesByLevel(int Level, TArray<UMounteaDialogueGraphNode*>& Nodes);

	void ClearGraph();

#pragma endregion 

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UEdGraph* EdGraph;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bCanRenameNode;
#endif
};
