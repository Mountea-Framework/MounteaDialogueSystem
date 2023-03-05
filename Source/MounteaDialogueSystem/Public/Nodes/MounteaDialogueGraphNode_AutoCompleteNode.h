// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode_CompleteNode.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_AutoCompleteNode.generated.h"

/**
 * Auto Complete Dialogue Node.
 * This node is no longer used.
 * 
 * This Node will automatically complete Dialogue.
 * Same functionality as Node being left with no branching.
 * Does help performance, however, is not required.
 */
UCLASS(Abstract, ClassGroup=("Mountea|Dialogue"), DisplayName="Auto Complete Node", meta=(ToolTip="Mountea Dialogue Tree: Auto Complete Node"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_AutoCompleteNode : public UMounteaDialogueGraphNode_CompleteNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_AutoCompleteNode();

	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
};
