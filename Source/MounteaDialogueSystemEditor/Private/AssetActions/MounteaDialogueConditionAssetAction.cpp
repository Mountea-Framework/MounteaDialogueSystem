// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "MounteaDialogueConditionAssetAction.h"

#include "Conditions/MounteaDialogueConditionBase.h"
#include "Consts/MounteaDialogueEditorConsts.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueConditionAssetAction"

FMounteaDialogueConditionAssetAction::FMounteaDialogueConditionAssetAction()
{}

FText FMounteaDialogueConditionAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueConditionAssetAction_Name", "Mountea Dialogue Edge Condition");
}

FColor FMounteaDialogueConditionAssetAction::GetTypeColor() const
{
	return FColor::Yellow;
}

UClass* FMounteaDialogueConditionAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueConditionBase::StaticClass();
}

uint32 FMounteaDialogueConditionAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	
	return  EAssetTypeCategories::Misc;
}

const TArray<FText>& FMounteaDialogueConditionAssetAction::GetSubMenus() const
{
	static const TArray<FText> AssetTypeActionSubMenu
	{
		FText::FromString(MounteaDialogueConsts::Submenu_Conditions)
	};
	return AssetTypeActionSubMenu;
};

#undef LOCTEXT_NAMESPACE