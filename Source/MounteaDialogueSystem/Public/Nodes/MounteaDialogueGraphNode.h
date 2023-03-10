// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
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
UCLASS(Abstract, BlueprintType, ClassGroup=("Mountea|Dialogue"), HideCategories=("Hidden", "Private", "Base"), AutoExpandCategories=("Mountea", "Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode : public UObject
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueGraphNode();

#pragma region Variables

#pragma region ReadOnly
public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Private")
	TArray<UMounteaDialogueGraphNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Private")
	TArray<UMounteaDialogueGraphNode*> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Private")
	TMap<UMounteaDialogueGraphNode*, UMounteaDialogueGraphEdge*> Edges;

	UPROPERTY(BlueprintReadOnly, Category = "Private")
	UMounteaDialogueGraph* Graph;

	UPROPERTY(BlueprintReadOnly, Category = "Private")
	int32 NodeIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Private")
	int32 MaxChildrenNodes = -1;

protected:

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Mountea", AdvancedDisplay)
	FGuid NodeGUID;

#pragma endregion

#pragma region Editable
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	uint8 bInheritGraphDecorators : 1;

	/**
	 * A list of Decorators that can help out with enhancing the Dialogue flow.
	 * Those Decorators are instanced and exist only as "triggers".
	 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mountea|Dialogue", NoClear, meta=(NoResetToDefault))
	TArray<FMounteaDialogueDecorator> NodeDecorators;

#pragma endregion

#pragma endregion 

#pragma region Functions

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	TArray<FMounteaDialogueDecorator> GetNodeDecorators() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool CanStartNode() const;
	virtual bool EvaluateDecorators() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	bool DoesInheritDecorators() const
	{ return bInheritGraphDecorators; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	int32 GetMaxChildNodes() const
	{ return MaxChildrenNodes; };


	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE int32 GetNodeIndex() const
	{ return NodeIndex; };
	void SetNodeIndex(const int32 NewIndex);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE FGuid GetNodeGUID() const
	{ return NodeGUID; };

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* GetGraph() const;

	

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE TArray<UMounteaDialogueGraphNode*> GetChildrenNodes() const
	{ return ChildrenNodes; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE TArray<UMounteaDialogueGraphNode*> GetParentNodes() const
	{return ParentNodes; };

	
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeTitle() const;
	virtual FText GetNodeTitle_Implementation() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetDescription() const;
	virtual FText GetDescription_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeCategory() const;
	virtual FText GetNodeCategory_Implementation() const;

	
	virtual void OnCreatedInEditor() {};

#pragma endregion 


#if WITH_EDITORONLY_DATA

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

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	FText NodeTooltipText;

	FText InternalName;

#endif

#if WITH_EDITOR

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeTooltipText() const;
	virtual FText GetNodeTooltipText_Implementation() const;
	
	virtual FLinearColor GetBackgroundColor() const;
	FText GetInternalName() const
	{ return InternalName; };
	
	virtual void SetNodeTitle(const FText& NewTitle);
	
	virtual bool CanCreateConnection(UMounteaDialogueGraphNode* Other, enum EEdGraphPinDirection Direction, FText& ErrorMessage);

	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat);

	virtual void OnPasted();

	FText GetDefaultTooltipBody() const;
#endif

};
