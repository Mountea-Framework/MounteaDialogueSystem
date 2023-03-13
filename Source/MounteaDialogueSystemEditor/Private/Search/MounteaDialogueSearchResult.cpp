// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "MounteaDialogueSearchResult.h"

#include "MounteaDialogueSearchUtils.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSearchResult"

FMounteaDialogueSearchResult::FMounteaDialogueSearchResult(const FText& InDisplayText, const TSharedPtr<Self>& InParent) : Super(InDisplayText, InParent)
{
	//Category = InDisplayText;
}

TSharedRef<SWidget> FMounteaDialogueSearchResult::CreateIcon() const
{
	const FLinearColor IconColor = FLinearColor::White;
	const FSlateBrush* Brush = nullptr;

	return
	SNew(SImage)
		.Image(Brush)
		.ColorAndOpacity(IconColor)
		.ToolTipText(GetCategory());
}

TWeakObjectPtr<const UMounteaDialogueGraph> FMounteaDialogueSearchResult::GetParentDialogue() const
{
	if (Parent.IsValid())
	{
		return Parent.Pin()->GetParentDialogue();
	}

	return nullptr;
}

FMounteaDialogueSearchResult_RootNode::FMounteaDialogueSearchResult_RootNode() : Super(FText::FromString(TEXT("INVALID")), nullptr)
{
	Category = LOCTEXT("MounteaDialogueSearchResult_RootNodeCategory", "Root");
}

FMounteaDialogueSearchResult_DialogueNode::FMounteaDialogueSearchResult_DialogueNode(const FText& InDisplayText, const TSharedPtr<FMounteaDialogueSearchResult>& InParent) : Super(InDisplayText, InParent)
{
	Category = LOCTEXT("MounteaDialogueSearchResult_DialogueNodeCategory", "Dialogue Node");
}

#pragma region Search_Dialogues

FReply FMounteaDialogueSearchResult_DialogueNode::OnClick()
{
	// TODO: Remove
	return FReply::Unhandled();
}

TSharedRef<SWidget> FMounteaDialogueSearchResult_DialogueNode::CreateIcon() const
{
	return Super::CreateIcon();
}

TWeakObjectPtr<const UMounteaDialogueGraph> FMounteaDialogueSearchResult_DialogueNode::GetParentDialogue() const
{
	// Get the Dialogue from this.
	if (DialogueGraph.IsValid())
	{
		return DialogueGraph;
	}

	return Super::GetParentDialogue();
}

#pragma endregion 

#pragma region Search_Nodes

FMounteaDialogueSearchResult_GraphNode::FMounteaDialogueSearchResult_GraphNode(const FText& InDisplayText, const TSharedPtr<FMounteaDialogueSearchResult>& InParent)
: Super(InDisplayText, InParent)
{}

FReply FMounteaDialogueSearchResult_GraphNode::OnClick()
{
	if (GraphNode.IsValid())
	{
		return FMounteaDialogueSearchUtils::OpenEditorAndJumpToGraphNode(GraphNode.Get()) ? FReply::Handled() : FReply::Unhandled();
	}

	return FReply::Unhandled();
}

TSharedRef<SWidget> FMounteaDialogueSearchResult_GraphNode::CreateIcon() const
{
	if (GraphNode.IsValid())
	{
		FLinearColor Color;
		const FSlateIcon Icon = GraphNode.Get()->GetIconAndTint(Color);
		return SNew(SImage)
				.Image(Icon.GetOptionalIcon())
				.ColorAndOpacity(Color)
				.ToolTipText(GetCategory());
	}
	
	return Super::CreateIcon();
}

#pragma endregion 

#undef LOCTEXT_NAMESPACE