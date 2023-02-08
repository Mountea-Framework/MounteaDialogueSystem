// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_AnswerNode.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=("Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_AnswerNode : public UMounteaDialogueGraphNode_DialogueNodeBase
{
	GENERATED_BODY()
	
public:

	UMounteaDialogueGraphNode_AnswerNode();

	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
};
