// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "MounteaDialogueGraphNode.generated.h"

class IMounteaDialogueManagerInterface;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphEdge;

/**
 * Mountea Dialogue Graph Node abstract Base class.
 * 
 * Provides generic functionality to be enhanced or overriden by Child Classes.
 * Does come with ability to define Colours, Name, Description and Title.
 * Contains information about Parent and Children Nodes.
 */
UCLASS(Abstract, BlueprintType, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea", "Dialogue", "Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode : public UObject
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueGraphNode();

#pragma region Variables

#pragma region ReadOnly
public:
	/**
	 * Array of parent nodes for the current active node in the dialogue traversal.
	 *❗ Parent nodes are nodes that have a directed edge pointing to the current active node.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	TArray<UMounteaDialogueGraphNode*> ParentNodes;
	/**
	 * The array of child nodes of the current dialogue node.
	 *❗ The order of the children nodes matter and determines the order in which the options are presented to the player.
	 *❔ Can be used to traverse the graph and to create UI to display the dialogue options.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	TArray<UMounteaDialogueGraphNode*> ChildrenNodes;
	/**
	 * Map of edges connecting this Node in the Mountea Dialogue Graph.
	 *❗ The key of the map is the source node, and the value is the edge connecting it to its target node.
	 *❔ Can be used to traverse the graph, get information about node connections...
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	TMap<UMounteaDialogueGraphNode*, UMounteaDialogueGraphEdge*> Edges;
	/**
	 * Pointer to the parent dialogue graph of this node.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private", meta=(DisplayThumbnail=false))
	UMounteaDialogueGraph* Graph;
	/**
	 * Temporary NodeIndex.
	 * This variable will be deleted.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	int32 NodeIndex = INDEX_NONE;


protected:
	/**
	 * The unique identifier for this Dialogue Graph Node.
	 *❗ This is used to differentiate between nodes, and must be unique within the graph.
	 *❔ Can be used for debugging and tracing purposes.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	FGuid NodeGUID;

private:
	/**
	 * The world that owns this Dialogue Graph Node.
	 *❗ This is the world in which this Dialogue Graph Node is currently running.
	 *❔ Can be used for accessing world-related functionality.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Private")
	UWorld* OwningWorld;

#pragma endregion

#pragma region Editable
public:
	/**
	 * The array of allowed input classes for this Dialogue Node.
	 *❗ Only nodes with classes from this array can be connected as inputs to this node.
	 *❔ Can be used to restrict the types of inputs this node can accept.
	 */
	UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly, Category="Base")
	TArray<TSubclassOf<UMounteaDialogueGraphNode>> AllowedInputClasses;

	/** Defines whether this Node will start automatically or if requires input.*/
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadOnly, Category="Base")
	uint8 bAutoStarts : 1;
	/**
	 * The maximum number of children nodes that this node can have.
	 *❗ If this value is -1, then there is no limit on the number of children nodes.
	 *❔ Can be used to enforce a maximum number of connections for certain types of nodes.
	 */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Base")
	int32 MaxChildrenNodes = -1;
	/**
	 * Indicates whether this node inherits the decorators from its parent Graph.
	 *❗ If true, the decorators of the parent Graph will be inherited and applied to this node during processing.
	 *❔ This flag can be used to control the inheritance of decorators for nodes in the dialogue graph.
	 */
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
	/**
	 * Initializes the node with the given world.
	 *
	 * @param InWorld The world to use for initialization.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue")
	void InitializeNode(UWorld* InWorld);
	virtual void InitializeNode_Implementation(UWorld* InWorld);
	/**
	 * Checks if the node should automatically start when the dialogue is played.
	 * @return true if the node should automatically start, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool DoesAutoStart() const
	{ return bAutoStarts; };
	
	virtual void PreProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);
	virtual void ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);
	/**
	 * Gets the decorators for this Dialogue Graph Node.
	 *❔ Returns only Valid decorators!
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	TArray<FMounteaDialogueDecorator> GetNodeDecorators() const;
	/**
	 * Returns true if the node can be started.
	 *❗ The implementation of this function is up to the subclass.
	 *❔ Can be used to validate if a node can be started before attempting to start it.
	 *❔ This can be further enhanced by Decorators.
	 * @return True if the node can be started, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool CanStartNode() const;
	virtual bool EvaluateDecorators() const;
	/**
	 * Returns whether this node inherits decorators from the dialogue graph.
	 * If this is set to true, this node will receive all decorators assigned to the graph.
	 * If it's set to false, the node will only have its own decorators.
	 *
	 * @return Whether this node inherits decorators from the graph.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	bool DoesInheritDecorators() const
	{ return bInheritGraphDecorators; };
	/**
	 * Returns how many Children Nodes this Node allows to have.
	 *❔ -1 means no limits.
	 *
	 * @return MaxChildrenNodes
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	int32 GetMaxChildNodes() const
	{ return MaxChildrenNodes; };


	/**
	 * Gets the index of the node within the dialogue graph.
	 *
	 * @return The index of the node.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE int32 GetNodeIndex() const
	{ return NodeIndex; };
	/**
	 * Sets the index of this dialogue node in the dialogue graph.
	 *
	 * @param NewIndex The new index to set.
	 */
	void SetNodeIndex(const int32 NewIndex);

	/**
	 * Gets the GUID of the node.
	 *
	 * @return The GUID of the node.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE FGuid GetNodeGUID() const
	{ return NodeGUID; };

	/**
	 * Gets the owning Graph of the node.
	 *❗ Might be null
	 *
	 * @return The owning Graph of the node.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	UMounteaDialogueGraph* GetGraph() const;

	
	/**
	 * Gets children Nodes this one has,
	 *❗ Might be empty
	 *
	 * @return Amount of children Nodes
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE TArray<UMounteaDialogueGraphNode*> GetChildrenNodes() const
	{ return ChildrenNodes; };
	/**
	 * Gets how many parent Nodes point to this one
	 *❗ Might be empty
	 *
	 * @return Amount of how parent Nodes
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FORCEINLINE TArray<UMounteaDialogueGraphNode*> GetParentNodes() const
	{return ParentNodes; };

	/**
	 * Serves purpose of validating Node before Dialogue gets Started.
	 * Any broken Node results in non-starting Dialogue to avoid crashes.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue")
	bool ValidateNodeRuntime() const;
	virtual bool ValidateNodeRuntime_Implementation() const
	{ return true; };
	
public:

	FORCEINLINE ULevel* GetLevel() const
	{
		return GetTypedOuter<ULevel>();
	}

	/**
	 * Provides a way to update Node's owning World.
	 * Useful for Loading sub-levels.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetNewWorld(UWorld* NewWorld);
	virtual UWorld* GetWorld() const override
	{
		if (OwningWorld) return OwningWorld;
		
		// CDO objects do not belong to a world
		// If the actors outer is destroyed or unreachable we are shutting down and the world should be nullptr
		if (
			!HasAnyFlags(RF_ClassDefaultObject) && ensureMsgf(GetOuter(), TEXT("Actor: %s has a null OuterPrivate in AActor::GetWorld()"), *GetFullName())
			&& !GetOuter()->HasAnyFlags(RF_BeginDestroyed) && !GetOuter()->IsUnreachable()
			)
		{
			if (ULevel* Level = GetLevel())
			{
				return Level->OwningWorld;
			}
		}
		return nullptr;
	}

#pragma endregion 


#if WITH_EDITORONLY_DATA

	// Defines whether this Node type allows inputs
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowInputNodes;

	// Defines whether this Node type allows outputs
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowOutputNodes;

	// Defines whether this Node can be copied
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowCopy;

	// Defines whether this Node can be cut
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowCut;

	// Defines whether this Node can be pasted
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowPaste;

	// Defines whether this Node can be deleted
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowDelete;

	// Defines whether this Node can be manually created
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowManualCreate;

	// Display title of the Node
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText NodeTitle;

	// Display name of the Node menu category
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText ContextMenuName;

	// List of compatible graph types
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Editor")
	TSubclassOf<UObject> CompatibleGraphType;

	// Defines background colour of this Node
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FLinearColor BackgroundColor;

	// Contains Node Tooltip text
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText NodeTooltipText;

	// User friendly node type name
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText NodeTypeName;

#endif

#if WITH_EDITOR

	/**
	 * Returns the tooltip text for this graph node.
	 *
	 * @return The tooltip text for this node.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeTooltipText() const;
	virtual FText GetNodeTooltipText_Implementation() const;

	/**
	 * Returns the Title text for this graph node.
	 *
	 * @return The Title text for this node.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeTitle() const;
	virtual FText GetNodeTitle_Implementation() const;

	/**
	 * Returns the Description text for this graph node.
	 *
	 * @return The Description text for this node.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetDescription() const;
	virtual FText GetDescription_Implementation() const;

	/**
	 * Returns the Node Category text for this graph node.
	 *
	 * @return The Node Category text for this node.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeCategory() const;
	virtual FText GetNodeCategory_Implementation() const;

	/**
	 * Returns the Documentation Link for this graph node.
	 *
	 * @return The Documentation Link for this node.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FString GetNodeDocumentationLink() const;
	virtual FString GetNodeDocumentationLink_Implementation() const;

	/**
	 * Returns the Background Colour for this graph node.
	 *
	 * @return The Background Colour for this node.
	 */
	virtual FLinearColor GetBackgroundColor() const;
	FText GetInternalName() const
	{ return NodeTypeName; };

	// Allows setting up the Node Title
	virtual void SetNodeTitle(const FText& NewTitle);

	// Allows advanced filtering if Node can be connected from other Node
	virtual bool CanCreateConnection(UMounteaDialogueGraphNode* Other, enum EEdGraphPinDirection Direction, FText& ErrorMessage);

	// Validation function responsible for generating user friendly validation messages
	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat);

	// Once Node is pasted, this function is called
	virtual void OnPasted();

	// Generates default Tooltip body text used for all Nodes
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetDefaultTooltipBody() const;
	virtual void OnCreatedInEditor() {};

#endif

};
