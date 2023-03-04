// All rights reserved Dominik Pavlicek 2023

#include "Graph/MounteaDialogueGraph.h"

#include "Edges/MounteaDialogueGraphEdge.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraph"

UMounteaDialogueGraph::UMounteaDialogueGraph()
{
	NodeType = UMounteaDialogueGraphNode::StaticClass();
	EdgeType = UMounteaDialogueGraphEdge::StaticClass();

	bEdgeEnabled = false;
	GraphGUID = FGuid::NewGuid();

#if WITH_EDITORONLY_DATA
	EdGraph = nullptr;

	bCanRenameNode = true;
#endif
}

FGuid UMounteaDialogueGraph::GetGraphGUID() const
{
	return GraphGUID;
}

TArray<UMounteaDialogueGraphNode*> UMounteaDialogueGraph::GetAllNodes() const
{
	return AllNodes;
}

TArray<UMounteaDialogueGraphNode*> UMounteaDialogueGraph::GetRootNodes() const
{
	return RootNodes;
}

UMounteaDialogueGraphNode* UMounteaDialogueGraph::GetStartNode() const
{
	return StartNode;
}

TArray<FMounteaDialogueDecorator> UMounteaDialogueGraph::GetGraphDecorators() const
{
	return GraphDecorators;
}

void UMounteaDialogueGraph::CreateGraph()
{
#if WITH_EDITOR
	// We already have existing Graph
	if (EdGraph != nullptr)
	{
		return;
	}

	// We already have existing Start Node
	if (StartNode != nullptr)
	{
		return;
	}

	StartNode = ConstructDialogueNode<UMounteaDialogueGraphNode_StartNode>();
	if (StartNode != nullptr )
	{
		StartNode->Graph = this;

		RootNodes.Add(StartNode);
		AllNodes.Add(StartNode);
	}
#endif
}

void UMounteaDialogueGraph::ClearGraph()
{
	for (int i = 0; i < AllNodes.Num(); ++i)
	{
		UMounteaDialogueGraphNode* Node = AllNodes[i];

		Node->ParentNodes.Empty();
		Node->ChildrenNodes.Empty();
		Node->Edges.Empty();
	}

	AllNodes.Empty();
	RootNodes.Empty();
}

void UMounteaDialogueGraph::PostInitProperties()
{
	UObject::PostInitProperties();

	// Ignore these cases
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		return;
	}

#if WITH_EDITOR

	CreateGraph();
	
#endif
}

#if WITH_EDITOR

bool UMounteaDialogueGraph::ValidateGraph(TArray<FText>& ValidationErrors, bool RichTextFormat)
{
	bool bReturnValue = true;

	// DECORATORS VALIDATION
	{
		TArray<UMounteaDialogueGraphNodeDecoratorBase*> UsedNodeDecorators;
		for (int i = 0; i < GraphDecorators.Num(); i++)
		{
			if (GraphDecorators.IsValidIndex(i) && GraphDecorators[i].DecoratorType)
			{
				UsedNodeDecorators.Add(GraphDecorators[i].DecoratorType);
			}
			else
			{
				const FString RichTextReturn =
				FString("* ").
				Append( TEXT("<RichTextBlock.Bold>Dialogue Graph</>")).
				Append(": has ").
				Append(TEXT("<RichTextBlock.Bold>invalid</> Node Decorator at Index: ")).
				Append(FString::FromInt(i )).
				Append(".");

				const FString TextReturn =
				GetName().
				Append(": has ").
				Append(TEXT("INVALID Node Decorator at Index: ")).
				Append(FString::FromInt(i )).
				Append(".");
		
				ValidationErrors.Add(FText::FromString(RichTextFormat ? RichTextReturn : TextReturn));

				bReturnValue = false;
			}
		}
		
		TMap<UClass*, int32> DuplicatedDecoratorsMap;
		for (const auto Itr : UsedNodeDecorators)
		{
			int32 ClassAppearance = 1;
			for (const auto Itr2 : UsedNodeDecorators)
			{
				if (Itr != Itr2 && Itr->StaticClass() == Itr2->StaticClass())
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
				bReturnValue = false;
			
				const FString RichTextReturn =
				FString("* ").
				Append(TEXT("<RichTextBlock.Bold>Dialogue Graph</>")).
				Append(": has Node Decorator ").
				Append("<RichTextBlock.Bold>").
				Append(Itr.Key->GetName().LeftChop(2)).
				Append("</> ").
				Append(FString::FromInt(Itr.Value)).
				Append("x times! Please, avoid duplicates!");
				
				const FString TextReturn =
				FString(TEXT("Dialogue Graph: has Node Decorator ")).
				Append( Itr.Key->GetName().LeftChop(2)).
				Append(" ").
				Append(FString::FromInt(Itr.Value)).
				Append("x times! Please, avoid duplicates!");
			
				ValidationErrors.Add(FText::FromString(RichTextFormat ? RichTextReturn : TextReturn));
			}
		}
	}
	
	if (StartNode == nullptr)
	{
		const FString RichTextReturn =
		FString("* ").
		Append(TEXT("<RichTextBlock.Bold>Dialogue Graph</>")).
		Append(": Has no Start Node!");

		const FString TextReturn =
		GetName().
		Append(": Has no Start Node!");
		
		ValidationErrors.Add(FText::FromString(RichTextFormat ? RichTextReturn : TextReturn));

		bReturnValue = false;
	}

	for (UMounteaDialogueGraphNode* Itr : AllNodes)
	{
		if (Itr != nullptr && (Itr->ValidateNode(ValidationErrors, RichTextFormat) == false))
		{
			bReturnValue = false;
		}
	}
	
	return bReturnValue;
}

EDataValidationResult UMounteaDialogueGraph::IsDataValid(TArray<FText>& ValidationErrors)
{
	if (ValidateGraph(ValidationErrors, false))
	{
		return EDataValidationResult::Valid;
	}
	
	return EDataValidationResult::Invalid;
}

#endif

#undef LOCTEXT_NAMESPACE
