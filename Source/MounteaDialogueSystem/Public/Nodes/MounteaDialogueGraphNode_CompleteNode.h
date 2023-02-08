// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_CompleteNode.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=("Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_CompleteNode : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_CompleteNode();

	virtual FText GetDescription_Implementation() const override;
};
