// All rights reserved Dominik Morse (Pavlicek) 2024


#include "MounteaDialogueDataTableAssetAction.h"

#include <Data/MounteaDialogueDataTable.h>

#define LOCTEXT_NAMESPACE "MounteaDialogueDataTableAssetAction"

FMounteaDialogueDataTableAssetAction::FMounteaDialogueDataTableAssetAction()
{
}

FText FMounteaDialogueDataTableAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueDataTableAssetAction_Name", "Mountea Dialogue Data Rows Table");
}

FColor FMounteaDialogueDataTableAssetAction::GetTypeColor() const
{
	return FColor(202, 30, 86);
}

UClass* FMounteaDialogueDataTableAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueDataTable::StaticClass();
}

uint32 FMounteaDialogueDataTableAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	}
	
	return  EAssetTypeCategories::Misc;
}

const TArray<FText>& FMounteaDialogueDataTableAssetAction::GetSubMenus() const
{
	static const TArray<FText> AssetTypeActionSubMenu
	{
		FText::FromString("2. Dialogue Data")
	};
	return AssetTypeActionSubMenu;
}

#undef LOCTEXT_NAMESPACE