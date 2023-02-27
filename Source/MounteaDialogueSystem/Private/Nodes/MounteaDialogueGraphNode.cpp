// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode.h"

#include "Graph/MounteaDialogueGraph.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueNode"

UMounteaDialogueGraphNode::UMounteaDialogueGraphNode()
{
	NodeGUID = FGuid::NewGuid();
	bInheritGraphDecorators = true;

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

	bHasBenManuallyRenamed = false;

	InternalName = LOCTEXT("MounteaDialogueNode_InternalName", "MounteaDialogueGraphNode");
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

void UMounteaDialogueGraphNode::SetNodeIndex(const int32 NewIndex)
{
	check(NewIndex>INDEX_NONE);
	NodeIndex = NewIndex;
}

bool UMounteaDialogueGraphNode::CanStartNode() const
{
	return true;
}

FText UMounteaDialogueGraphNode::GetNodeTitle_Implementation() const
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

bool UMounteaDialogueGraphNode::ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat)
{
	bool bResult = true;
	if (ParentNodes.Num() == 0 && ChildrenNodes.Num() == 0)
	{
		bResult = false;
		
		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": This Node has no Connections!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": This Node has no Connections!");
		
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (bAllowInputNodes && ParentNodes.Num() == 0)
	{
		bResult = false;
		
		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": This Node requires Inputs, however, none are found!");
		
		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": This Node requires Inputs, however, none are found!");
		
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (NodeDecorators.Num() > 1)
	{
		TMap<int32, FMounteaDialogueDecorator> Duplicates;
		for (int i = 0; i < NodeDecorators.Num(); i++)
		{
			for (auto Itr2 : NodeDecorators)
			{
				if (NodeDecorators[i].DecoratorType == Itr2.DecoratorType)
				{
					Duplicates.Add(i, NodeDecorators[i]);
				}
			}
		}
	
		if (Duplicates.Num() > 0)
		{
			const FString RichTextReturn =
			FString("* ").
			Append(TEXT("<RichTextBlock.Bold>Dialogue Graph</>")).
			Append(": has ").
			Append(FString::FromInt(Duplicates.Num())).
			Append("x the Decorators! Please, try to avoid duplicates.");

			const FString TextReturn =
			GetName().
			Append(": has ").
			Append(FString::FromInt(Duplicates.Num())).
			Append("x the Decorators! Please, try to avoid duplicates.");
		
			ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));

			bResult = false;
		}
	}
	
	return bResult;
}

void UMounteaDialogueGraphNode::OnPasted()
{
	NodeGUID = FGuid::NewGuid();
}

#endif

#undef LOCTEXT_NAMESPACE
