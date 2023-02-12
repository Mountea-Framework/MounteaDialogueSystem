// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "AssetGraphScheme_MounteaDialogueGraph.h"

#include "FConnectionDrawingPolicy_MounteaDialogueGraph.h"
#include "GraphEditorActions.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "ToolMenu.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Framework/Commands/GenericCommands.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "AssetGraphScheme_MounteaDialogueGraph"

int32 UAssetGraphScheme_MounteaDialogueGraph::CurrentCacheRefreshID = 0;

class FNodeVisitorCycleChecker
{
public:
	/** Check whether a loop in the graph would be caused by linking the passed-in nodes */
	bool CheckForLoop(UEdGraphNode* StartNode, UEdGraphNode* EndNode)
	{
		VisitedNodes.Add(StartNode);

		return TraverseInputNodesToRoot(EndNode);
	}

private:
	bool TraverseInputNodesToRoot(UEdGraphNode* Node)
	{
		VisitedNodes.Add(Node);

		for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* MyPin = Node->Pins[PinIndex];

			if (MyPin->Direction == EGPD_Output)
			{
				for (int32 LinkedPinIndex = 0; LinkedPinIndex < MyPin->LinkedTo.Num(); ++LinkedPinIndex)
				{
					UEdGraphPin* OtherPin = MyPin->LinkedTo[LinkedPinIndex];
					if (OtherPin)
					{
						UEdGraphNode* OtherNode = OtherPin->GetOwningNode();
						if (VisitedNodes.Contains(OtherNode))
						{
							return false;
						}
						else
						{
							return TraverseInputNodesToRoot(OtherNode);
						}
					}
				}
			}
		}

		return true;
	}

	TSet<UEdGraphNode*> VisitedNodes;
};

UEdGraphNode* FAssetSchemaAction_MounteaDialogueGraph_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = nullptr;

	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("MounteaDialogueGraphEditorNewNode", "Mountea Dialogue Graph Editor: New Node"));
		ParentGraph->Modify();
		if (FromPin != nullptr)
			FromPin->Modify();

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		NodeTemplate->DialogueGraphNode->SetFlags(RF_Transactional);
		NodeTemplate->SetFlags(RF_Transactional);

		ResultNode = NodeTemplate;
	}

	return ResultNode;
}

void FAssetSchemaAction_MounteaDialogueGraph_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}

UEdGraphNode* FAssetSchemaAction_MounteaDialogueGraph_NewEdge::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = nullptr;

	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("MounteaDialogueGraphEditorNewEdge", "Mountea Dialogue Graph Editor: New Edge"));
		ParentGraph->Modify();
		if (FromPin != nullptr)
			FromPin->Modify();

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		NodeTemplate->MounteaDialogueGraphEdge->SetFlags(RF_Transactional);
		NodeTemplate->SetFlags(RF_Transactional);

		ResultNode = NodeTemplate;
	}
	
	return ResultNode;
}

void FAssetSchemaAction_MounteaDialogueGraph_NewEdge::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}

void UAssetGraphScheme_MounteaDialogueGraph::GetBreakLinkToSubMenuActions(UToolMenu* Menu, UEdGraphPin* InGraphPin)
{
	// Make sure we have a unique name for every entry in the list
	TMap< FString, uint32 > LinkTitleCount;

	FToolMenuSection& Section = Menu->FindOrAddSection("MounteaDialogueGraphAssetGraphSchemaPinActions");

	// Add all the links we could break from
	for (TArray<class UEdGraphPin*>::TConstIterator Links(InGraphPin->LinkedTo); Links; ++Links)
	{
		UEdGraphPin* Pin = *Links;
		FString TitleString = Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString();
		FText Title = FText::FromString(TitleString);
		if (Pin->PinName != TEXT(""))
		{
			TitleString = FString::Printf(TEXT("%s (%s)"), *TitleString, *Pin->PinName.ToString());

			// Add name of connection if possible
			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeTitle"), Title);
			Args.Add(TEXT("PinName"), Pin->GetDisplayName());
			Title = FText::Format(LOCTEXT("BreakDescPin", "{NodeTitle} ({PinName})"), Args);
		}

		uint32& Count = LinkTitleCount.FindOrAdd(TitleString);

		FText Description;
		FFormatNamedArguments Args;
		Args.Add(TEXT("NodeTitle"), Title);
		Args.Add(TEXT("NumberOfNodes"), Count);

		if (Count == 0)
		{
			Description = FText::Format(LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args);
		}
		else
		{
			Description = FText::Format(LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args);
		}
		++Count;

		Section.AddMenuEntry(NAME_None, Description, Description, FSlateIcon(), FUIAction(
			FExecuteAction::CreateUObject(this, &UAssetGraphScheme_MounteaDialogueGraph::BreakSinglePinLink, const_cast<UEdGraphPin*>(InGraphPin), *Links)));
	}
}

EGraphType UAssetGraphScheme_MounteaDialogueGraph::GetGraphType(const UEdGraph* TestEdGraph) const
{
	return GT_StateMachine;
}

void UAssetGraphScheme_MounteaDialogueGraph::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UMounteaDialogueGraph* Graph = CastChecked<UMounteaDialogueGraph>(ContextMenuBuilder.CurrentGraph->GetOuter());

	if (Graph->NodeType == nullptr)
	{
		return;
	}

	const bool bNoParent = (ContextMenuBuilder.FromPin == NULL);

	 FText AddToolTip = LOCTEXT("NewMoutneaDialogueGraphNodeTooltip", "Add Dialogue Node here");

	TSet<TSubclassOf<UMounteaDialogueGraphNode> > Visited;

	FText Desc = Graph->NodeType.GetDefaultObject()->ContextMenuName;
	FText NodeCategory = Graph->NodeType.GetDefaultObject()->GetNodeCategory();

	if (Desc.IsEmpty())
	{
		FString Title = Graph->NodeType->GetName();
		Title.RemoveFromEnd("_C");
		Desc = FText::FromString(Title);
	}

	if (!Graph->NodeType->HasAnyClassFlags(CLASS_Abstract))
	{
		TSharedPtr<FAssetSchemaAction_MounteaDialogueGraph_NewNode> NewNodeAction(new FAssetSchemaAction_MounteaDialogueGraph_NewNode(NodeCategory, Desc, AddToolTip, 0));
		NewNodeAction->NodeTemplate = NewObject<UEdNode_MounteaDialogueGraphNode>(ContextMenuBuilder.OwnerOfTemporaries);
		NewNodeAction->NodeTemplate->DialogueGraphNode = NewObject<UMounteaDialogueGraphNode>(NewNodeAction->NodeTemplate, Graph->NodeType);
		NewNodeAction->NodeTemplate->DialogueGraphNode->Graph = Graph;
		ContextMenuBuilder.AddAction(NewNodeAction);

		Visited.Add(Graph->NodeType);
	}

	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(Graph->NodeType) && !It->HasAnyClassFlags(CLASS_Abstract) && !Visited.Contains(*It))
		{
			TSubclassOf<UMounteaDialogueGraphNode> NodeType = *It;

			if (It->GetName().StartsWith("REINST") || It->GetName().StartsWith("SKEL"))
				continue;

			if (!Graph->GetClass()->IsChildOf(NodeType.GetDefaultObject()->CompatibleGraphType))
				continue;

			if (!NodeType.GetDefaultObject()->bAllowManualCreate)
				continue;
			
			Desc = NodeType.GetDefaultObject()->ContextMenuName;
			AddToolTip = NodeType.GetDefaultObject()->GetDescription();

			NodeCategory = NodeType.GetDefaultObject()->GetNodeCategory();

			if (Desc.IsEmpty())
			{
				FString Title = NodeType->GetName();
				Title.RemoveFromEnd("_C");
				Desc = FText::FromString(Title);
			}

			TSharedPtr<FAssetSchemaAction_MounteaDialogueGraph_NewNode> Action(new FAssetSchemaAction_MounteaDialogueGraph_NewNode(NodeCategory, Desc, AddToolTip, 0));
			Action->NodeTemplate = NewObject<UEdNode_MounteaDialogueGraphNode>(ContextMenuBuilder.OwnerOfTemporaries);
			Action->NodeTemplate->DialogueGraphNode = NewObject<UMounteaDialogueGraphNode>(Action->NodeTemplate, NodeType);
			Action->NodeTemplate->DialogueGraphNode->Graph = Graph;
			ContextMenuBuilder.AddAction(Action);

			Visited.Add(NodeType);
		}
	}
}

void UAssetGraphScheme_MounteaDialogueGraph::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (Context->Pin)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("MounteaDialogueGraphAssetGraphSchemaNodeActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
			// Only display the 'Break Links' option if there is a link to break!
			if (Context->Pin->LinkedTo.Num() > 0)
			{
				Section.AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);

				// add sub menu for break link to
				if (Context->Pin->LinkedTo.Num() > 1)
				{
					Section.AddSubMenu(
						"BreakLinkTo",
						LOCTEXT("BreakLinkTo", "Break Link To..."),
						LOCTEXT("BreakSpecificLinks", "Break a specific link..."),
						FNewToolMenuDelegate::CreateUObject((UAssetGraphScheme_MounteaDialogueGraph* const)this, &UAssetGraphScheme_MounteaDialogueGraph::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(Context->Pin)));
				}
				else
				{
					((UAssetGraphScheme_MounteaDialogueGraph* const)this)->GetBreakLinkToSubMenuActions(Menu, const_cast<UEdGraphPin*>(Context->Pin));
				}
			}
		}
	}
	else if (Context->Node)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("MounteaDialogueGraphAssetGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "Node Actions"));
			Section.AddSeparator(FName(TEXT("Main Options")));
			Section.AddMenuEntry(FGenericCommands::Get().Rename);
			Section.AddSeparator(FName(TEXT("Other Options")));
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
			Section.AddMenuEntry(FGenericCommands::Get().Cut);
			Section.AddMenuEntry(FGenericCommands::Get().Copy);
			Section.AddMenuEntry(FGenericCommands::Get().Duplicate);

			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
		}
	}

	Super::GetContextMenuActions(Menu, Context);
}

const FPinConnectionResponse UAssetGraphScheme_MounteaDialogueGraph::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	// Make sure the pins are not on the same node
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Both are on the same node"));
	}

	// Compare the directions
	if ((A->Direction == EGPD_Input) && (B->Direction == EGPD_Input))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorInput", "Can't connect input node to input node"));
	}
	else if ((A->Direction == EGPD_Output) && (B->Direction == EGPD_Output))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorOutput", "Can't connect output node to output node"));
	}

	// check for cycles
	FNodeVisitorCycleChecker CycleChecker;
	if (!CycleChecker.CheckForLoop(A->GetOwningNode(), B->GetOwningNode()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorCycle", "Can't create a graph cycle"));
	}

	UEdNode_MounteaDialogueGraphNode* EdNode_A = Cast<UEdNode_MounteaDialogueGraphNode>(A->GetOwningNode());
	UEdNode_MounteaDialogueGraphNode* EdNode_B = Cast<UEdNode_MounteaDialogueGraphNode>(B->GetOwningNode());

	if (EdNode_A == nullptr || EdNode_B == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError", "Not a valid UMounteaDialogueGraphEdNode"));
	}

	FText ErrorMessage;
	if (A->Direction == EGPD_Input)
	{
		if (!EdNode_A->DialogueGraphNode->CanCreateConnection(EdNode_B->DialogueGraphNode, EGPD_Input, ErrorMessage))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
		}
	}
	else
	{
		if (!EdNode_B->DialogueGraphNode->CanCreateConnection(EdNode_A->DialogueGraphNode, EGPD_Output, ErrorMessage))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
		}
	}
	
	if (EdNode_A->DialogueGraphNode->GetGraph()->bEdgeEnabled)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("PinConnect", "Connect nodes with edge"));
	}
	else
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
	}
}

bool UAssetGraphScheme_MounteaDialogueGraph::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* A, UEdGraphPin* B) const
{
	UEdNode_MounteaDialogueGraphNode* NodeA = Cast<UEdNode_MounteaDialogueGraphNode>(A->GetOwningNode());
	UEdNode_MounteaDialogueGraphNode* NodeB = Cast<UEdNode_MounteaDialogueGraphNode>(B->GetOwningNode());

	if (NodeA == nullptr || NodeB == nullptr)
		return false;

	if (NodeA->GetInputPin() == nullptr || NodeA->GetOutputPin() == nullptr || NodeB->GetInputPin() == nullptr || NodeB->GetOutputPin() == nullptr)
		return false;

	UMounteaDialogueGraph* Graph = NodeA->DialogueGraphNode->GetGraph();

	FVector2D InitPos((NodeA->NodePosX + NodeB->NodePosX) / 2, (NodeA->NodePosY + NodeB->NodePosY) / 2);

	FAssetSchemaAction_MounteaDialogueGraph_NewEdge Action;
	Action.NodeTemplate = NewObject<UEdNode_MounteaDialogueGraphEdge>(NodeA->GetGraph());
	Action.NodeTemplate->SetEdge(NewObject<UMounteaDialogueGraphEdge>(Action.NodeTemplate, Graph->EdgeType));
	UEdNode_MounteaDialogueGraphEdge* EdgeNode = Cast<UEdNode_MounteaDialogueGraphEdge>(Action.PerformAction(NodeA->GetGraph(), nullptr, InitPos, false));

	if (A->Direction == EGPD_Output)
	{
		EdgeNode->CreateConnections(NodeA, NodeB);
	}
	else
	{
		EdgeNode->CreateConnections(NodeB, NodeA);
	}

	return true;
}

FConnectionDrawingPolicy* UAssetGraphScheme_MounteaDialogueGraph::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	/*
	if (const UMounteaDialogueGraphEditorSettings* MounteaDialogueGraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
	{
		if (MounteaDialogueGraphEditorSettings->AllowAdvancedWiring())
		{
			return new FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
		}
	}
	*/
	
	return new FConnectionDrawingPolicy_MounteaDialogueGraph(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

FLinearColor UAssetGraphScheme_MounteaDialogueGraph::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

void UAssetGraphScheme_MounteaDialogueGraph::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));

	Super::BreakNodeLinks(TargetNode);
}

void UAssetGraphScheme_MounteaDialogueGraph::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

void UAssetGraphScheme_MounteaDialogueGraph::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));

	Super::BreakSinglePinLink(SourcePin, TargetPin);
}

UEdGraphPin* UAssetGraphScheme_MounteaDialogueGraph::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
	UEdNode_MounteaDialogueGraphNode* EdNode = Cast<UEdNode_MounteaDialogueGraphNode>(InTargetNode);
	switch (InSourcePinDirection)
	{
	case EGPD_Input:
		return EdNode->GetOutputPin();
	case EGPD_Output:
		return EdNode->GetInputPin();
	default:
		return nullptr;
	}
}

bool UAssetGraphScheme_MounteaDialogueGraph::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	return Cast<UEdNode_MounteaDialogueGraphNode>(InTargetNode) != nullptr;
}

bool UAssetGraphScheme_MounteaDialogueGraph::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UAssetGraphScheme_MounteaDialogueGraph::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UAssetGraphScheme_MounteaDialogueGraph::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
}

void UAssetGraphScheme_MounteaDialogueGraph::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	Super::CreateDefaultNodesForGraph(Graph);
}

#undef LOCTEXT_NAMESPACE
