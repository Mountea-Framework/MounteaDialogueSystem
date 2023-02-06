// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "EdNode_MounteaDialogueGraphEdge.generated.h"

class UEdNode_MounteaDialogueGraphNode;
class UMounteaDialogueGraphEdge;

/**
 * 
 */
UCLASS(MinimalAPI)
class  UEdNode_MounteaDialogueGraphEdge : public UEdGraphNode
{
	GENERATED_BODY()

public:
	
	UPROPERTY()
	class UEdGraph* Graph;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Mountea Dialogue Graph")
	UMounteaDialogueGraphEdge* MounteaDialogueGraphEdge;

public:

	void SetEdge(UMounteaDialogueGraphEdge* Edge);

	virtual void AllocateDefaultPins() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;

	virtual void PrepareForCopying() override;

	virtual UEdGraphPin* GetInputPin() const { return Pins[0]; }
	virtual UEdGraphPin* GetOutputPin() const { return Pins[1]; }

	void CreateConnections(UEdNode_MounteaDialogueGraphNode* Start, UEdNode_MounteaDialogueGraphNode* End);

	UEdNode_MounteaDialogueGraphNode* GetStartNode();
	UEdNode_MounteaDialogueGraphNode* GetEndNode();
};
