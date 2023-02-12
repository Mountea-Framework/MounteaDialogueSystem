// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode.h"

#include "Graph/MounteaDialogueGraph.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueNode"

UMounteaDialogueGraphNode::UMounteaDialogueGraphNode()
{
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UMounteaDialogueGraph::StaticClass();

	BackgroundColor = FLinearColor::Black;

	bAllowInputNodes = true;
	bAllowOutputNodes = true;

	bAllowCopy = true;
	bAllowCut = true;
	bAllowDelete = true;
	bAllowPaste = true;
	bAllowManualCreate = true;
#endif
}

UMounteaDialogueGraphEdge* UMounteaDialogueGraphNode::GetEdge(UMounteaDialogueGraphNode* ChildNode)
{
	return Edges.Contains(ChildNode) ? Edges.FindChecked(ChildNode) : nullptr;
}

bool UMounteaDialogueGraphNode::IsLeafNode() const
{
	return ChildrenNodes.Num() == 0;
}

UMounteaDialogueGraph* UMounteaDialogueGraphNode::GetGraph() const
{
	return Graph;
}

FText UMounteaDialogueGraphNode::GetDescription_Implementation() const
{
	return LOCTEXT("NodeDesc", "Mountea Dialogue Graph Node");
}

FText UMounteaDialogueGraphNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("NodeCategory", "Mountea Dialogue Tree Node");
}

#if WITH_EDITOR

FLinearColor UMounteaDialogueGraphNode::GetBackgroundColor() const
{
	return BackgroundColor;
}

FText UMounteaDialogueGraphNode::GetNodeTitle() const
{
	return NodeTitle;
}

void UMounteaDialogueGraphNode::SetNodeTitle(const FText& NewTitle)
{
	NodeTitle = NewTitle;
}

bool UMounteaDialogueGraphNode::CanCreateConnection(UMounteaDialogueGraphNode* Other, enum EEdGraphPinDirection Direction, FText& ErrorMessage)
{
	if (Other == nullptr)
	{
		ErrorMessage = FText::FromString("Invalid Other node!");
		return false;
	}
	
	return true;
}

#endif

#undef LOCTEXT_NAMESPACE
