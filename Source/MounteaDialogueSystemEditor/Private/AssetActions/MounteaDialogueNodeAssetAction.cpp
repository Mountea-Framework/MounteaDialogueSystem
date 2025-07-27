// All rights reserved Dominik Morse 2024


#include "MounteaDialogueNodeAssetAction.h"

#include "Nodes/MounteaDialogueGraphNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueNodeAssetAction"

FMounteaDialogueNodeAssetAction::FMounteaDialogueNodeAssetAction()
{
}

FText FMounteaDialogueNodeAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueNodeAssetAction", "Mountea Dialogue Graph Node");
}

FColor FMounteaDialogueNodeAssetAction::GetTypeColor() const
{
	return FColor::Orange;
}

UClass* FMounteaDialogueNodeAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueGraphNode::StaticClass();
}

uint32 FMounteaDialogueNodeAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));

	return EAssetTypeCategories::Misc;
}

const TArray<FText>& FMounteaDialogueNodeAssetAction::GetSubMenus() const
{
	static TArray<FText> AssetTypeActionSubMenu
	{
		FText::FromString("3. Configuration")
	};
	return AssetTypeActionSubMenu;
}

#undef LOCTEXT_NAMESPACE
