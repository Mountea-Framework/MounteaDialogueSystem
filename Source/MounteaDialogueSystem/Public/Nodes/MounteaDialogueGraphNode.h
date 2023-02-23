// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode.generated.h"

class UMounteaDialogueGraph;
class UMounteaDialogueGraphEdge;

/**
 * Mountea Dialogue Graph Node abstract Base class.
 * 
 * Provides generic functionality to be enhanced or overriden by Child Classes.
 * Does come with ability to define Colours, Name, Description and Title.
 * Contains information about Parent and Children Nodes.
 */
UCLASS(Abstract, BlueprintType, ClassGroup=("Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode : public UObject
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueGraphNode();

#pragma region Variables

public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TMap<UMounteaDialogueGraphNode*, UMounteaDialogueGraphEdge*> Edges;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* Graph;

protected:

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Mountea|Dialogue")
	FGuid NodeGUID;

#pragma endregion 

#pragma region Functions

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE FGuid GetNodeGUID() const
	{ return NodeGUID; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE TArray<UMounteaDialogueGraphNode*> GetChildrenNodes() const
	{ return ChildrenNodes; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE TArray<UMounteaDialogueGraphNode*> GetParentNodes() const
	{return ParentNodes; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool CanStartNode() const;
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue", meta=(DevelopmentOnly=true))
	virtual FText GetNodeTitle() const;
		
	/**
	 * Returns true if there are no connected Nodes to this one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	bool IsLeafNode() const;

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* GetGraph() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetDescription() const;
	virtual FText GetDescription_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeCategory() const;
	virtual FText GetNodeCategory_Implementation() const;

	virtual void OnCreatedInEditor() {};
	virtual UMounteaDialogueGraphEdge* GetEdge(UMounteaDialogueGraphNode* ChildNode);

#pragma endregion 


#if WITH_EDITORONLY_DATA

	bool bHasBenManuallyRenamed;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowInputNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowOutputNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowCopy;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowCut;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowPaste;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowDelete;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bAllowManualCreate;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	FText NodeTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	FText ContextMenuName;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	TSubclassOf<UMounteaDialogueGraph> CompatibleGraphType;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	FLinearColor BackgroundColor;

	FText InternalName;

#endif

#if WITH_EDITOR
	virtual FLinearColor GetBackgroundColor() const;
	FText GetInternalName() const
	{ return InternalName; };
	
	virtual void SetNodeTitle(const FText& NewTitle);
	
	virtual bool CanCreateConnection(UMounteaDialogueGraphNode* Other, enum EEdGraphPinDirection Direction, FText& ErrorMessage);

	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat);

	virtual void OnPasted();
#endif

};
