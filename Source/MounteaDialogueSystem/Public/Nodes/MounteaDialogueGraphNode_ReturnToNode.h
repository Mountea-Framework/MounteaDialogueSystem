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
UCLASS( ClassGroup=("Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_ReturnToNode : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

	UMounteaDialogueGraphNode_ReturnToNode();

public:

	virtual void ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;

public:

	/**
	 * Defines how long it takes before the actual Jump happens.
	 * Short delay time can avoid cutting audio and can provide time for Client-sided actions.
	 */
	UPROPERTY(SaveGame, Category="Return", EditAnywhere, BlueprintReadOnly, 
		meta=(Units = "s", UIMin = 0.01, ClampMin = 0.01),
		meta=(NoResetToDefault))
	float DelayDuration;

	/**
	 * Shows list of Node Indexes.
	 * This is not very user friendly, however, to combat this Preview is generated with ability to click on it and get focused on selected Node.
	 */
	UPROPERTY(SaveGame, Category="Return", EditAnywhere, BlueprintReadOnly, 
		meta=(GetOptions ="GetRowNames"),
		meta=(NoResetToDefault),
		meta=(HiddenInGraph))
	FString SelectedNodeIndex;

	UPROPERTY(SaveGame, Category="Return", EditAnywhere, BlueprintReadOnly, 
		meta=(EditCondition ="SelectedNode!=nullptr"),
		meta=(NoResetToDefault))
	uint8 bAutoCompleteSelectedNode : 1;

	/**
	 * Dialogue Node to which this Node leads to.
	 */
	UPROPERTY(SaveGame, Category="Private", VisibleAnywhere, BlueprintReadOnly, 
		meta=(NoResetToDefault, DisplayThumbnail="false"),
		meta=(HiddenInGraph))
	TObjectPtr<UMounteaDialogueGraphNode> SelectedNode;

	/**
	 * Filters OUT all nodes by that class.
	 */
	UPROPERTY(SaveGame, Category="Editor", EditAnywhere, BlueprintReadOnly, 
		meta=(GetOptions ="GetRowNames", NoResetToDefault),
		meta=(HiddenInGraph))
	TArray<TSubclassOf<UMounteaDialogueGraphNode>> AllowedNodesFilter;

#if WITH_EDITORONLY_DATA

	FSimpleDelegate ReturnNodeUpdated;

#endif

protected:

	FTimerHandle TimerHandle_Delay;

	UFUNCTION()
	void OnDelayDurationExpired(const TScriptInterface<IMounteaDialogueManagerInterface>& MounteaDialogueManagerInterface);

#if WITH_EDITOR
	
	virtual FText GetNodeCategory_Implementation() const override;
	virtual bool ValidateNode(FDataValidationContext& Context, const bool RichFormat) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual FText GetDescription_Implementation() const override;
	virtual FString GetNodeDocumentationLink_Implementation() const override
	{ return TEXT("https://mountea.tools/docs/DialogueSystem/DialogueNodes/ReturnToNode/"); };

#endif

private:

	UFUNCTION()
	TArray<FString> GetRowNames() const;
};
