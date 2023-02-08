// All rights reserved Dominik Pavlicek 2023


#include "EdNode_MounteaDialogueGraphNode.h"

#include "EdGraph_MounteaDialogueGraph.h"
#include "MounteaDialogueGraphNode.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"

#define LOCTEXT_NAMESPACE "UEdNode_MounteaDialogueGraphNode"

UEdNode_MounteaDialogueGraphNode::UEdNode_MounteaDialogueGraphNode()
{
	bCanRenameNode = true;
}

UEdNode_MounteaDialogueGraphNode::~UEdNode_MounteaDialogueGraphNode()
{
}

void UEdNode_MounteaDialogueGraphNode::SetMounteaDialogueGraphNode(UMounteaDialogueGraphNode* NewNode)
{
	DialogueGraphNode = NewNode;
}

UEdGraph_MounteaDialogueGraph* UEdNode_MounteaDialogueGraphNode::GetDialogueGraphEdGraph() const
{
	return Cast<UEdGraph_MounteaDialogueGraph>(GetGraph());
}

void UEdNode_MounteaDialogueGraphNode::AllocateDefaultPins()
{
	if (DialogueGraphNode == nullptr)
	{
		EditorLOG_ERROR(TEXT("[AllocateDefaultPins] Cannot find Owning Graph Node!"))
		return;
	}
		
	if (DialogueGraphNode->bAllowInputNodes)
	{
		CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
	}

	if (DialogueGraphNode->bAllowOutputNodes)
	{
		CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
	}
}

FText UEdNode_MounteaDialogueGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (DialogueGraphNode == nullptr)
	{
		return Super::GetNodeTitle(TitleType);
	}

	return DialogueGraphNode->GetNodeTitle();
}

void UEdNode_MounteaDialogueGraphNode::PrepareForCopying()
{
	DialogueGraphNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UEdNode_MounteaDialogueGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

FLinearColor UEdNode_MounteaDialogueGraphNode::GetBackgroundColor() const
{
	return DialogueGraphNode ? DialogueGraphNode->GetBackgroundColor() : FLinearColor::Black;
}

UEdGraphPin* UEdNode_MounteaDialogueGraphNode::GetInputPin() const
{
	return Pins[0];
}

UEdGraphPin* UEdNode_MounteaDialogueGraphNode::GetOutputPin() const
{
	if (Pins.IsValidIndex(1))
	{
		return Pins[1];
	}

	return Pins[0];
}

bool UEdNode_MounteaDialogueGraphNode::CanUserDeleteNode() const
{
	if( !Super::CanUserDeleteNode())
	{
		return false;
	}

	if (! DialogueGraphNode)
	{
		return false;
	}

	
	return DialogueGraphNode->bAllowDelete;
}

bool UEdNode_MounteaDialogueGraphNode::CanDuplicateNode() const
{
	if( !Super::CanUserDeleteNode())
	{
		return false;
	}

	if (! DialogueGraphNode)
	{
		return false;
	}

	
	return DialogueGraphNode->bAllowCopy;
}

void UEdNode_MounteaDialogueGraphNode::PostEditUndo()
{
	Super::PostEditUndo();
}

#undef LOCTEXT_NAMESPACE