// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_AnswerNode.generated.h"

/**
 * Mountea Dialogue Answer Node.
 * 
 * This Node represents Player's answers.
 * This Node requires Player's input to be started.
 * Requires Dialogue Data Table to work properly.
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), DisplayName="Answer Node", meta=(ToolTip="Mountea Dialogue Tree: Answer Node"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_AnswerNode : public UMounteaDialogueGraphNode_DialogueNodeBase
{
	GENERATED_BODY()
	
	UMounteaDialogueGraphNode_AnswerNode();

public:

	virtual void PreProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;
	virtual void ProcessNode() override;

#if WITH_EDITOR
	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
#endif
};
