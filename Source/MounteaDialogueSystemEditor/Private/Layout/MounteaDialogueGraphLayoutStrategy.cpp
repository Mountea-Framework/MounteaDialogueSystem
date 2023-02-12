// All rights reserved Dominik Pavlicek 2023


#include "MounteaDialogueGraphLayoutStrategy.h"

#include "Nodes/MounteaDialogueGraphNode.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Ed/SEdNode_MounteaDialogueGraphNode.h"

#include "Kismet/KismetMathLibrary.h"

UMounteaDialogueGraphLayoutStrategy::UMounteaDialogueGraphLayoutStrategy()
{
	Settings = nullptr;
	MaxIteration = 50;
	OptimalDistance = 150;
}

UMounteaDialogueGraphLayoutStrategy::~UMounteaDialogueGraphLayoutStrategy()
{
}

int32 UMounteaDialogueGraphLayoutStrategy::GetNodeWidth(UEdNode_MounteaDialogueGraphNode* EdNode)
{
	return EdNode->SEdNode->GetCachedGeometry().GetLocalSize().X;
}

int32 UMounteaDialogueGraphLayoutStrategy::GetNodeHeight(UEdNode_MounteaDialogueGraphNode* EdNode)
{
	return EdNode->SEdNode->GetCachedGeometry().GetLocalSize().Y;
}

FBox2D UMounteaDialogueGraphLayoutStrategy::GetNodeBound(UEdGraphNode* EdNode)
{
	int32 NodeWidth = GetNodeWidth(Cast<UEdNode_MounteaDialogueGraphNode>(EdNode));
	int32 NodeHeight = GetNodeHeight(Cast<UEdNode_MounteaDialogueGraphNode>(EdNode));
	FVector2D Min(EdNode->NodePosX, EdNode->NodePosY);
	FVector2D Max(EdNode->NodePosX + NodeWidth, EdNode->NodePosY + NodeHeight);
	return FBox2D(Min, Max);
}

FBox2D UMounteaDialogueGraphLayoutStrategy::GetActualBounds(UMounteaDialogueGraphNode* RootNode)
{
	int Level = 0;
	TArray<UMounteaDialogueGraphNode*> CurrLevelNodes = { RootNode };
	TArray<UMounteaDialogueGraphNode*> NextLevelNodes;

	FBox2D Rtn = GetNodeBound(EdGraph->NodeMap[RootNode]);

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UMounteaDialogueGraphNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			Rtn += GetNodeBound(EdGraph->NodeMap[Node]);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
	return Rtn;
}

void UMounteaDialogueGraphLayoutStrategy::RandomLayoutOneTree(UMounteaDialogueGraphNode* RootNode, const FBox2D& Bound)
{
	int Level = 0;
	TArray<UMounteaDialogueGraphNode*> CurrLevelNodes = { RootNode };
	TArray<UMounteaDialogueGraphNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UMounteaDialogueGraphNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			UEdNode_MounteaDialogueGraphNode* EdNode_Node = EdGraph->NodeMap[Node];

			EdNode_Node->NodePosX = UKismetMathLibrary::RandomFloatInRange(Bound.Min.X, Bound.Max.X);
			EdNode_Node->NodePosY = UKismetMathLibrary::RandomFloatInRange(Bound.Min.Y, Bound.Max.Y);

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
