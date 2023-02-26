// All rights reserved Dominik Pavlicek 2023


#include "MounteaDialogueGraphAssetAction.h"

#include "Graph/MounteaDialogueGraph.h"
#include "AssetEditor/AssetEditor_MounteaDialogueGraph.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueNodeAssetAction"

FMounteaDialogueGraphAssetAction::FMounteaDialogueGraphAssetAction()
{
}

FText FMounteaDialogueGraphAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueGraphAssetAction", "1. Mountea Dialogue Tree");
}

/*
FText FMounteaDialogueGraphAssetAction::GetAssetDescription(const FAssetData& AssetData) const
{
	return LOCTEXT("MounteaDialogueGraphAssetAction", "Mountea Dialogue Tree Asset for creating Mountea Dialogues.");
}
*/

FColor FMounteaDialogueGraphAssetAction::GetTypeColor() const
{
	return FColor::Orange;
}

UClass* FMounteaDialogueGraphAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueGraph::StaticClass();
}

void FMounteaDialogueGraphAssetAction::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UMounteaDialogueGraph* Graph = Cast<UMounteaDialogueGraph>(*ObjIt))
		{
			TSharedRef<FAssetEditor_MounteaDialogueGraph> NewGraphEditor(new FAssetEditor_MounteaDialogueGraph());
			NewGraphEditor->InitMounteaDialogueGraphAssetEditor(Mode, EditWithinLevelEditor, Graph);
		}
	}
}

uint32 FMounteaDialogueGraphAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	}
	
	return  EAssetTypeCategories::Misc;
};

#undef LOCTEXT_NAMESPACE
