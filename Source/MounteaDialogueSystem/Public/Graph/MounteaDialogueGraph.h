// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameplayTagContainer.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "MounteaDialogueGraph.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraphEdge;

struct FMounteaDialogueDecorator;


/**
 * Mountea Dialogue Graph.
 * 
 * Can be manually created from Content Browser, using Mountea Dialogue category.
 * Comes with Node editor, which provides easy to follow visual way to create Dialogue Trees.
 */
UCLASS(BlueprintType, ClassGroup=("Mountea|Dialogue"), DisplayName="Mountea Dialogue Tree", HideCategories=("Hidden", "Private", "Base"), AutoExpandCategories=("Mountea", "Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraph : public UObject
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraph();

#pragma region Variables
	
protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mountea|Dialogue", NoClear, meta=(NoResetToDefault))
	TArray<FMounteaDialogueDecorator> GraphDecorators;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mountea|Dialogue")
	FGameplayTagContainer GraphTags;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea")
	FGuid GraphGUID;

public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	class UMounteaDialogueGraphNode* StartNode = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TSubclassOf<UMounteaDialogueGraphNode> NodeType;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TSubclassOf<UMounteaDialogueGraphEdge> EdgeType;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> RootNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> AllNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	bool bEdgeEnabled;

#pragma endregion

#pragma region Functions

public:

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	FGuid GetGraphGUID() const;
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> GetAllNodes() const;
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> GetRootNodes() const;
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	UMounteaDialogueGraphNode* GetStartNode() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	TArray<FMounteaDialogueDecorator> GetGraphDecorators() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	TArray<FMounteaDialogueDecorator> GetAllDecorators() const;
	
	bool CanStartDialogueGraph() const;
	
public:
	
	void CreateGraph();
	void ClearGraph();

	FORCEINLINE bool IsEdgeEnabled() const
	{ return bEdgeEnabled; };

	virtual void PostInitProperties() override;

#pragma endregion 

#if WITH_EDITORONLY_DATA

public:
	UPROPERTY()
	class UEdGraph* EdGraph;

	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue|Editor")
	bool bCanRenameNode;

#endif

#if WITH_EDITOR

	virtual bool ValidateGraph(TArray<FText>& ValidationErrors, bool RichTextFormat);
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

#endif
	
public:

	// Construct and initialize a node within this Dialogue.
	template<class T>
	T* ConstructDialogueNode(TSubclassOf<UMounteaDialogueGraphNode> DialogueNodeClass = T::StaticClass())
	{
		// Set flag to be transactional so it registers with undo system
		T* DialogueNode = NewObject<T>(this, DialogueNodeClass, NAME_None, RF_Transactional);
		DialogueNode->OnCreatedInEditor();
		return DialogueNode;
	}
};
