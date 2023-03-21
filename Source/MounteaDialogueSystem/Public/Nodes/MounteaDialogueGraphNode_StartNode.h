// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_StartNode.generated.h"

/**
 * Mountea Dialogue Start Node.
 * 
 * This Node will be added to the Dialogue Graph automatically when Graph is created.
 * This Node cannot be created manually.
 * This Node cannot be deleted from Graph.
 * Does not implement any logic, works as an Anchor starting point.
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), DisplayName="Start Node", meta=(ToolTip="Mountea Dialogue Tree: Start Node"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_StartNode : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_StartNode();

#if WITH_EDITOR
	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
	
	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat);
#endif
};
