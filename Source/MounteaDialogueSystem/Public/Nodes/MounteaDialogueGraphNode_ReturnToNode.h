// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "MounteaDialogueGraphNode_ReturnToNode.generated.h"

/**
 * Mountea Dialogue Graph Node Return To Node
 * 
 * Provides ability to return from Dialogue Node to different one.
 * Useful when dialogue branching disallows pin connections.
 */
UCLASS( ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea", "Dialogue", "Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_ReturnToNode : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

	UMounteaDialogueGraphNode_ReturnToNode();

public:

	virtual void ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;

public:

	/**
	 * Shows list of Node Indexes.
	 * This is not very user friendly, however, to combat this Preview is generated with ability to click on it and get focused on selected Node.
	 */
	UPROPERTY(SaveGame, Category="Mountea|Dialogue", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames"))
	FString SelectedNodeIndex;

	UPROPERTY(SaveGame, Category="Private", VisibleAnywhere, BlueprintReadOnly, meta=(NoResetToDefault, DisplayThumbnail="false"))
	UMounteaDialogueGraphNode* SelectedNode;

	/**
	 * Filters OUT all nodes by that class.
	 */
	UPROPERTY(SaveGame, Category="Editor", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames", NoResetToDefault))
	TArray<TSubclassOf<UMounteaDialogueGraphNode>> AllowedNodesFilter;

#if WITH_EDITORONLY_DATA

	FSimpleDelegate ReturnNodeUpdated;

#endif

protected:

#if WITH_EDITOR
	
	virtual FText GetNodeCategory_Implementation() const override;
	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual FText GetDescription_Implementation() const override;
	virtual FString GetNodeDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Return-To-Node"); };

#endif

private:

	UFUNCTION()
	TArray<FString> GetRowNames() const
	{
		if (Graph)
		{
			TArray<FString> NodesNames;
			for (const auto& Itr : Graph->GetAllNodes())
			{
				if (Itr)
				{
					// Check if this is allowed class
					bool bIsAllowed = true;
					for (auto& ItrClass : AllowedNodesFilter)
					{
						if (Itr->IsA(ItrClass)) bIsAllowed = false;
					}
					
					// Show only those allowed
					if (bIsAllowed)
					{
						FString AllowedIndex = FString::FromInt(Graph->AllNodes.Find(Itr));
						NodesNames.Add(AllowedIndex);
					}
				}
			}

			return NodesNames;
		}

		return TArray<FString>();
	}
};
