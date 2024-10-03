// All rights reserved Dominik Pavlicek 2023


#include "EdNode_MounteaDialogueGraphNode.h"

#include "Helpers/MounteaDialogueSystemEditorBFC.h"
#include "EdGraph_MounteaDialogueGraph.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"

#define LOCTEXT_NAMESPACE "UEdNode_MounteaDialogueGraphNode"

UEdNode_MounteaDialogueGraphNode::UEdNode_MounteaDialogueGraphNode()
{
	bCanRenameNode = true;

	bAllowCopy = true;
	bAllowDelete = true;
	bAllowDuplicate = true;
	bAllowPaste = true;
}

UEdNode_MounteaDialogueGraphNode::~UEdNode_MounteaDialogueGraphNode()
{
}

void UEdNode_MounteaDialogueGraphNode::SetMounteaDialogueGraphNode(UMounteaDialogueGraphNode* NewNode)
{
	DialogueGraphNode = NewNode;

	if (DialogueGraphNode)
	{
		bAllowCopy = DialogueGraphNode->bAllowCopy;
		bAllowDelete = DialogueGraphNode->bAllowDelete;
		bAllowDuplicate = DialogueGraphNode->bAllowPaste;
		bAllowPaste = DialogueGraphNode->bAllowPaste;
	}
}

UEdGraph_MounteaDialogueGraph* UEdNode_MounteaDialogueGraphNode::GetDialogueGraphEdGraph() const
{
	return Cast<UEdGraph_MounteaDialogueGraph>(GetGraph());
};

void UEdNode_MounteaDialogueGraphNode::AllocateDefaultPins()
{
	if (DialogueGraphNode == nullptr)
	{
		EditorLOG_ERROR(TEXT("[AllocateDefaultPins] Cannot find Owning Graph Node!"))
		//return;
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
	return UMounteaDialogueSystemEditorBFC::GetNodeTitle(DialogueGraphNode);;
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
	// Getting Node colour based on the Settings if any found, otherwise use this logic
	if (UMounteaDialogueGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
	{
		FLinearColor ReturnColour;
		if (GraphEditorSettings->FindNodeBackgroundColourOverride(DialogueGraphNode->GetClass(), ReturnColour))
		{
			return ReturnColour;
		}
	}
	
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

	if (DialogueGraphNode)
	{
		return DialogueGraphNode->bAllowDelete;
	}	
	return bAllowDelete;
}

bool UEdNode_MounteaDialogueGraphNode::CanDuplicateNode() const
{
	if( !Super::CanUserDeleteNode())
	{
		return false;
	}

	if (DialogueGraphNode)
	{
		return DialogueGraphNode->bAllowCopy;
	}
	
	return bAllowCopy;
}

bool UEdNode_MounteaDialogueGraphNode::CanUserPasteNodes() const
{
	if (DialogueGraphNode)
	{
		return DialogueGraphNode->bAllowPaste;
	}

	return bAllowPaste;
}

FText UEdNode_MounteaDialogueGraphNode::GetTooltipText() const
{
	if (DialogueGraphNode)
	{
		return DialogueGraphNode->GetNodeTooltipText();
	}
	
	return NSLOCTEXT("UEdNode_MounteaDialogueGraphNode", "DefaultToolTip", "Mountea Dialogue Node");
}

FSlateIcon UEdNode_MounteaDialogueGraphNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static const FSlateIcon Icon = FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.Node.Icon.small");
	OutColor = DialogueGraphNode->GetBackgroundColor();
	return Icon;
}

bool UEdNode_MounteaDialogueGraphNode::Modify(bool bAlwaysMarkDirty)
{
	bool bSatisfied = Super::Modify(bAlwaysMarkDirty);

	UpdatePosition();
	
	return bSatisfied;
}

void UEdNode_MounteaDialogueGraphNode::UpdatePosition()
{
	if (DialogueGraphNode)
	{
		DialogueGraphNode->NodePosition = FIntPoint(NodePosX, NodePosY);
		DialogueGraphNode->Modify(true);
	}
}

void UEdNode_MounteaDialogueGraphNode::PostEditUndo()
{
	Super::PostEditUndo();
}

void UEdNode_MounteaDialogueGraphNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#undef LOCTEXT_NAMESPACE
