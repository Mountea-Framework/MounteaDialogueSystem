// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "MounteaDialogueGraphNode_CompleteNode.generated.h"

/**
 * Complete Dialogue Node.
 * 
 * This Node will complete Dialogue after Player's input.
 * Indicates that Dialogue can be manually closed.
 * Requires Dialogue Data Table to work properly.
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), DisplayName="Complete Node", meta=(ToolTip="Mountea Dialogue Tree: Complete Node"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_CompleteNode : public UMounteaDialogueGraphNode_DialogueNodeBase
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_CompleteNode();

	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
};
