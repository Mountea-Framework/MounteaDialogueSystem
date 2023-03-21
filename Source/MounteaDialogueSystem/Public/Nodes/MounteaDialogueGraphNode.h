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
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	TArray<UMounteaDialogueGraphNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	TArray<UMounteaDialogueGraphNode*> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	TMap<UMounteaDialogueGraphNode*, UMounteaDialogueGraphEdge*> Edges;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private", meta=(DisplayThumbnail=false))
	UMounteaDialogueGraph* Graph;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	int32 NodeIndex = INDEX_NONE;


protected:

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Private")
	FGuid NodeGUID;

private:

	UPROPERTY(VisibleAnywhere, Category = "Private")
	UWorld* OwningWorld;

#pragma endregion

#pragma region Editable
public:

	UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly, Category="Base")
	TArray<TSubclassOf<UMounteaDialogueGraphNode>> AllowedInputClasses;

	/** Defines whether this Node will start automatically or if requires input.*/
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadOnly, Category="Base")
	uint8 bAutoStarts : 1;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Base")
	int32 MaxChildrenNodes = -1;

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

	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue")
	void InitializeNode(UWorld* InWorld);
	virtual void InitializeNode_Implementation(UWorld* InWorld);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool DoesAutoStart() const
	{ return bAutoStarts; };
	
	virtual void PreProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);
	virtual void ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);

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
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowInputNodes;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowOutputNodes;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowCopy;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowCut;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowPaste;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowDelete;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	bool bAllowManualCreate;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText NodeTitle;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText ContextMenuName;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Editor")
	TSubclassOf<UMounteaDialogueGraph> CompatibleGraphType;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FLinearColor BackgroundColor;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText NodeTooltipText;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Editor")
	FText NodeTypeName;

#endif

#if WITH_EDITOR

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeTooltipText() const;
	virtual FText GetNodeTooltipText_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeTitle() const;
	virtual FText GetNodeTitle_Implementation() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetDescription() const;
	virtual FText GetDescription_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FText GetNodeCategory() const;
	virtual FText GetNodeCategory_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue", meta=(DevelopmentOnly=true))
	FString GetNodeDocumentationLink() const;
	virtual FString GetNodeDocumentationLink_Implementation() const;
	
	virtual FLinearColor GetBackgroundColor() const;
	FText GetInternalName() const
	{ return NodeTypeName; };
	
	virtual void SetNodeTitle(const FText& NewTitle);
	
	virtual bool CanCreateConnection(UMounteaDialogueGraphNode* Other, enum EEdGraphPinDirection Direction, FText& ErrorMessage);

	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat);

	virtual void OnPasted();

	FText GetDefaultTooltipBody() const;
	virtual void OnCreatedInEditor() {};

#endif

};
