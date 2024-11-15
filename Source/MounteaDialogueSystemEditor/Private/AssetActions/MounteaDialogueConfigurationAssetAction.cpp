// All rights reserved Dominik Morse (Pavlicek) 2024


#include "MounteaDialogueConfigurationAssetAction.h"

#include "Settings/MounteaDialogueConfiguration.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueConfigurationAssetAction"

UMounteaDialogueConfigurationAssetAction::UMounteaDialogueConfigurationAssetAction()
{
}

FText UMounteaDialogueConfigurationAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueConfigurationAssetAction_Name", "Mountea Dialogue Configuration");
}

FColor UMounteaDialogueConfigurationAssetAction::GetTypeColor() const
{
	return FColor(69, 3, 55);
}

UClass* UMounteaDialogueConfigurationAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueConfiguration::StaticClass();
}

uint32 UMounteaDialogueConfigurationAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	}
	
	return  EAssetTypeCategories::Misc;
}

const TArray<FText>& UMounteaDialogueConfigurationAssetAction::GetSubMenus() const
{
	static const TArray<FText> AssetTypeActionSubMenu
	{
		FText::FromString("3. Configuration")
	};
	return AssetTypeActionSubMenu;
}

#undef LOCTEXT_NAMESPACE