// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "MounteaDialogueSearchManager.h"

#include "MounteaDialogueSearchFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSearchManager"

FMounteaDialogueSearchManager* FMounteaDialogueSearchManager::Instance = nullptr;

FMounteaDialogueSearchManager* FMounteaDialogueSearchManager::Get()
{
	if (Instance == nullptr)
	{
		Instance = new Self();
	}

	return Instance;
}

FMounteaDialogueSearchManager::FMounteaDialogueSearchManager()
{
	// Create the Tab Ids
	for (int32 TabIdx = 0; TabIdx < UE_ARRAY_COUNT(GlobalFindResultsTabIDs); TabIdx++)
	{
		const FName TabID = FName(*FString::Printf(TEXT("GlobalNotYetDialogueFindResults_%02d"), TabIdx + 1));
		GlobalFindResultsTabIDs[TabIdx] = TabID;
	}
}

FMounteaDialogueSearchManager::~FMounteaDialogueSearchManager()
{
	UnInitialize();
}

bool FMounteaDialogueSearchManager::QueryGraphNode(const FMounteaDialogueSearchFilter& SearchFilter, const UEdNode_MounteaDialogueGraphNode* InGraphNode, const TSharedPtr<FMounteaDialogueSearchResult>& OutParentNode) const
{
	if (SearchFilter.SearchString.IsEmpty() || !OutParentNode.IsValid() || !IsValid(InGraphNode))
	{
		return false;
	}
	
	bool bContainsSearchString = false;
	const UMounteaDialogueGraphNode* Node = InGraphNode->DialogueGraphNode;

	if (Node == nullptr) return false;
	
	const FString NodeType = Node->InternalName.ToString();
	
	const FText DisplayText = FText::Format
	(
		LOCTEXT("TreeGraphNodeCategory", "Found results in {0}"),
		FText::FromString(NodeType)
	);

	const TSharedPtr<FMounteaDialogueSearchResult_GraphNode> TreeGraphNode = MakeShared<FMounteaDialogueSearchResult_GraphNode>(DisplayText, OutParentNode);
	TreeGraphNode->SetCategory(FText::FromString(NodeType));
	TreeGraphNode->SetGraphNode(InGraphNode);
	if (bContainsSearchString)
	{
		OutParentNode->AddChild(TreeGraphNode);
	}

	// Search by Title
	if (SearchFilter.bIncludeNodeTitle)
	{
		if (Node->NodeTitle.ToString().Contains(SearchFilter.SearchString))
		{
			bContainsSearchString = true;
			MakeChildTextNode
			(
				TreeGraphNode,
				FText::FromName(FName(Node->NodeTitle.ToString() )),
				LOCTEXT("NodeTitleKey", "Node Title"),
				TEXT("Node Title")
			);
		}
	}

	// Search by InternalName
	if (SearchFilter.bIncludeNodeType)
	{
		if (Node->InternalName.ToString().Contains(SearchFilter.SearchString))
		{
			bContainsSearchString = true;
			MakeChildTextNode
			(
				TreeGraphNode,
				FText::FromName(FName(Node->InternalName.ToString() )),
				LOCTEXT("NodeTypeKey", "Node Type"),
				TEXT("Node Type")
			);
		}
	}

	// Search by Decorators
	if (SearchFilter.bIncludeNodeDecoratorsTypes)
	{
		const TArray<FMounteaDialogueDecorator>& NodeDecorators = Node->GetNodeDecorators();
		for (int32 Index = 0, Num = NodeDecorators.Num(); Index < Num; Index++)
		{
			bContainsSearchString = QueryNodeDecorators
			(
				SearchFilter,
				NodeDecorators[Index],
				TreeGraphNode,
				Index,
				TEXT("DecoratorType")
			)
			|| bContainsSearchString;
		}
	}

	// Search by Node Data
	if (SearchFilter.bIncludeNodeData)
	{
		if (const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node))
		{
			if (DialogueNodeBase->GetRowName().ToString().Contains(SearchFilter.SearchString))
			{
				bContainsSearchString = true;
				MakeChildTextNode
				(
					TreeGraphNode,
					FText::FromName(FName(Node->InternalName.ToString() )),
					LOCTEXT("NodeDataRowKey", "Node Data"),
					TEXT("Node Data")
				);
			}
		}
	}

	// Search by GUID
	if (SearchFilter.bIncludeNodeGUID)
	{
		const FString FoundGUID = Node->GetNodeGUID().ToString();
		if (FoundGUID.Contains(SearchFilter.SearchString))
		{
			bContainsSearchString = true;
			MakeChildTextNode
			(
				TreeGraphNode,
				FText::FromString(FoundGUID),
				LOCTEXT("NodeGUID", "Node GUID"),
				TEXT("Node GUID")
			);
		}
	}
	
	if (bContainsSearchString)
	{
		OutParentNode->AddChild(TreeGraphNode);
	}

	return bContainsSearchString;
}

bool FMounteaDialogueSearchManager::QueryNodeDecorators(const FMounteaDialogueSearchFilter& SearchFilter, const FMounteaDialogueDecorator& InDecorator, const TSharedPtr<FMounteaDialogueSearchResult>& OutParentNode, int32 DecoratorIndex, FName DecoratorMemberName) const
{
	if (SearchFilter.SearchString.IsEmpty() || !OutParentNode.IsValid())
	{
		return false;
	}
	bool bContainsSearchString = false;

	if (InDecorator.DecoratorType == nullptr) return false;

	// Search by Decorator Name
	if (InDecorator.DecoratorType->GetName().Contains(SearchFilter.SearchString))
	{
		bContainsSearchString = true;

		FString DecoratorName = InDecorator.DecoratorType->GetClass()->GetName();
		// Format Name
		{
			if (DecoratorName.Contains(TEXT("_GEN_VARIABLE")))
			{
				DecoratorName.ReplaceInline(TEXT("_GEN_VARIABLE"), TEXT(""));
			}
			if(DecoratorName.EndsWith(TEXT("_C")) && DecoratorName.StartsWith(TEXT("Default__")))
			{
				DecoratorName.RightChopInline(9);
				DecoratorName.LeftChopInline(2);
			}
			if (DecoratorName.EndsWith(TEXT("_C")))
			{
				DecoratorName.LeftChopInline(2);
			}
		}
		
		const FText Category = FText::Format
		(
			LOCTEXT("DecoratorName", "Node Decorator: {0} at Index: {1}"),
			FText::FromString(DecoratorName), FText::AsNumber(DecoratorIndex)
		);
		MakeChildTextNode
		(
			OutParentNode,
			FText::FromString(DecoratorName),
			Category,
			Category.ToString()
		);
	}

	return bContainsSearchString;
}

bool FMounteaDialogueSearchManager::QuerySingleDialogue(const FMounteaDialogueSearchFilter& SearchFilter, const UMounteaDialogueGraph* InDialogue, TSharedPtr<FMounteaDialogueSearchResult>& OutParentNode)
{
	if (SearchFilter.SearchString.IsEmpty() || !OutParentNode.IsValid() || !IsValid(InDialogue))
	{
		return false;
	}

	const UEdGraph_MounteaDialogueGraph* Graph = CastChecked<UEdGraph_MounteaDialogueGraph>(InDialogue->EdGraph);

	const TSharedPtr<FMounteaDialogueSearchResult_DialogueNode> TreeDialogueNode = MakeShared<FMounteaDialogueSearchResult_DialogueNode>
	(
			FText::FromString(InDialogue->GetPathName()), OutParentNode
	);
	TreeDialogueNode->SetDialogueGraph(Graph);

	// Find in GraphNodes
	bool bFoundInDialogue = false;
	const TArray<UEdGraphNode*>& AllGraphNodes = Graph->Nodes;
	for (UEdGraphNode* Node : AllGraphNodes)
	{
		bool bFoundInNode = false;
		if (const UEdNode_MounteaDialogueGraphNode* GraphNode = Cast<UEdNode_MounteaDialogueGraphNode>(Node))
		{
			bFoundInNode = QueryGraphNode(SearchFilter, GraphNode, TreeDialogueNode);
		}

		// Found at least one match in one of the nodes.
		bFoundInDialogue = bFoundInNode || bFoundInDialogue;
	}

	if (bFoundInDialogue)
	{
		OutParentNode->AddChild(TreeDialogueNode);
	}

	return bFoundInDialogue;
}

void FMounteaDialogueSearchManager::Initialize(TSharedPtr<FWorkspaceItem> ParentTabCategory)
{
	// Must ensure we do not attempt to load the AssetRegistry Module while saving a package, however, if it is loaded already we can safely obtain it
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	OnAssetAddedHandle = AssetRegistry->OnAssetAdded().AddRaw(this, &Self::HandleOnAssetAdded);
	OnAssetRemovedHandle = AssetRegistry->OnAssetRemoved().AddRaw(this, &Self::HandleOnAssetRemoved);
	OnAssetRenamedHandle = AssetRegistry->OnAssetRenamed().AddRaw(this, &Self::HandleOnAssetRenamed);

	if (AssetRegistry->IsLoadingAssets())
	{
		OnFilesLoadedHandle = AssetRegistry->OnFilesLoaded().AddRaw(this, &Self::HandleOnAssetRegistryFilesLoaded);
	}
	else
	{
		// Already loaded
		HandleOnAssetRegistryFilesLoaded();
	}
	OnAssetLoadedHandle = FCoreUObjectDelegates::OnAssetLoaded.AddRaw(this, &Self::HandleOnAssetLoaded);
}

void FMounteaDialogueSearchManager::UnInitialize()
{
	if (AssetRegistry)
	{
		if (OnAssetAddedHandle.IsValid())
		{
			AssetRegistry->OnAssetAdded().Remove(OnAssetAddedHandle);
			OnAssetAddedHandle.Reset();
		}
		if (OnAssetRemovedHandle.IsValid())
		{
			AssetRegistry->OnAssetRemoved().Remove(OnAssetRemovedHandle);
			OnAssetRemovedHandle.Reset();
		}
		if (OnFilesLoadedHandle.IsValid())
		{
			AssetRegistry->OnFilesLoaded().Remove(OnFilesLoadedHandle);
			OnFilesLoadedHandle.Reset();
		}
		if (OnAssetRenamedHandle.IsValid())
		{
			AssetRegistry->OnAssetRenamed().Remove(OnAssetRenamedHandle);
			OnAssetRenamedHandle.Reset();
		}
	}

	if (OnAssetLoadedHandle.IsValid())
	{
		FCoreUObjectDelegates::OnAssetLoaded.Remove(OnAssetLoadedHandle);
		OnAssetLoadedHandle.Reset();
	}
}

void FMounteaDialogueSearchManager::HandleOnAssetAdded(const FAssetData& InAssetData)
{
	
}

void FMounteaDialogueSearchManager::HandleOnAssetRemoved(const FAssetData& InAssetData)
{
	
}

void FMounteaDialogueSearchManager::HandleOnAssetRenamed(const FAssetData& InAssetData, const FString& InOldName)
{
	
}

void FMounteaDialogueSearchManager::HandleOnAssetLoaded(UObject* InAsset)
{
	
}

void FMounteaDialogueSearchManager::HandleOnAssetRegistryFilesLoaded()
{
	
}


#undef LOCTEXT_NAMESPACE
