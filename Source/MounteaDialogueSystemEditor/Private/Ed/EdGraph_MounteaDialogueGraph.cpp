// All rights reserved Dominik Pavlicek 2023


#include "EdGraph_MounteaDialogueGraph.h"

#include "EdNode_MounteaDialogueGraphEdge.h"
#include "EdNode_MounteaDialogueGraphNode.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"

UEdGraph_MounteaDialogueGraph::UEdGraph_MounteaDialogueGraph()
{
}

UEdGraph_MounteaDialogueGraph::~UEdGraph_MounteaDialogueGraph()
{
	AssetEditor = nullptr;
}

void UEdGraph_MounteaDialogueGraph::RebuildMounteaDialogueGraph()
{
	EditorLOG_INFO(TEXT("UMounteaDialogueGraphEdGraph::RebuildMounteaDialogueGraph has been called"));

	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();

	Clear();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UEdNode_MounteaDialogueGraphNode* EdNode = Cast<UEdNode_MounteaDialogueGraphNode>(Nodes[i]))
		{
			if (EdNode->DialogueGraphNode == nullptr)
				continue;

			UMounteaDialogueGraphNode* MounteaDialogueGraphNode = EdNode->DialogueGraphNode;

			NodeMap.Add(MounteaDialogueGraphNode, EdNode);

			Graph->AllNodes.Add(MounteaDialogueGraphNode);

			EdNode->SetDialogueNodeIndex( Graph->AllNodes.Find(EdNode->DialogueGraphNode) );

			for (int PinIdx = 0; PinIdx < EdNode->Pins.Num(); ++PinIdx)
			{
				UEdGraphPin* Pin = EdNode->Pins[PinIdx];

				if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
					continue;

				for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
				{
					UMounteaDialogueGraphNode* ChildNode = nullptr;
					if (UEdNode_MounteaDialogueGraphNode* EdNode_Child = Cast<UEdNode_MounteaDialogueGraphNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						ChildNode = EdNode_Child->DialogueGraphNode;
					}
					else if (UEdNode_MounteaDialogueGraphEdge* EdNode_Edge = Cast<UEdNode_MounteaDialogueGraphEdge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						UEdNode_MounteaDialogueGraphNode* Child = EdNode_Edge->GetEndNode();;
						if (Child != nullptr)
						{
							ChildNode = Child->DialogueGraphNode;
						}
					}

					if (ChildNode != nullptr)
					{
						MounteaDialogueGraphNode->ChildrenNodes.Add(ChildNode);

						ChildNode->ParentNodes.Add(MounteaDialogueGraphNode);
					}
					else
					{
						EditorLOG_ERROR(TEXT("[RebuildMounteaDialogueGraph] Can't find child node"));
					}
				}
			}
		}
		else if (UEdNode_MounteaDialogueGraphEdge* EdgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(Nodes[i]))
		{
			UEdNode_MounteaDialogueGraphNode* StartNode = EdgeNode->GetStartNode();
			UEdNode_MounteaDialogueGraphNode* EndNode = EdgeNode->GetEndNode();
			UMounteaDialogueGraphEdge* Edge = EdgeNode->MounteaDialogueGraphEdge;

			if (StartNode == nullptr || EndNode == nullptr || Edge == nullptr)
			{
				EditorLOG_ERROR(TEXT("[RebuildMounteaDialogueGraph] Add edge failed."));
				continue;
			}

			EdgeMap.Add(Edge, EdgeNode);

			Edge->Graph = Graph;
			Edge->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
			Edge->StartNode = StartNode->DialogueGraphNode;
			Edge->EndNode = EndNode->DialogueGraphNode;
			Edge->StartNode->Edges.Add(Edge->EndNode, Edge);
		}
	}

	for (int i = 0; i < Graph->AllNodes.Num(); ++i)
	{
		UMounteaDialogueGraphNode* Node = Graph->AllNodes[i];
		if (Node->ParentNodes.Num() == 0)
		{
			Graph->RootNodes.Add(Node);

			SortNodes(Node);
		}

		Node->Graph = Graph;
		Node->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	Graph->RootNodes.Sort([&](const UMounteaDialogueGraphNode& L, const UMounteaDialogueGraphNode& R)
	{
		UEdNode_MounteaDialogueGraphNode* EdNode_LNode = NodeMap[&L];
		UEdNode_MounteaDialogueGraphNode* EdNode_RNode = NodeMap[&R];
		return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
	});
}

UMounteaDialogueGraph* UEdGraph_MounteaDialogueGraph::GetMounteaDialogueGraph() const
{
	return CastChecked<UMounteaDialogueGraph>(GetOuter());
}

bool UEdGraph_MounteaDialogueGraph::Modify(bool bAlwaysMarkDirty)
{
	bool Rtn = Super::Modify(bAlwaysMarkDirty);

	GetMounteaDialogueGraph()->Modify();

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Nodes[i]->Modify();
	}

	return Rtn;
}

void UEdGraph_MounteaDialogueGraph::PostEditUndo()
{
	Super::PostEditUndo();

	NotifyGraphChanged();
}

void UEdGraph_MounteaDialogueGraph::SetAssetEditor(FAssetEditor_MounteaDialogueGraph* NewAssetEditor)
{
	AssetEditor = MakeShareable(NewAssetEditor);
}

void UEdGraph_MounteaDialogueGraph::Clear()
{
	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();

	Graph->ClearGraph();
	NodeMap.Reset();
	EdgeMap.Reset();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UEdNode_MounteaDialogueGraphNode* EdNode = Cast<UEdNode_MounteaDialogueGraphNode>(Nodes[i]))
		{
			UMounteaDialogueGraphNode* MounteaDialogueGraphNode = EdNode->DialogueGraphNode;
			MounteaDialogueGraphNode->ParentNodes.Reset();
			MounteaDialogueGraphNode->ChildrenNodes.Reset();
			MounteaDialogueGraphNode->Edges.Reset();
		}
	}
}

void UEdGraph_MounteaDialogueGraph::SortNodes(UMounteaDialogueGraphNode* RootNode)
{
	int Level = 0;
	TArray<UMounteaDialogueGraphNode*> CurrLevelNodes = { RootNode };
	TArray<UMounteaDialogueGraphNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		int32 LevelWidth = 0;
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UMounteaDialogueGraphNode* Node = CurrLevelNodes[i];

			auto Comp = [&](const UMounteaDialogueGraphNode& L, const UMounteaDialogueGraphNode& R)
			{
				UEdNode_MounteaDialogueGraphNode* EdNode_LNode = NodeMap[&L];
				UEdNode_MounteaDialogueGraphNode* EdNode_RNode = NodeMap[&R];
				return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
			};

			Node->ChildrenNodes.Sort(Comp);
			Node->ParentNodes.Sort(Comp);

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
