// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode.generated.h"

class UMounteaDialogueGraph;
class UMounteaDialogueGraphEdge;

/**
 * 
 */
UCLASS(Abstract, ClassGroup=("Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode : public UObject
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueGraphNode();

#pragma region Variables

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TMap<UMounteaDialogueGraphNode*, UMounteaDialogueGraphEdge*> Edges;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* Graph;

#pragma endregion 

#pragma region Functions
	
	UFUNCTION(BlueprintCallable, Category = "GenericGraphNode")
	virtual UMounteaDialogueGraphEdge* GetEdge(UMounteaDialogueGraphNode* ChildNode);

	/**
	 * Returns true if there are no connected Nodes to this one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	bool IsLeafNode() const;

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* GetGraph() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MissionNode")
	FText GetDescription() const;
	virtual FText GetDescription_Implementation() const;

#pragma endregion 

#if WITH_EDITORONLY_DATA

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowInputNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowOutputNodes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Mountea|Dialogue|Editor")
	FText NodeTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	TSubclassOf<UMounteaDialogueGraph> CompatibleGraphType;

	UPROPERTY(EditDefaultsOnly, Category = "Mountea|Dialogue|Editor")
	FLinearColor BackgroundColor;

	UPROPERTY(EditDefaultsOnly, Category = "Mountea|Dialogue|Editor")
	FText ContextMenuName;
#endif

#if WITH_EDITOR
	virtual FLinearColor GetBackgroundColor() const;

	virtual FText GetNodeTitle() const;

	virtual void SetNodeTitle(const FText& NewTitle);
	
	virtual bool CanCreateConnection(UMounteaDialogueGraphNode* Other, enum EEdGraphPinDirection Direction, FText& ErrorMessage);
#endif

};
