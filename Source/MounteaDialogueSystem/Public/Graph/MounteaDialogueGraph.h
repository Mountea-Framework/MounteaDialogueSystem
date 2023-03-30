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

	/**
	 * The list of decorators for the dialogue graph.
	 * Decorators are used to add extra functionality or behavior to the nodes in the graph.
	 * This array should contain an instance of each decorator used in the graph.
	 * The order of the decorators in this array determines the order in which they will be applied to the nodes.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mountea|Dialogue", NoClear, meta=(NoResetToDefault))
	TArray<FMounteaDialogueDecorator> GraphDecorators;
	/**
	 * A set of gameplay tags associated with this dialogue graph.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mountea|Dialogue")
	FGameplayTagContainer GraphTags;
	/**
	* GUID for this Mountea Dialogue Graph.
	*❗ Unique identifier for this Dialogue Graph instance.
	*❔ Can be used for debugging and tracing purposes.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "Mountea")
	FGuid GraphGUID;

public:
	
	// Pointer to the starting node of the dialogue graph.
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	class UMounteaDialogueGraphNode* StartNode = nullptr;
	/**
	 * The class of the dialogue node represented by this instance.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TSubclassOf<UMounteaDialogueGraphNode> NodeType;
	/**
	 * The class of the dialogue edge represented by this instance.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TSubclassOf<UMounteaDialogueGraphEdge> EdgeType;
	/**
	 * An array of root nodes in the dialogue graph. These are the nodes that do not have any incoming connections.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> RootNodes;
	/**
	 * Array containing all the nodes in the graph, including both root nodes and child nodes.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> AllNodes;
	// Flag indicating whether an edge is enabled
	UPROPERTY(BlueprintReadOnly, Category = "Mountea|Dialogue")
	bool bEdgeEnabled;

#pragma endregion

#pragma region Functions

public:
	/**
	 * Returns the GUID of the graph.
	 *
	 * @return The GUID of the graph.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	FGuid GetGraphGUID() const;
	/**
	 * Returns an array containing all nodes in the dialogue graph.
	 * @return An array of all nodes in the dialogue graph.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> GetAllNodes() const;
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> GetRootNodes() const;
	/**
	 * Returns the root nodes of the dialogue graph.
	 *
	 * @return An array of all root nodes.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	UMounteaDialogueGraphNode* GetStartNode() const;
	/**
	 * Returns the array of decorators that are associated with this graph.
	 *
	 * @return The array of decorators.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	TArray<FMounteaDialogueDecorator> GetGraphDecorators() const;
	/**
	 * Returns the array of decorators that are associated with this graph and its nodes.
	 *
	 * @return The array of decorators.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	TArray<FMounteaDialogueDecorator> GetAllDecorators() const;
	/**
	 * Determines whether the dialogue graph can be started.
	 * 
	 * @return true if the graph can be started, false otherwise.
	 */
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

#endif
};
