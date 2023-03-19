// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "MounteaDialogueGraphNode_LeadNode.generated.h"

/**
 * Mountea Dialogue Lead Node.
 * 
 * This Node represents NPC lines.
 * This Node starts automatically upon reaching in the Dialogue Tree.
 * Requires Dialogue Data Table to work properly.
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), DisplayName="Lead Node", meta=(ToolTip="Mountea Dialogue Tree: Lead Node"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_LeadNode : public UMounteaDialogueGraphNode_DialogueNodeBase
{
	GENERATED_BODY()
	
	UMounteaDialogueGraphNode_LeadNode();

public:

	virtual void PreProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;
	virtual void ProcessNode() override;

#if WITH_EDITOR
	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
#endif
};
