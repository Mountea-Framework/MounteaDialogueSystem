// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_StartNode.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=("Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_StartNode : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_StartNode();

	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
};
