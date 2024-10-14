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
	bReturnValue &= ValidateDecorators(Context, RichTextFormat, GraphDecorators, TEXT("Node"));
	bReturnValue &= ValidateDecorators(Context, RichTextFormat, GraphScopeDecorators, TEXT("Scoped Node"));

	// Validate individual graph decorators
	bReturnValue &= ValidateGraphDecorators(Context, RichTextFormat);

	// Validate scoped graph decorators
	bReturnValue &= ValidateGraphScopeDecorators(Context, RichTextFormat);

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
	TMap<UClass*, int32> DuplicatedDecoratorsMap;

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
		AddDuplicateDecoratorErrors(Context, RichTextFormat, DuplicatedDecoratorsMap);
		bReturnValue = false;
	}

	return bReturnValue;
}

void UMounteaDialogueGraph::FindDuplicatedDecorators(const TArray<UMounteaDialogueDecoratorBase*>& UsedNodeDecorators, TMap<UClass*, int32>& DuplicatedDecoratorsMap) const 
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

		if (ClassAppearance > 1 && !DuplicatedDecoratorsMap.Contains(Itr->GetClass()))
		{
			DuplicatedDecoratorsMap.Add(Itr->GetClass(), ClassAppearance);
		}
	}
}

void UMounteaDialogueGraph::AddInvalidDecoratorError(FDataValidationContext& Context, bool RichTextFormat, int32 Index, const FString& DecoratorTypeName) const 
{
	const FString RichTextReturn = FString::Printf(TEXT("* <RichTextBlock.Bold>Dialogue Graph</>: has <RichTextBlock.Bold>invalid</> %s Decorator at Index: %d."), *DecoratorTypeName, Index);
	const FString TextReturn = FString::Printf(TEXT("%s: has INVALID %s Decorator at Index: %d."), *GetName(), *DecoratorTypeName, Index);

	Context.AddError(FText::FromString(RichTextFormat ? RichTextReturn : TextReturn));
}

void UMounteaDialogueGraph::AddDuplicateDecoratorErrors(FDataValidationContext& Context, bool RichTextFormat, const TMap<UClass*, int32>& DuplicatedDecoratorsMap) const 
{
	for (const auto& Itr : DuplicatedDecoratorsMap)
	{
		const FString RichTextReturn = FString::Printf(TEXT("* <RichTextBlock.Bold>Dialogue Graph</>: has Node Decorator <RichTextBlock.Bold>%s</> %dx times! Please, avoid duplicates!"), *Itr.Key->GetName(), Itr.Value);
		const FString TextReturn = FString::Printf(TEXT("Dialogue Graph: has Node Decorator %s %dx times! Please, avoid duplicates!"), *Itr.Key->GetName(), Itr.Value);

		Context.AddError(FText::FromString(RichTextFormat ? RichTextReturn : TextReturn));
	}
}

bool UMounteaDialogueGraph::ValidateGraphDecorators(FDataValidationContext& Context, bool RichTextFormat) const 
{
	bool bReturnValue = true;
	for (auto Itr : GetGraphDecorators())
	{
		TArray<FText> DecoratorErrors;
		if (!Itr.ValidateDecorator(DecoratorErrors))
		{
			AddDecoratorErrors(Context, RichTextFormat, DecoratorErrors);
			bReturnValue = false;
		}
	}
	return bReturnValue;
}

bool UMounteaDialogueGraph::ValidateGraphScopeDecorators(FDataValidationContext& Context, bool RichTextFormat) const 
{
	bool bReturnValue = true;
	for (auto Itr : GetGraphScopeDecorators())
	{
		TArray<FText> DecoratorErrors;
		if (!Itr.ValidateDecorator(DecoratorErrors))
		{
			AddDecoratorErrors(Context, RichTextFormat, DecoratorErrors);
			bReturnValue = false;
		}
	}
	return bReturnValue;
}

void UMounteaDialogueGraph::AddDecoratorErrors(FDataValidationContext& Context, bool RichTextFormat, const TArray<FText>& DecoratorErrors) const 
{
	for (auto Error : DecoratorErrors)
	{
		const FString ErrorTextRich = FString::Printf(TEXT("* <RichTextBlock.Bold>Dialogue Graph</>: %s"), *Error.ToString());
		const FString ErrorTextSimple = FString::Printf(TEXT("%s: %s"), *GetName(), *Error.ToString());

		Context.AddError(FText::FromString(RichTextFormat ? ErrorTextRich : ErrorTextSimple));
	}
}

bool UMounteaDialogueGraph::ValidateStartNode(FDataValidationContext& Context, bool RichTextFormat) const 
{
	if (StartNode == nullptr)
	{
		const FString RichTextReturn = FString(TEXT("* <RichTextBlock.Bold>Dialogue Graph</>: Has no Start Node!"));
		const FString TextReturn = FString::Printf(TEXT("%s: Has no Start Node!"), *GetName());

		Context.AddError(FText::FromString(RichTextFormat ? RichTextReturn : TextReturn));
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
