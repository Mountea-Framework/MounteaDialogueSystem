// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not 
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "MounteaDialogueGraphNode_OpenChildGraph.generated.h"

class UMounteaDialogueGraph;

/**
 * Mountea Dialogue Open Child Graph Node
 *
 * Placeholder node used to represent transition into another dialogue graph.
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), NotBlueprintable, DisplayName="Open Child Graph", meta=(ToolTip="Mountea Dialogue Tree: Open Child Graph Node"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_OpenChildGraph : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_OpenChildGraph();

	virtual void ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;

	/** Soft reference to the dialogue graph which should be opened in a future implementation pass. */
	UPROPERTY(SaveGame, Category="Mountea|Dialogue", EditAnywhere, BlueprintReadOnly, 
		meta=(NoResetToDefault),
		meta=(DisplayThumbnail="false"),
		meta=(ForceShowEngineContent, ForceShowPluginContent))
	TSoftObjectPtr<UMounteaDialogueGraph> TargetDialogue;

#if WITH_EDITOR
	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
	virtual FString GetNodeDocumentationLink_Implementation() const override
	{ return TEXT("https://mountea.tools/docs/DialogueSystem/DialogueNodes/OpenChildGraphNode/"); };
#endif
};
