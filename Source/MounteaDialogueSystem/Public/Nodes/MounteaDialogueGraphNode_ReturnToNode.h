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

	UPROPERTY(SaveGame, Category="Mountea|Dialogue", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames", NoResetToDefault))
	FString SelectedNodeGUID;

	UPROPERTY(SaveGame, Category="Mountea|Dialogue", VisibleAnywhere, BlueprintReadOnly, meta=(NoResetToDefault))
	FText SelectedNodeName;

	UPROPERTY(SaveGame, Category="Mountea|Dialogue", VisibleAnywhere, BlueprintReadOnly, meta=(NoResetToDefault, DisplayThumbnail="false"))
	UMounteaDialogueGraphNode* SelectedNode;

	UPROPERTY(SaveGame, Category="Mountea|Dialogue", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames", NoResetToDefault))
	TSubclassOf<UMounteaDialogueGraphNode> AllowedNodesFilter;
	
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
				// TODO: Implement allowed Filter
				if (Itr && Itr != this)
				{
					FString NodeName = Itr->GetNodeGUID().ToString();
					NodesNames.Add(NodeName);
				}
			}

			return NodesNames;
		}

		return TArray<FString>();
	}
};
