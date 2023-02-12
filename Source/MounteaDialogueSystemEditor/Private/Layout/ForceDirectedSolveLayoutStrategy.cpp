// All rights reserved Dominik Pavlicek 2023


#include "ForceDirectedSolveLayoutStrategy.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

UForceDirectedSolveLayoutStrategy::UForceDirectedSolveLayoutStrategy()
{
	bRandomInit = false;
	CoolDownRate = 10;
	InitTemperature = 10.f;
}

static inline float CoolDown(float Temp, float CoolDownRate)
{
	if (Temp < .01) return .01;
	return Temp - (Temp / CoolDownRate);
}

static inline float GetAttractForce(float X, float K)
{
	return (X * X) / K;
}

static inline float GetRepulseForce(float X, float k)
{
	return X != 0 ? k * k / X : TNumericLimits<float>::Max();
}

void UForceDirectedSolveLayoutStrategy::Layout(UEdGraph* InEdGraph)
{
	EdGraph = Cast<UEdGraph_MounteaDialogueGraph>(InEdGraph);
	check(EdGraph != nullptr);

	EdGraph->RebuildMounteaDialogueGraph();
	Graph = EdGraph->GetMounteaDialogueGraph();
	check(Graph != nullptr);

	if (Settings != nullptr)
	{
		OptimalDistance = Settings->GetOptimalDistance();
		MaxIteration = Settings->GetMaxIteration();
		bRandomInit = Settings->IsRandomInit();
	}

	FBox2D PreTreeBound(ForceInitToZero);
	for (int32 i = 0; i < Graph->RootNodes.Num(); ++i)
	{
		PreTreeBound = LayoutOneTree(Graph->RootNodes[i], PreTreeBound);
	}
}

FBox2D UForceDirectedSolveLayoutStrategy::LayoutOneTree(UMounteaDialogueGraphNode* RootNode, const FBox2D& PreTreeBound)
{
	float Temp = InitTemperature;
	FBox2D TreeBound = GetActualBounds(RootNode);
	TreeBound.Min.X += PreTreeBound.Max.X + OptimalDistance;
	TreeBound.Max.X += PreTreeBound.Max.X + OptimalDistance;

	if (bRandomInit)
	{
		RandomLayoutOneTree(RootNode, TreeBound);
	}

	float RepulseForce, AttractForce, Distance;
	FVector2D Diff;

	TMap<UEdGraphNode*, FVector2D> NodeToDisplacement;

	for (int32 i = 0; i < EdGraph->Nodes.Num(); ++i)
	{
		NodeToDisplacement.Add(EdGraph->Nodes[i], FVector2D(0.f, 0.f));
	}

	for (int32 IterrationNum = 0; IterrationNum < MaxIteration; ++IterrationNum)
	{
		// Calculate the repulsive forces.
		for (int32 i = 0; i < EdGraph->Nodes.Num(); ++i)
		{
			for (int32 j = 0; j < EdGraph->Nodes.Num(); ++j)
			{
				if (i == j)
					continue;
				Diff.X = EdGraph->Nodes[i]->NodePosX - EdGraph->Nodes[j]->NodePosX;
				Diff.Y = EdGraph->Nodes[i]->NodePosY - EdGraph->Nodes[j]->NodePosY;
				Distance = Diff.Size();
				Diff.Normalize();

				RepulseForce = Distance > 2 * OptimalDistance ? 0 : GetRepulseForce(Distance, OptimalDistance);

				NodeToDisplacement[EdGraph->Nodes[i]] += Diff * RepulseForce;
			}
		}

		// Calculate the attractive forces.
		int Level = 0;
		TArray<UMounteaDialogueGraphNode*> CurrLevelNodes = { RootNode };
		TArray<UMounteaDialogueGraphNode*> NextLevelNodes;

		while (CurrLevelNodes.Num() != 0)
		{
			for (int32 i = 0; i < CurrLevelNodes.Num(); ++i)
			{
				UMounteaDialogueGraphNode* Node = CurrLevelNodes[i];
				check(Node != nullptr);

				UEdNode_MounteaDialogueGraphNode* EdNode_ParentNode = EdGraph->NodeMap[Node];

				for (int32 j = 0; j < Node->ChildrenNodes.Num(); ++j)
				{
					NextLevelNodes.Add(Node->ChildrenNodes[j]);

					UEdNode_MounteaDialogueGraphNode* EdNode_ChildNode = EdGraph->NodeMap[Node->ChildrenNodes[j]];

					Diff.X = EdNode_ChildNode->NodePosX - EdNode_ParentNode->NodePosY;
					Diff.Y = EdNode_ChildNode->NodePosY - EdNode_ParentNode->NodePosY;
					Distance = Diff.Size();
					Diff.Normalize();

					AttractForce = GetAttractForce(Distance, OptimalDistance);

					NodeToDisplacement[EdNode_ParentNode] += Distance * Diff;
					NodeToDisplacement[EdNode_ChildNode] -= Distance * Diff;
				}
			}

			CurrLevelNodes = NextLevelNodes;
			NextLevelNodes.Reset();
			++Level;
		}

		for (int32 i = 0; i < EdGraph->Nodes.Num(); ++i)
		{
			UEdGraphNode* EdNode = EdGraph->Nodes[i];
			Distance = NodeToDisplacement[EdNode].Size();
			NodeToDisplacement[EdNode].Normalize();

			float Minimum = Distance < Temp ? Distance : Temp;
			EdNode->NodePosX += NodeToDisplacement[EdNode].X * Minimum;
			EdNode->NodePosY += NodeToDisplacement[EdNode].Y * Minimum;
		}

		Temp = CoolDown(Temp, CoolDownRate);
	}

	FBox2D ActualBound = GetActualBounds(RootNode);

	FVector2D Center = ActualBound.GetCenter();
	FVector2D TreeCenter = TreeBound.GetCenter();

	FVector2D Scale = (TreeBound.Max - TreeBound.Min) / (ActualBound.Max - ActualBound.Min);

	for (int32 i = 0; i < EdGraph->Nodes.Num(); ++i)
	{
		UEdGraphNode* EdNode = EdGraph->Nodes[i];
		EdNode->NodePosX = TreeCenter.X + Scale.X * (EdNode->NodePosX - Center.X);
		EdNode->NodePosY = TreeCenter.Y + Scale.Y * (EdNode->NodePosY - Center.Y);
	}

	return TreeBound;
}
