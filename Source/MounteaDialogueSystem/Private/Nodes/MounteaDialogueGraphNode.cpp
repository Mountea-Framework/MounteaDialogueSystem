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

	InternalName = LOCTEXT("MounteaDialogueNode_InternalName", "MounteaDialogueGraphNode");
	NodeTooltipText = LOCTEXT("MounteaDialogueNode_Tooltip", "Mountea Dialogue Base Node.\n\nChild Nodes provide more Information.");
#endif
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

TArray<FMounteaDialogueDecorator> UMounteaDialogueGraphNode::GetNodeDecorators() const
{
	TArray<FMounteaDialogueDecorator> TempReturn;
	TArray<FMounteaDialogueDecorator> Return;
	
	for (auto Itr : NodeDecorators)
	{
		if (Itr.DecoratorType != nullptr)
		{
			TempReturn.AddUnique(Itr);
		}
	}

	/* TODO: Cleanup duplicates
	for (auto Itr : TempReturn)
	{
		
	}
	*/
	
	Return = TempReturn;
	return Return;
}

bool UMounteaDialogueGraphNode::CanStartNode() const
{
	return EvaluateDecorators();
}

bool UMounteaDialogueGraphNode::EvaluateDecorators() const
{
	if (GetGraph() == nullptr)
	{
		LOG_ERROR(TEXT("[EvaluateDecorators] Graph is null (invalid)!"))
		return false;
	}
	
	bool bSatisfied = true;
	TArray<FMounteaDialogueDecorator> AllDecorators;
	if (bInheritGraphDecorators)
	{
		// Add those Decorators rather than asking Graph to evaluate, because Nodes might introduce specific context
		AllDecorators.Append(GetGraph()->GetGraphDecorators());
	}

	AllDecorators.Append(GetNodeDecorators());

	if (AllDecorators.Num() == 0) return bSatisfied;

	for (auto Itr : AllDecorators)
	{
		if (Itr.EvaluateDecorator() == false) bSatisfied = false;
	}

	return bSatisfied;
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode::GetNodeTooltipText_Implementation() const
{
	return FText::Format(LOCTEXT("MounteaDialogueGraphNode_FinalTooltip", "{0}\n\n{1}"), GetDefaultTooltipBody(), NodeTooltipText);
}

FLinearColor UMounteaDialogueGraphNode::GetBackgroundColor() const
{
	return BackgroundColor;
}

void UMounteaDialogueGraphNode::SetNodeIndex(const int32 NewIndex)
{
	check(NewIndex>INDEX_NONE);
	NodeIndex = NewIndex;
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

	// DECORATORS VALIDATION
	{
		TArray<UMounteaDialogueDecoratorBase*> UsedNodeDecorators;
		for (int i = 0; i < NodeDecorators.Num(); i++)
		{
			if (NodeDecorators.IsValidIndex(i) && NodeDecorators[i].DecoratorType && UsedNodeDecorators.Contains(NodeDecorators[i].DecoratorType) == false)
			{
				UsedNodeDecorators.Add(NodeDecorators[i].DecoratorType);
			}
			else
			{
				const FString RichTextReturn =
				FString("* ").
				Append( TEXT("<RichTextBlock.Bold>")).
				Append(GetNodeTitle().ToString()).
				Append(TEXT("</>")).
				Append(": has ").
				Append(TEXT("<RichTextBlock.Bold>invalid</> Node Decorator at Index: ")).
				Append(FString::FromInt(i )).
				Append(".");

				FString TextReturn = GetNodeTitle().ToString();
				TextReturn.
				Append(": has ").
				Append(TEXT("INVALID Node Decorator at Index: ")).
				Append(FString::FromInt(i )).
				Append(".");
		
				ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));

				bResult = false;
			}
		}

		TMap<UClass*, int32> DuplicatedDecoratorsMap;
		for (const auto Itr : UsedNodeDecorators)
		{
			int32 ClassAppearance = 1;
			for (const auto Itr2 : UsedNodeDecorators)
			{
				if (Itr != Itr2 && Itr->GetClass() == Itr2->GetClass())
				{
					auto A = Itr->GetClass()->GetName();
					ClassAppearance++;
				}
			}

			if (ClassAppearance > 1 && DuplicatedDecoratorsMap.Contains(Itr->GetClass()) == false)
			{
				DuplicatedDecoratorsMap.Add(Itr->GetClass(), ClassAppearance);
			}
		}

		if (DuplicatedDecoratorsMap.Num() > 0)
		{
			for (const auto Itr : DuplicatedDecoratorsMap)
			{
				bResult = false;
		
				const FString RichTextReturn =
				FString("* ").
				Append("<RichTextBlock.Bold>").
				Append(NodeTitle.ToString()).
				Append("</>").
				Append(": has Node Decorator ").
				Append("<RichTextBlock.Bold>").
				Append(Itr.Key->GetName().LeftChop(2)).
				Append("</> ").
				Append(FString::FromInt(Itr.Value)).
				Append("x times! Please, avoid duplicates!");
			
				const FString TextReturn =
				FString(NodeTitle.ToString()).
				Append(NodeTitle.ToString()).
				Append(": has Node Decorator ").
				Append( Itr.Key->GetName().LeftChop(2)).
				Append(" ").
				Append(FString::FromInt(Itr.Value)).
				Append("x times! Please, avoid duplicates!");
		
				ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
			}
		}
	}
	
	return bResult;
}

void UMounteaDialogueGraphNode::OnPasted()
{
	NodeGUID = FGuid::NewGuid();
}

FText UMounteaDialogueGraphNode::GetDefaultTooltipBody() const
{
	const FText InheritsValue = bInheritGraphDecorators ? LOCTEXT("True","Yes") : LOCTEXT("False","No");
	const FText Inherits = FText::Format(LOCTEXT("UMounteaDialogueGraphNode_InheritsTooltip", "Inherits Graph Decorators: {0}"), InheritsValue);

	FText ImplementsNumber;
	if (NodeDecorators.Num() == 0) ImplementsNumber = LOCTEXT("None","-");
	else ImplementsNumber = FText::FromString(FString::FromInt(NodeDecorators.Num()));
	
	const FText Implements = FText::Format(LOCTEXT("UMounteaDialogueGraphNode_ImplementsTooltip", "Implements Decorators: {0}"), ImplementsNumber);
	
	return FText::Format(LOCTEXT("UMounteaDialogueGraphNode_BaseTooltip", "{0}\n\n{1}\n{2}"), InternalName,  Inherits, Implements);
}

#endif

#undef LOCTEXT_NAMESPACE
