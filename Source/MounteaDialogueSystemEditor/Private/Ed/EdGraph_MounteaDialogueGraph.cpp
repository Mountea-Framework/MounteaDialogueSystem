// All rights reserved Dominik Pavlicek 2023


#include "EdGraph_MounteaDialogueGraph.h"

#include "EdNode_MounteaDialogueGraphEdge.h"
#include "EdNode_MounteaDialogueGraphNode.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"
#include "Helpers/MounteaDialogueSystemEditorBFC.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

UEdGraph_MounteaDialogueGraph::UEdGraph_MounteaDialogueGraph()
{
}

UEdGraph_MounteaDialogueGraph::~UEdGraph_MounteaDialogueGraph()
{
}

void UEdGraph_MounteaDialogueGraph::RebuildMounteaDialogueGraph()
{
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
			EdNode->UpdatePosition();
			
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

			//SortNodes(Node);
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

	AssignExecutionOrder();
}

UEdNode_MounteaDialogueGraphEdge* UEdGraph_MounteaDialogueGraph::CreateEdgeNode(UEdNode_MounteaDialogueGraphNode* StartNode, UEdNode_MounteaDialogueGraphNode* EndNode)
{
	UEdNode_MounteaDialogueGraphEdge* EdgeNode = NewObject<UEdNode_MounteaDialogueGraphEdge>(this);
	EdgeNode->AllocateDefaultPins();
	EdgeNode->CreateConnections(StartNode, EndNode);
	EdgeNode->MounteaDialogueGraphEdge = NewObject<UMounteaDialogueGraphEdge>(GetMounteaDialogueGraph());
	EdgeNode->MounteaDialogueGraphEdge->StartNode = StartNode->DialogueGraphNode;
	EdgeNode->MounteaDialogueGraphEdge->EndNode = EndNode->DialogueGraphNode;
	EdgeNode->MounteaDialogueGraphEdge->Graph = GetMounteaDialogueGraph();

	return EdgeNode;
}

UMounteaDialogueGraph* UEdGraph_MounteaDialogueGraph::GetMounteaDialogueGraph() const
{
	return CastChecked<UMounteaDialogueGraph>(GetOuter());
}

bool UEdGraph_MounteaDialogueGraph::Modify(bool bAlwaysMarkDirty)
{
	bool Rtn = Super::Modify(bAlwaysMarkDirty);

	ResetExecutionOrders();
	AssignExecutionOrder();
	
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Nodes[i]->Modify();
	}
	return Rtn;
}

void UEdGraph_MounteaDialogueGraph::PostEditUndo()
{
	NotifyGraphChanged();

	Super::PostEditUndo();
}

void UEdGraph_MounteaDialogueGraph::SetDialogueEditorPtr(TWeakPtr<FAssetEditor_MounteaDialogueGraph> NewPtr)
{
	DialogueEditorPtr = NewPtr;
}

bool UEdGraph_MounteaDialogueGraph::JumpToNode(const UMounteaDialogueGraphNode* Node)
{
	return FMounteaDialogueGraphEditorUtilities::OpenEditorAndJumpToGraphNode(DialogueEditorPtr, *NodeMap.Find(Node));
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
	TArray<UMounteaDialogueGraphNode*> CurrLevelNodes = {RootNode};
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

void UEdGraph_MounteaDialogueGraph::ResetExecutionOrders() const
{
	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();
	if (!Graph) return;

	for (UMounteaDialogueGraphNode* Node : Graph->AllNodes)
	{
		if (Node)
		{
			Node->ExecutionOrder = INDEX_NONE;
		}
	}
}

UMounteaDialogueGraphNode* UEdGraph_MounteaDialogueGraph::GetParentNode(const UMounteaDialogueGraphNode& Node)
{
	if (Node.ParentNodes.Num() > 0)
	{
		return Node.ParentNodes[0];
	}
	return nullptr;
}

void UEdGraph_MounteaDialogueGraph::AssignExecutionOrder()
{
	ResetExecutionOrders();

	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();
	if (!Graph) return;

	TMap<int32, TArray<UMounteaDialogueGraphNode*>> LayeredNodes;
	int32 CurrentExecutionOrder = 1;
	
	if (UMounteaDialogueGraphNode* RootNode = Graph->GetStartNode())
	{
		RootNode->ExecutionOrder = 0;
		AssignNodeToLayer(RootNode, 0, LayeredNodes);
	}

	for (int32 LayerIndex = 0; LayeredNodes.Contains(LayerIndex); ++LayerIndex)
	{
		TArray<UMounteaDialogueGraphNode*>& NodesInLayer = LayeredNodes[LayerIndex];
		NodesInLayer.Sort([this](const UMounteaDialogueGraphNode& A, const UMounteaDialogueGraphNode& B)
		{
			if (!NodeMap.Contains(&A)) return false;
			if (!NodeMap.Contains(&B)) return false;
			
			UEdNode_MounteaDialogueGraphNode* EdNode_A = NodeMap.FindRef(&A);
			UEdNode_MounteaDialogueGraphNode* EdNode_B = NodeMap.FindRef(&B);

			UMounteaDialogueGraphNode* ParentA = GetParentNode(A);
			UMounteaDialogueGraphNode* ParentB = GetParentNode(B);

			if (!ParentA && !ParentB) return EdNode_A->NodePosX < EdNode_B->NodePosX;
			if (ParentA && !ParentB) return true;
			if (!ParentA && ParentB) return false;
			
			if (ParentA && ParentB)
			{
				if (ParentA->ExecutionOrder != ParentB->ExecutionOrder)
				{
					return ParentA->ExecutionOrder < ParentB->ExecutionOrder;
				}
			}

			// If we reach here, either both nodes have parents with the same execution order
			// In this case, sort based on X position
			return EdNode_A->NodePosX < EdNode_B->NodePosX;
		});

		for (UMounteaDialogueGraphNode* Node : NodesInLayer)
		{
			if (!Node) continue;
			if (Node->IsA(UMounteaDialogueGraphNode_StartNode::StaticClass())) continue;
			if (GetParentNode(*Node))
			{
				Node->ExecutionOrder = CurrentExecutionOrder++;
			}
			else
			{
				Node->ExecutionOrder = INDEX_NONE;
			}
		}
	}
}

void UEdGraph_MounteaDialogueGraph::AssignNodeToLayer(UMounteaDialogueGraphNode* Node, int32 LayerIndex, TMap<int32, TArray<UMounteaDialogueGraphNode*>>& LayeredNodes)
{
	if (!Node) return;

	LayeredNodes.FindOrAdd(LayerIndex).Add(Node);

	for (UMounteaDialogueGraphNode* ChildNode : Node->ChildrenNodes)
	{
		AssignNodeToLayer(ChildNode, LayerIndex + 1, LayeredNodes);
	}
}