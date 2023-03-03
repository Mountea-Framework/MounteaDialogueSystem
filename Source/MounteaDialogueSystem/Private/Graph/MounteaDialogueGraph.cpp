// All rights reserved Dominik Pavlicek 2023

#include "Graph/MounteaDialogueGraph.h"

#include "Edges/MounteaDialogueGraphEdge.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
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

void UMounteaDialogueGraph::Print(bool ToConsole, bool ToScreen)
{

	int Level = 0;
	TArray<UMounteaDialogueGraphNode*> CurrLevelNodes = RootNodes;
	TArray<UMounteaDialogueGraphNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UMounteaDialogueGraphNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			FString Message = FString::Printf(TEXT("%s, Level %d"), *Node->GetDescription().ToString(), Level);

			if (ToConsole)
			{
				LOG_INFO(TEXT("%s"), *Message);
			}

#if WITH_EDITOR
			if (ToScreen && GEngine != nullptr)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Blue, Message);
			}
#endif
			
			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}

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

int UMounteaDialogueGraph::GetLevelNum() const
{
	int Level = 0;
	TArray<UMounteaDialogueGraphNode*> CurrLevelNodes = RootNodes;
	TArray<UMounteaDialogueGraphNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UMounteaDialogueGraphNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}

	return Level;
}

void UMounteaDialogueGraph::GetNodesByLevel(int Level, TArray<UMounteaDialogueGraphNode*>& Nodes)
{
	int CurrLEvel = 0;
	TArray<UMounteaDialogueGraphNode*> NextLevelNodes;

	Nodes = RootNodes;

	while (Nodes.Num() != 0)
	{
		if (CurrLEvel == Level)
			break;

		for (int i = 0; i < Nodes.Num(); ++i)
		{
			UMounteaDialogueGraphNode* Node = Nodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		Nodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++CurrLEvel;
	}
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
		
		//MarkPackageDirty();
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
	auto ParentResult = UObject::IsDataValid(ValidationErrors);
	
	if (ValidateGraph(ValidationErrors, false))
	{
		return EDataValidationResult::Valid;
	}
	else
	{
		return EDataValidationResult::Invalid;
	}
}

#endif

#undef LOCTEXT_NAMESPACE
