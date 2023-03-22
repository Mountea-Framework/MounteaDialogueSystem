// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueDecorator_Details.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"


#define LOCTEXT_NAMESPACE "FMounteaDialogueDecorator_Details"


void FMounteaDialogueDecorator_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// Only support one object being customized
	if (ObjectsBeingCustomized.Num() != 1) return;

	const TWeakObjectPtr<UMounteaDialogueDecoratorBase> WeakDecorator = Cast<UMounteaDialogueDecoratorBase>(ObjectsBeingCustomized[0].Get());
	if (!WeakDecorator.IsValid()) return;

	EditingDecorator = WeakDecorator.Get();
	if (!EditingDecorator) return;

	// Only edit if editing Graph Editor
	if (DetailBuilder.GetBaseClass()->IsChildOf(UEdNode_MounteaDialogueGraphNode::StaticClass()) == false)
	{ return; };
	
	SavedLayoutBuilder = &DetailBuilder;

	// Hide those categories in Graph Editor
	DetailBuilder.HideCategory("Base");
	DetailBuilder.HideCategory("Private");
	DetailBuilder.HideCategory("Hidden");
	DetailBuilder.HideCategory("Hide");
	DetailBuilder.HideCategory("Editor");
}

#undef LOCTEXT_NAMESPACE