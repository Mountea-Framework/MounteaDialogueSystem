// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Widgets/Views/STreeView.h"

class UMounteaDialogueGraph;
/**
 * Template Tree Item Node class.
 */
template <class SelfType>
class FMounteaTreeItemNode : public TSharedFromThis<SelfType>
{
	
public:

	FMounteaTreeItemNode(const FText& InDisplayText, const TSharedPtr<SelfType>& InParent)
	: Parent(InParent),
	DisplayText(InDisplayText)
	{};
	virtual ~FMounteaTreeItemNode() {};

#pragma region Click_Functions
	
	virtual FReply OnClick()
	{
		// If there is a parent, handle it using the parent's functionality
		if (Parent.IsValid())
		{
			return Parent.Pin()->OnClick();
		}

		return FReply::Unhandled();
	}

#pragma endregion

#pragma region DisplayText_Functions

	FText GetDisplayText() const { return DisplayText; }
	FName GetDisplayTextAsFName() const { return FName(*DisplayText.ToString()); }
	void SetDisplayText(const FText& InText) { DisplayText = InText; }
	bool DoesDisplayTextContains(const FString& InSearch, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		return DisplayText.ToString().Contains(InSearch, SearchCase);
	}

#pragma endregion

#pragma region Parent_Functions

	bool HasParent() const { return Parent.IsValid(); }
	TWeakPtr<SelfType> GetParent() const { return Parent; }
	void SetParent(TWeakPtr<SelfType> InParentNode) { Parent = InParentNode; }
	void ClearParent() { Parent.Reset(); }

#pragma endregion

#pragma region Children_Functions

public:
	
	bool HasChildren() const { return Children.Num() > 0; }
	const TArray<TSharedPtr<SelfType>>& GetChildren() const { return Children; }
	void GetVisibleChildren(TArray<TSharedPtr<SelfType>>& OutChildren)
	{
		for (const TSharedPtr<SelfType>& Child : Children)
		{
			if (Child->IsVisible())
			{
				OutChildren.Add(Child);
			}
		}
	}
	virtual void AddChild(const TSharedPtr<SelfType>& ChildNode)
	{
		ensure(!ChildNode->IsRoot());
		ChildNode->SetParent(this->AsShared());
		Children.Add(ChildNode);
	}
	virtual void SetChildren(const TArray<TSharedPtr<SelfType>>& InChildren)
	{
		Children = InChildren;
		for (const TSharedPtr<SelfType>& Child : Children)
		{
			ensure(!Child->IsRoot());
			Child->SetParent(this->AsShared());
		}
	}
	virtual void ClearChildren()
	{
		Children.Empty();
	}

	void ExpandAllChildren(const TSharedPtr<STreeView<TSharedPtr<SelfType>>>& TreeView, bool bRecursive = true)
	{
		static constexpr bool bShouldExpandItem = true;
		if (!HasChildren())
		{
			return;
		}

		TreeView->SetItemExpansion(this->AsShared(), bShouldExpandItem);
		for (const TSharedPtr<SelfType>& ChildNode : Children)
		{
			if (bRecursive)
			{
				// recursive on all children.
				ChildNode->ExpandAllChildren(TreeView, bRecursive);
			}
			else
			{
				// Only direct children
				TreeView->SetItemExpansion(ChildNode, bShouldExpandItem);
			}
		}
	}

#pragma endregion

#pragma region Helper_Functions

public:
	bool IsRoot() const { return !Parent.IsValid(); }

#pragma endregion 

protected:
	/** Any children listed under this node. */
	TArray<TSharedPtr<SelfType>> Children;

	/** The node that this is a direct child of (empty if this is a root node)  */
	TWeakPtr<SelfType> Parent;

	/** The displayed text for this item. */
	FText DisplayText;

	/** Is this node displayed? */
	bool bIsVisible = true;
};

class FMounteaDialogueSearchResult : public FMounteaTreeItemNode<FMounteaDialogueSearchResult>
{
	typedef FMounteaDialogueSearchResult Self;
	typedef FMounteaTreeItemNode Super;
	
public:
	
	FMounteaDialogueSearchResult(const FText& InDisplayText, const TSharedPtr<Self>& InParent);

public:

	// Create an icon to represent the result
	virtual TSharedRef<SWidget>	CreateIcon() const;

	// Gets the Dialogue housing all these search results. Aka the Dialogue this search result belongs to.
	virtual TWeakObjectPtr<const UMounteaDialogueGraph> GetParentDialogue() const;

	// Category:
	FText GetCategory() const { return Category; }
	void SetCategory(const FText& InCategory) { Category = InCategory; }

	// CommentString
	FString GetCommentString() const { return CommentString; }
	void SetCommentString(const FString& InCommentString) { CommentString = InCommentString; }

protected:
	// The category of this node.
	FText Category;

	// Display text for comment information
	FString CommentString;
};

// Root Node, should not be displayed.
class FMounteaDialogueSearchResult_RootNode : public FMounteaDialogueSearchResult
{
	typedef FMounteaDialogueSearchResult Super;
	
public:
	FMounteaDialogueSearchResult_RootNode();
};

// Tree Node result that represents the Node
class FMounteaDialogueSearchResult_DialogueNode : public FMounteaDialogueSearchResult
{
	typedef FMounteaDialogueSearchResult Super;
	
public:
	FMounteaDialogueSearchResult_DialogueNode(const FText& InDisplayText, const TSharedPtr<FMounteaDialogueSearchResult>& InParent);

	virtual FReply OnClick() override;
	virtual TSharedRef<SWidget> CreateIcon() const override;
	virtual TWeakObjectPtr<const UMounteaDialogueGraph> GetParentDialogue() const override;

	// GraphNode:
	void SetDialogueGraph(TWeakObjectPtr<const UEdGraph_MounteaDialogueGraph> InDialogueGraph) { DialogueGraph = InDialogueGraph->GetMounteaDialogueGraph(); }

protected:
	// The GraphNode this represents.
	TWeakObjectPtr<const UMounteaDialogueGraph> DialogueGraph;
};

// Tree Node result that represents the GraphNode
class FMounteaDialogueSearchResult_GraphNode : public FMounteaDialogueSearchResult
{
	typedef FMounteaDialogueSearchResult Super;
	
public:
	FMounteaDialogueSearchResult_GraphNode(const FText& InDisplayText, const TSharedPtr<FMounteaDialogueSearchResult>& InParent);

	virtual FReply OnClick() override;
	virtual TSharedRef<SWidget> CreateIcon() const override;

	// GraphNode:
	void SetGraphNode(TWeakObjectPtr<const UEdNode_MounteaDialogueGraphNode> InGraphNode) { GraphNode = InGraphNode; }

protected:
	// The GraphNode this represents.
	TWeakObjectPtr<const UEdNode_MounteaDialogueGraphNode> GraphNode;
};