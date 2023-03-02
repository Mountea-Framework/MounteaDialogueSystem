// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphNode_Details.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FMounteaDialogueGraphNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// TODO: Update default behaviour to Show Advanced categories
	/*
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// Only support one object being customized
	if (ObjectsBeingCustomized.Num() != 1) return;

	TWeakObjectPtr<UMounteaDialogueGraphNode> WeakGraphNode = Cast<UMounteaDialogueGraphNode>(ObjectsBeingCustomized[0].Get());
	if (!WeakGraphNode.IsValid()) return;

	GraphNode = WeakGraphNode.Get();
	if (!IsValid(GraphNode)) return;

	DetailLayoutBuilder = &DetailBuilder;
	IDetailCategoryBuilder& BaseDataCategory = DetailLayoutBuilder->EditCategory(TEXT("Dialogue"), FText::GetEmpty(), ECategoryPriority::Important);
	BaseDataCategory.InitiallyCollapsed(false);
	const TSharedPtr<IPropertyHandle> PropertyDialogueNode = DetailLayoutBuilder->GetProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode, NodeDecorators), UMounteaDialogueGraphNode::StaticClass());

	IDetailCategoryBuilder& HiddenDataCategory = DetailLayoutBuilder->EditCategory(TEXT("Hidden"), FText::GetEmpty(), ECategoryPriority::Important);
	HiddenDataCategory.SetCategoryVisibility(true);
	HiddenDataCategory.SetShowAdvanced(true);
	*/
}
