// All rights reserved Dominik Pavlicek 2023


#include "MounteaDialogueAdditionalDataAssetAction.h"
#include "Data/DialogueAdditionalData.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueAdditionalDataAssetAction"

FMounteaDialogueAdditionalDataAssetAction::FMounteaDialogueAdditionalDataAssetAction()
{}

FText FMounteaDialogueAdditionalDataAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueAdditionalDataAssetAction_Name", "2. Mountea Dialogue Additional Data");
}

FColor FMounteaDialogueAdditionalDataAssetAction::GetTypeColor() const
{
	//return FColor::Magenta;
	return FColor(202, 30, 86);
}

UClass* FMounteaDialogueAdditionalDataAssetAction::GetSupportedClass() const
{
	return UDialogueAdditionalData::StaticClass();
}

uint32 FMounteaDialogueAdditionalDataAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	}
	
	return  EAssetTypeCategories::Misc;
};

#undef LOCTEXT_NAMESPACE
