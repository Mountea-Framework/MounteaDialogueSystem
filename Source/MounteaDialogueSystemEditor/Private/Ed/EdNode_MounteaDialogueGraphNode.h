// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
//#include "EdGraph_MounteaDialogueGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdNode_MounteaDialogueGraphNode.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraph;
class SEdNode_MounteaDialogueGraphNode;
class UEdGraph_MounteaDialogueGraph;

/**
 * 
 */
UCLASS(MinimalAPI)
class UEdNode_MounteaDialogueGraphNode : public UEdGraphNode
{
	GENERATED_BODY()
	
public:

	UEdNode_MounteaDialogueGraphNode();
	virtual ~UEdNode_MounteaDialogueGraphNode() override;

	void SetMounteaDialogueGraphNode(UMounteaDialogueGraphNode* NewNode);
	
	UEdGraph_MounteaDialogueGraph* GetDialogueGraphEdGraph() const;

	virtual int32 GetDialogueNodeIndex() const { return NodeIndex; }
	/** Sets the Dialogue node index number, this represents the index from the DlgDialogue.Nodes Array */
	virtual void SetDialogueNodeIndex(int32 InIndex)
	{
		check(InIndex > INDEX_NONE);
		NodeIndex = InIndex;
	}

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PrepareForCopying() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

	virtual FLinearColor GetBackgroundColor() const;
	virtual UEdGraphPin* GetInputPin() const;
	virtual UEdGraphPin* GetOutputPin() const;

	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;

	virtual bool CanUserPasteNodes() const;

	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Mountea Dialogue Graph")
	UMounteaDialogueGraphNode* DialogueGraphNode;

	SEdNode_MounteaDialogueGraphNode* SEdNode;

	int32 NodeIndex = INDEX_NONE;

private:

	bool bAllowCopy;
	bool bAllowDelete;
	bool bAllowDuplicate;
	bool bAllowPaste;
};
