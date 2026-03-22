// All rights reserved Dominik Pavlicek 2023


#include "EdGraph_MounteaDialogueGraph.h"

#include "GraphEditAction.h"
#include "EdNode_MounteaDialogueGraphEdge.h"
#include "EdNode_MounteaDialogueGraphNode.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Helpers/MounteaDialogueEditorDetailsTypes.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"
#include "Helpers/MounteaDialogueSystemEditorBFC.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
#include "GraphScheme/AssetGraphScheme_MounteaDialogueGraph.h"

namespace
{
	void BreakAllDirectLinksBetween(UEdGraphPin* OutputPin, UEdGraphPin* InputPin)
	{
		if (!OutputPin || !InputPin)
			return;

		while (OutputPin->LinkedTo.Contains(InputPin))
		{
			OutputPin->BreakLinkTo(InputPin);
		}
	}
}

UEdGraph_MounteaDialogueGraph::UEdGraph_MounteaDialogueGraph()
{
}

UEdGraph_MounteaDialogueGraph::~UEdGraph_MounteaDialogueGraph()
{
}

bool UEdGraph_MounteaDialogueGraph::NormalizeEdgeNodes(bool BMigrateDirectLinks, int32& OutMigratedLinks, int32& OutRemovedDuplicateEdges)
{
	OutMigratedLinks = 0;
	OutRemovedDuplicateEdges = 0;
	bool bWasModified = false;

	TMap<TPair<UEdNode_MounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*>, UEdNode_MounteaDialogueGraphEdge*> uniqueEdges;
	TArray<UEdNode_MounteaDialogueGraphEdge*> duplicateEdges;

	for (UEdGraphNode* graphNode : Nodes)
	{
		UEdNode_MounteaDialogueGraphEdge* edgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(graphNode);
		if (!edgeNode)
			continue;

		UEdNode_MounteaDialogueGraphNode* startNode = edgeNode->GetStartNode();
		UEdNode_MounteaDialogueGraphNode* endNode = edgeNode->GetEndNode();
		if (!startNode || !endNode)
		{
			duplicateEdges.Add(edgeNode);
			continue;
		}

		const TPair<UEdNode_MounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*> edgeKey(startNode, endNode);
		if (uniqueEdges.Contains(edgeKey))
		{
			duplicateEdges.Add(edgeNode);
			continue;
		}

		uniqueEdges.Add(edgeKey, edgeNode);
	}

	for (UEdNode_MounteaDialogueGraphEdge* duplicateEdge : duplicateEdges)
	{
		if (!duplicateEdge)
			continue;

		duplicateEdge->Modify();
		duplicateEdge->BreakAllNodeLinks();
		duplicateEdge->DestroyNode();
		OutRemovedDuplicateEdges++;
		bWasModified = true;
	}

	if (!BMigrateDirectLinks)
		return bWasModified;

	TArray<TPair<UEdNode_MounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*>> directLinks;
	for (UEdGraphNode* graphNode : Nodes)
	{
		UEdNode_MounteaDialogueGraphNode* startNode = Cast<UEdNode_MounteaDialogueGraphNode>(graphNode);
		if (!startNode || !startNode->GetOutputPin())
			continue;

		for (UEdGraphPin* linkedPin : startNode->GetOutputPin()->LinkedTo)
		{
			UEdNode_MounteaDialogueGraphNode* endNode = linkedPin ? Cast<UEdNode_MounteaDialogueGraphNode>(linkedPin->GetOwningNode()) : nullptr;
			if (!endNode)
				continue;

			directLinks.AddUnique(TPair<UEdNode_MounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*>(startNode, endNode));
		}
	}

	const UAssetGraphScheme_MounteaDialogueGraph* graphSchema = Cast<UAssetGraphScheme_MounteaDialogueGraph>(GetSchema());

	for (const TPair<UEdNode_MounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*>& directLink : directLinks)
	{
		UEdNode_MounteaDialogueGraphNode* startNode = directLink.Key;
		UEdNode_MounteaDialogueGraphNode* endNode = directLink.Value;
		if (!startNode || !endNode || !startNode->GetOutputPin() || !endNode->GetInputPin())
			continue;

		const TPair<UEdNode_MounteaDialogueGraphNode*, UEdNode_MounteaDialogueGraphNode*> edgeKey(startNode, endNode);
		const bool bHasEdgeNode = uniqueEdges.Contains(edgeKey);
		const bool bHadDirectLink = startNode->GetOutputPin()->LinkedTo.Contains(endNode->GetInputPin());

		BreakAllDirectLinksBetween(startNode->GetOutputPin(), endNode->GetInputPin());
		if (bHadDirectLink)
			bWasModified = true;

		if (bHasEdgeNode)
			continue;

		bool bCreatedEdge = false;
		if (graphSchema)
			bCreatedEdge = graphSchema->CreateAutomaticConversionNodeAndConnections(startNode->GetOutputPin(), endNode->GetInputPin());

		if (!bCreatedEdge)
		{
			UEdNode_MounteaDialogueGraphEdge* edgeNode = CreateEdgeNode(startNode, endNode);
			if (edgeNode)
			{
				edgeNode->CreateNewGuid();
				edgeNode->NodePosX = (startNode->NodePosX + endNode->NodePosX) * 0.5f;
				edgeNode->NodePosY = (startNode->NodePosY + endNode->NodePosY) * 0.5f;
				edgeNode->SetFlags(RF_Transactional);
				if (edgeNode->MounteaDialogueGraphEdge)
					edgeNode->MounteaDialogueGraphEdge->SetFlags(RF_Transactional);
				AddNode(edgeNode, true, false);
				bCreatedEdge = true;
			}
		}

		if (bCreatedEdge)
		{
			OutMigratedLinks++;
			bWasModified = true;
			uniqueEdges.Add(edgeKey, nullptr);
		}
	}

	return bWasModified;
}

void UEdGraph_MounteaDialogueGraph::RebuildMounteaDialogueGraph()
{
	bSuppressDeltaSync = true;

	int32 migratedLinks = 0;
	int32 removedDuplicateEdges = 0;
	NormalizeEdgeNodes(false, migratedLinks, removedDuplicateEdges);

	if (removedDuplicateEdges > 0)
		UE_LOG(LogTemp, Warning, TEXT("[RebuildMounteaDialogueGraph] Removed %d duplicate edge node(s)."), removedDuplicateEdges);

	UMounteaDialogueGraph* dialogueGraph = GetMounteaDialogueGraph();

	Clear();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UEdNode_MounteaDialogueGraphNode* editorNode = Cast<UEdNode_MounteaDialogueGraphNode>(Nodes[i]))
		{
			if (editorNode->DialogueGraphNode == nullptr)
				continue;

			UMounteaDialogueGraphNode* mounteaDialogueGraphNode = editorNode->DialogueGraphNode;

			NodeMap.Add(mounteaDialogueGraphNode, editorNode);

			dialogueGraph->AllNodes.Add(mounteaDialogueGraphNode);

			const int32 nodeIdx = dialogueGraph->AllNodes.Find(editorNode->DialogueGraphNode);
			editorNode->SetDialogueNodeIndex(nodeIdx);
			editorNode->DialogueGraphNode->SetNodeIndex(nodeIdx);
			editorNode->UpdatePosition();
			
			for (int PinIdx = 0; PinIdx < editorNode->Pins.Num(); ++PinIdx)
			{
				UEdGraphPin* nodePin = editorNode->Pins[PinIdx];

				if (nodePin->Direction != EEdGraphPinDirection::EGPD_Output)
					continue;

				for (int LinkToIdx = 0; LinkToIdx < nodePin->LinkedTo.Num(); ++LinkToIdx)
				{
					UMounteaDialogueGraphNode* ChildNode = nullptr;
					if (UEdNode_MounteaDialogueGraphNode* editorNodeChild = Cast<UEdNode_MounteaDialogueGraphNode>(nodePin->LinkedTo[LinkToIdx]->GetOwningNode()))
						ChildNode = editorNodeChild->DialogueGraphNode;
					else if (UEdNode_MounteaDialogueGraphEdge* editorNodeEdge = Cast<UEdNode_MounteaDialogueGraphEdge>(nodePin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						UEdNode_MounteaDialogueGraphNode* childNode = editorNodeEdge->GetEndNode();;
						if (childNode != nullptr)
							ChildNode = childNode->DialogueGraphNode;
					}

					if (ChildNode != nullptr)
					{
						mounteaDialogueGraphNode->ChildrenNodes.Add(ChildNode);

						ChildNode->ParentNodes.Add(mounteaDialogueGraphNode);
					}
					else
						EditorLOG_ERROR(TEXT("[RebuildMounteaDialogueGraph] Can't find child node"));
				}
			}
		}
		else if (UEdNode_MounteaDialogueGraphEdge* EdgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(Nodes[i]))
		{
			UEdNode_MounteaDialogueGraphNode* startNode = EdgeNode->GetStartNode();
			UEdNode_MounteaDialogueGraphNode* endNode = EdgeNode->GetEndNode();
			UMounteaDialogueGraphEdge* edge = EdgeNode->MounteaDialogueGraphEdge;

			if (startNode == nullptr || endNode == nullptr || edge == nullptr)
			{
				EditorLOG_ERROR(TEXT("[RebuildMounteaDialogueGraph] Add edge failed."));
				continue;
			}

			EdgeMap.Add(edge, EdgeNode);

			edge->Graph = dialogueGraph;
			edge->Rename(nullptr, dialogueGraph, REN_DontCreateRedirectors | REN_DoNotDirty);
			edge->StartNode = startNode->DialogueGraphNode;
			edge->EndNode = endNode->DialogueGraphNode;
			edge->StartNode->Edges.Add(edge->EndNode, edge);
		}
	}
	
	for (int i = 0; i < dialogueGraph->AllNodes.Num(); ++i)
	{
		UMounteaDialogueGraphNode* dialogueNode = dialogueGraph->AllNodes[i];
		if (dialogueNode->ParentNodes.Num() == 0)
			dialogueGraph->RootNodes.Add(dialogueNode);

		dialogueNode->Graph = dialogueGraph;
		dialogueNode->Rename(nullptr, dialogueGraph, REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	dialogueGraph->RootNodes.Sort([&](const UMounteaDialogueGraphNode& L, const UMounteaDialogueGraphNode& R)
	{
		UEdNode_MounteaDialogueGraphNode* EdNode_LNode = NodeMap[&L];
		UEdNode_MounteaDialogueGraphNode* EdNode_RNode = NodeMap[&R];
		return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
	});

	AssignExecutionOrder();

	bSuppressDeltaSync = false;
}

UEdNode_MounteaDialogueGraphEdge* UEdGraph_MounteaDialogueGraph::CreateEdgeNode(UEdNode_MounteaDialogueGraphNode* StartNode, UEdNode_MounteaDialogueGraphNode* EndNode)
{
	UEdNode_MounteaDialogueGraphEdge* edgeNode = NewObject<UEdNode_MounteaDialogueGraphEdge>(this);
	edgeNode->AllocateDefaultPins();
	edgeNode->CreateConnections(StartNode, EndNode);
	edgeNode->MounteaDialogueGraphEdge = NewObject<UMounteaDialogueGraphEdge>(GetMounteaDialogueGraph());
	edgeNode->MounteaDialogueGraphEdge->StartNode = StartNode->DialogueGraphNode;
	edgeNode->MounteaDialogueGraphEdge->EndNode = EndNode->DialogueGraphNode;
	edgeNode->MounteaDialogueGraphEdge->Graph = GetMounteaDialogueGraph();

	return edgeNode;
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
	SyncTopology();

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
	UMounteaDialogueGraph* dialogueGraph = GetMounteaDialogueGraph();
	if (!dialogueGraph) 
		return;

	for (UMounteaDialogueGraphNode* graphNode : dialogueGraph->AllNodes)
	{
		if (graphNode)
			graphNode->ExecutionOrder = INDEX_NONE;
	}
}

UMounteaDialogueGraphNode* UEdGraph_MounteaDialogueGraph::GetParentNode(const UMounteaDialogueGraphNode& Node)
{
	if (Node.ParentNodes.Num() > 0)
		return Node.ParentNodes[0];
	return nullptr;
}

void UEdGraph_MounteaDialogueGraph::UpdateFocusedInstance(const FPIEInstanceData& InstanceId)
{
	FocusedInstance = InstanceId;
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
			
			if (ParentA && ParentB && (ParentA->ExecutionOrder != ParentB->ExecutionOrder))
				return ParentA->ExecutionOrder < ParentB->ExecutionOrder;

			// If we reach here, either both nodes have parents with the same execution order
			// In this case, sort based on X position
			return EdNode_A->NodePosX < EdNode_B->NodePosX;
		});

		for (UMounteaDialogueGraphNode* Node : NodesInLayer)
		{
			if (!Node) 
				continue;
			if (Node->IsA(UMounteaDialogueGraphNode_StartNode::StaticClass())) 
				continue;
			if (GetParentNode(*Node))
				Node->ExecutionOrder = CurrentExecutionOrder++;
			else
				Node->ExecutionOrder = INDEX_NONE;
		}
	}
}

void UEdGraph_MounteaDialogueGraph::AssignNodeToLayer(UMounteaDialogueGraphNode* Node, int32 LayerIndex, TMap<int32, TArray<UMounteaDialogueGraphNode*>>& LayeredNodes)
{
	if (!Node) 
		return;

	LayeredNodes.FindOrAdd(LayerIndex).Add(Node);

	for (UMounteaDialogueGraphNode* ChildNode : Node->ChildrenNodes)
	{
		AssignNodeToLayer(ChildNode, LayerIndex + 1, LayeredNodes);
	}
}

void UEdGraph_MounteaDialogueGraph::SyncTopology()
{
	bSuppressDeltaSync = true;

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

			const int32 nodeIdx = Graph->AllNodes.Find(EdNode->DialogueGraphNode);
			EdNode->SetDialogueNodeIndex(nodeIdx);
			EdNode->DialogueGraphNode->SetNodeIndex(nodeIdx);
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
						ChildNode = EdNode_Child->DialogueGraphNode;
					else if (UEdNode_MounteaDialogueGraphEdge* EdNode_Edge = Cast<UEdNode_MounteaDialogueGraphEdge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						UEdNode_MounteaDialogueGraphNode* Child = EdNode_Edge->GetEndNode();
						if (Child != nullptr)
							ChildNode = Child->DialogueGraphNode;
					}

					if (ChildNode != nullptr)
					{
						MounteaDialogueGraphNode->ChildrenNodes.Add(ChildNode);
						ChildNode->ParentNodes.Add(MounteaDialogueGraphNode);
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
				continue;

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
			Graph->RootNodes.Add(Node);

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

	bSuppressDeltaSync = false;
}

void UEdGraph_MounteaDialogueGraph::NotifyGraphChanged(const FEdGraphEditAction& Action)
{
	Super::NotifyGraphChanged(Action);

	if (bSuppressDeltaSync)
		return;

	if (Action.Action & GRAPHACTION_AddNode)
	{
		for (const UEdGraphNode* node : Action.Nodes)
		{
			UEdNode_MounteaDialogueGraphNode* edNode = const_cast<UEdNode_MounteaDialogueGraphNode*>(Cast<UEdNode_MounteaDialogueGraphNode>(node));
			if (edNode)
				RegisterNode(edNode);
		}
	}
	else if (Action.Action & GRAPHACTION_RemoveNode)
	{
		for (const UEdGraphNode* node : Action.Nodes)
		{
			if (const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(node))
			{
				if (edNode->DialogueGraphNode)
					UnregisterNode(edNode->DialogueGraphNode);
			}
			else if (const UEdNode_MounteaDialogueGraphEdge* edEdge = Cast<UEdNode_MounteaDialogueGraphEdge>(node))
			{
				if (edEdge->MounteaDialogueGraphEdge)
					UnregisterEdge(edEdge->MounteaDialogueGraphEdge);
			}
		}
	}
}

void UEdGraph_MounteaDialogueGraph::RegisterNode(UEdNode_MounteaDialogueGraphNode* EdNode)
{
	if (!EdNode || !EdNode->DialogueGraphNode)
		return;

	UMounteaDialogueGraphNode* node = EdNode->DialogueGraphNode;

	if (NodeMap.Contains(node))
		return;

	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();

	NodeMap.Add(node, EdNode);
	Graph->AllNodes.Add(node);
	const int32 nodeIdx = Graph->AllNodes.Find(node);
	EdNode->SetDialogueNodeIndex(nodeIdx);
	node->SetNodeIndex(nodeIdx);
	EdNode->UpdatePosition();

	node->Graph = Graph;
	node->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);

	if (node->ParentNodes.Num() == 0)
	{
		Graph->RootNodes.AddUnique(node);
		Graph->RootNodes.Sort([&](const UMounteaDialogueGraphNode& L, const UMounteaDialogueGraphNode& R)
		{
			UEdNode_MounteaDialogueGraphNode* const* LEdNode = NodeMap.Find(&L);
			UEdNode_MounteaDialogueGraphNode* const* REdNode = NodeMap.Find(&R);
			if (!LEdNode || !REdNode) return false;
			return (*LEdNode)->NodePosX < (*REdNode)->NodePosX;
		});
	}

	AssignExecutionOrder();
}

void UEdGraph_MounteaDialogueGraph::UnregisterNode(UMounteaDialogueGraphNode* Node)
{
	if (!Node || !NodeMap.Contains(Node))
		return;

	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();

	for (UMounteaDialogueGraphNode* parent : Node->ParentNodes)
	{
		parent->ChildrenNodes.Remove(Node);
		parent->Edges.Remove(Node);
	}

	for (UMounteaDialogueGraphNode* child : Node->ChildrenNodes)
	{
		child->ParentNodes.Remove(Node);
		if (child->ParentNodes.Num() == 0)
			Graph->RootNodes.AddUnique(child);
	}

	for (auto it = EdgeMap.CreateIterator(); it; ++it)
	{
		UMounteaDialogueGraphEdge* edge = it->Key;
		if (edge && (edge->StartNode == Node || edge->EndNode == Node))
			it.RemoveCurrent();
	}

	Node->ParentNodes.Reset();
	Node->ChildrenNodes.Reset();
	Node->Edges.Reset();

	NodeMap.Remove(Node);
	Graph->AllNodes.Remove(Node);
	Graph->RootNodes.Remove(Node);

	Graph->RootNodes.Sort([&](const UMounteaDialogueGraphNode& L, const UMounteaDialogueGraphNode& R)
	{
		UEdNode_MounteaDialogueGraphNode* const* LEdNode = NodeMap.Find(&L);
		UEdNode_MounteaDialogueGraphNode* const* REdNode = NodeMap.Find(&R);
		if (!LEdNode || !REdNode) return false;
		return (*LEdNode)->NodePosX < (*REdNode)->NodePosX;
	});

	AssignExecutionOrder();
}

void UEdGraph_MounteaDialogueGraph::RegisterEdge(UEdNode_MounteaDialogueGraphEdge* EdEdge)
{
	if (!EdEdge || !EdEdge->MounteaDialogueGraphEdge)
		return;

	UEdNode_MounteaDialogueGraphNode* startEdNode = EdEdge->GetStartNode();
	UEdNode_MounteaDialogueGraphNode* endEdNode = EdEdge->GetEndNode();

	if (!startEdNode || !endEdNode)
		return;

	UMounteaDialogueGraphEdge* Edge = EdEdge->MounteaDialogueGraphEdge;

	if (EdgeMap.Contains(Edge))
		return;

	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();

	EdgeMap.Add(Edge, EdEdge);

	Edge->Graph = Graph;
	Edge->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
	Edge->StartNode = startEdNode->DialogueGraphNode;
	Edge->EndNode = endEdNode->DialogueGraphNode;
	Edge->StartNode->Edges.Add(Edge->EndNode, Edge);

	Edge->StartNode->ChildrenNodes.AddUnique(Edge->EndNode);
	Edge->EndNode->ParentNodes.AddUnique(Edge->StartNode);

	Graph->RootNodes.Remove(Edge->EndNode);

	AssignExecutionOrder();
}

void UEdGraph_MounteaDialogueGraph::UnregisterEdge(UMounteaDialogueGraphEdge* Edge)
{
	if (!Edge || !EdgeMap.Contains(Edge))
		return;

	if (!Edge->StartNode || !Edge->EndNode)
	{
		EdgeMap.Remove(Edge);
		return;
	}

	UMounteaDialogueGraph* Graph = GetMounteaDialogueGraph();

	Edge->StartNode->ChildrenNodes.Remove(Edge->EndNode);
	Edge->EndNode->ParentNodes.Remove(Edge->StartNode);
	Edge->StartNode->Edges.Remove(Edge->EndNode);

	if (Edge->EndNode->ParentNodes.Num() == 0)
	{
		Graph->RootNodes.AddUnique(Edge->EndNode);
		Graph->RootNodes.Sort([&](const UMounteaDialogueGraphNode& L, const UMounteaDialogueGraphNode& R)
		{
			UEdNode_MounteaDialogueGraphNode* const* LEdNode = NodeMap.Find(&L);
			UEdNode_MounteaDialogueGraphNode* const* REdNode = NodeMap.Find(&R);
			if (!LEdNode || !REdNode) 
				return false;
			return (*LEdNode)->NodePosX < (*REdNode)->NodePosX;
		});
	}

	EdgeMap.Remove(Edge);

	AssignExecutionOrder();
}
