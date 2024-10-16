// All rights reserved Dominik Pavlicek 2023

#include "Graph/MounteaDialogueGraph.h"

#include "Edges/MounteaDialogueGraphEdge.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Misc/DataValidation.h"
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

void UMounteaDialogueGraph::SetGraphGUID(const FGuid& NewGuid)
{
	GraphGUID = NewGuid;
}

UMounteaDialogueGraphNode* UMounteaDialogueGraph::FindNodeByGuid(const FGuid& NodeGuid)
{
	TArray<UMounteaDialogueGraphNode*> allDialogueNodes = AllNodes;
	allDialogueNodes.AddUnique(StartNode);
	for (UMounteaDialogueGraphNode* Node : allDialogueNodes)
	{
		if (Node && Node->GetNodeGUID() == NodeGuid)
		{
			return Node;
		}
	}

	return nullptr;
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
	TArray<FMounteaDialogueDecorator> TempReturn;
	TArray<FMounteaDialogueDecorator> Return;

	for (auto Itr : GraphDecorators)
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

TArray<FMounteaDialogueDecorator> UMounteaDialogueGraph::GetGraphScopeDecorators() const
{
	return GraphScopeDecorators;
}

TArray<FMounteaDialogueDecorator> UMounteaDialogueGraph::GetAllDecorators() const
{
	TArray<FMounteaDialogueDecorator> TempReturn;
	TArray<FMounteaDialogueDecorator> Return;

	for (const auto& Itr : AllNodes)
	{
		if (Itr && Itr->GetNodeDecorators().Num() > 0)
		{
			TempReturn.Append(Itr->NodeDecorators);
		}
	}

	TempReturn.Append(GetGraphDecorators());
	TempReturn.Append(GetGraphScopeDecorators());

	return TempReturn;
}

bool UMounteaDialogueGraph::CanStartDialogueGraph() const
{
	bool bSatisfied = true;

	if (AllNodes.Num() == 0)
	{
		return false;
	}

	for (const auto& Itr : AllNodes)
	{
		if (!Itr)
		{
			return false;
		}

		if (Itr->ValidateNodeRuntime() == false)
		{
			return false;
		}
	}

	auto Decorators = GetAllDecorators();

	if (Decorators.Num() == 0)
	{
		return bSatisfied;
	}

	TArray<FText> DecoratorValidations;
	for (auto Itr : Decorators)
	{
		if (Itr.ValidateDecorator(DecoratorValidations) == false) bSatisfied = false;
	}

	if (DecoratorValidations.Num() > 0)
	{
		for (auto Itr : DecoratorValidations)
		{
			LOG_ERROR(TEXT("%s"), *Itr.ToString());
		}
	}
	return bSatisfied;
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
	if (StartNode != nullptr)
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

void UMounteaDialogueGraph::RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().AddUniqueDynamic(this, &UMounteaDialogueGraph::TickMounteaEvent);
	}
}

void UMounteaDialogueGraph::UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().RemoveDynamic(this, &UMounteaDialogueGraph::TickMounteaEvent);
	}
}

void UMounteaDialogueGraph::TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick, float DeltaTime)
{
	GraphTickEvent.Broadcast(this, ParentTick, DeltaTime);
}

#if WITH_EDITOR

bool UMounteaDialogueGraph::ValidateGraph(FDataValidationContext& Context, bool RichTextFormat) const
{
	bool bReturnValue = true;
	
	// Validate Graph and Scoped Decorators
	bReturnValue &= ValidateDecorators(Context, RichTextFormat, GraphDecorators, TEXT("Graph Decorators"));
	bReturnValue &= ValidateDecorators(Context, RichTextFormat, GraphScopeDecorators, TEXT("Scoped Graph Decorators"));

	// Validate Start Node
	bReturnValue &= ValidateStartNode(Context, RichTextFormat);

	// Validate all nodes
	bReturnValue &= ValidateAllNodes(Context, RichTextFormat);

	return bReturnValue;
}

bool UMounteaDialogueGraph::ValidateDecorators(FDataValidationContext& Context, bool RichTextFormat, const TArray<FMounteaDialogueDecorator>& Decorators, const FString& DecoratorTypeName) const
{
	bool bReturnValue = true;

	TArray<UMounteaDialogueDecoratorBase*> UsedNodeDecorators;
	TMap<UMounteaDialogueDecoratorBase*, int32> DuplicatedDecoratorsMap;

	// Collect valid decorators
	for (int i = 0; i < Decorators.Num(); i++)
	{
		if (Decorators.IsValidIndex(i) && Decorators[i].DecoratorType)
		{
			UsedNodeDecorators.Add(Decorators[i].DecoratorType);
		}
		else
		{
			AddInvalidDecoratorError(Context, RichTextFormat, i, DecoratorTypeName);
			bReturnValue = false;
		}
	}

	// Check for duplicates
	FindDuplicatedDecorators(UsedNodeDecorators, DuplicatedDecoratorsMap);
	if (DuplicatedDecoratorsMap.Num() > 0)
	{
		AddDuplicateDecoratorErrors(Context, RichTextFormat, DuplicatedDecoratorsMap, DecoratorTypeName);
		bReturnValue = false;
	}
	
	return bReturnValue && ValidateGraphDecorators(Context, RichTextFormat, Decorators, DecoratorTypeName);
}

void UMounteaDialogueGraph::FindDuplicatedDecorators(const TArray<UMounteaDialogueDecoratorBase*>& UsedNodeDecorators, TMap<UMounteaDialogueDecoratorBase*, int32>& DuplicatedDecoratorsMap) const
{
	for (const auto& Itr : UsedNodeDecorators)
	{
		if (!Itr) continue;

		// Let's ignore stackable decorators (executing commands etc.)
		if (Itr->IsDecoratorStackable()) continue;
		
		int32 ClassAppearance = 1;
		for (const auto& Itr2 : UsedNodeDecorators)
		{
			if (Itr != Itr2 && Itr->GetClass() == Itr2->GetClass())
			{
				ClassAppearance++;
			}
		}

		if (ClassAppearance > 1 && !DuplicatedDecoratorsMap.Contains(Itr))
		{
			DuplicatedDecoratorsMap.Add(Itr, ClassAppearance);
		}
	}
}

void UMounteaDialogueGraph::AddInvalidDecoratorError(FDataValidationContext& Context, bool RichTextFormat, int32 Index, const FString& DecoratorTypeName) const
{
	const FText TempRichText = FText::Format(INVTEXT("* <RichTextBlock.Bold>Dialogue Graph</>: invalid <RichTextBlock.Bold>{0} Decorator</> at Index: {1}."), FText::FromString(DecoratorTypeName), FText::FromString(FString::FromInt(Index)));
	const FText TempText = FText::Format(INVTEXT("{0}: invalid {1} Decorator at Index: {2}."), FText::FromString(GetName()),  FText::FromString(DecoratorTypeName), FText::FromString(FString::FromInt(Index)));

	Context.AddError(FText(RichTextFormat ? TempRichText : TempText));
}

void UMounteaDialogueGraph::AddDuplicateDecoratorErrors(FDataValidationContext& Context, bool RichTextFormat, const TMap<UMounteaDialogueDecoratorBase*, int32>& DuplicatedDecoratorsMap, const FString& DecoratorTypeName) const
{
	for (const auto& Itr : DuplicatedDecoratorsMap) 
	{
		const FText TempRichText = FText::Format(INVTEXT("* <RichTextBlock.Bold>Dialogue Graph</>: <RichTextBlock.Bold>{0}</>\nNode Decorator <RichTextBlock.Bold>{1}</> {2}x times! Please, avoid duplicates!"), 
			FText::FromString(DecoratorTypeName), Itr.Key->GetDecoratorName(), Itr.Value);
		const FText TempText = FText::Format(INVTEXT("{0}: {1} has Node Decorator {2} {3}x times! Please, avoid duplicates!"), 
			FText::FromString(GetName()), FText::FromString(DecoratorTypeName), Itr.Key->GetDecoratorName(), Itr.Value);

		Context.AddError(RichTextFormat ? TempRichText : TempText);
	}
}

bool UMounteaDialogueGraph::ValidateGraphDecorators(FDataValidationContext& Context, bool RichTextFormat, const TArray<FMounteaDialogueDecorator>& Decorators, const FString& DecoratorTypeName) const
{
	bool bReturnValue = true;
	TArray<FText> DecoratorErrors;
	for (auto Itr : Decorators)
	{
		if (!Itr.ValidateDecorator(DecoratorErrors))
		{
			bReturnValue = false;
		}
	}

	AddDecoratorErrors(Context, RichTextFormat, DecoratorErrors, DecoratorTypeName);

	return bReturnValue;
}

void UMounteaDialogueGraph::AddDecoratorErrors(FDataValidationContext& Context, bool RichTextFormat, const TArray<FText>& DecoratorErrors, const FString& DecoratorTypeName) const
{
	for (auto Error : DecoratorErrors)
	{
		const FText TempRichText = FText::Format(INVTEXT("* <RichTextBlock.Bold>Dialogue Graph</>: <RichTextBlock.Bold>{0}</>\n{1}"),  FText::FromString(DecoratorTypeName), Error);
		const FText TempText = FText::Format(INVTEXT("{0}: {1} {2}"), FText::FromString(GetName()),  FText::FromString(DecoratorTypeName), Error);

		Context.AddError(RichTextFormat ? TempRichText : TempText);
	}
}

bool UMounteaDialogueGraph::ValidateStartNode(FDataValidationContext& Context, bool RichTextFormat) const
{
	if (StartNode == nullptr)
	{
		const FText TempRichText = INVTEXT("* <RichTextBlock.Bold>Dialogue Graph</>: Has no Start Node!");
		const FText TempText = FText::Format(INVTEXT("{0}: Has no Start Node!"), FText::FromString(GetName()));

		Context.AddError(RichTextFormat ? TempRichText : TempText);
		return false;
	}
	return true;
}

bool UMounteaDialogueGraph::ValidateAllNodes(FDataValidationContext& Context, bool RichTextFormat) const
{
	bool bReturnValue = true;
	for (UMounteaDialogueGraphNode* Itr : AllNodes)
	{
		if (Itr != nullptr && !Itr->ValidateNode(Context, RichTextFormat))
		{
			bReturnValue = false;
		}
	}
	return bReturnValue;
}


EDataValidationResult UMounteaDialogueGraph::IsDataValid(FDataValidationContext& Context) 
{
	if (ValidateGraph(Context, false))
	{
		return EDataValidationResult::Valid;
	}

	return EDataValidationResult::Invalid;
}

UMounteaDialogueGraphNode* UMounteaDialogueGraph::ConstructDialogueNode(
	TSubclassOf<UMounteaDialogueGraphNode> NodeClass)
{
	// Set flag to be transactional so it registers with undo system
	UMounteaDialogueGraphNode* DialogueNode = NewObject<UMounteaDialogueGraphNode>(
		this, NodeClass, NAME_None, RF_Transactional);
	if (DialogueNode)
	{
		DialogueNode->OnCreatedInEditor();
		DialogueNode->Graph = this;
	}
	return DialogueNode;
}

#endif

#undef LOCTEXT_NAMESPACE
