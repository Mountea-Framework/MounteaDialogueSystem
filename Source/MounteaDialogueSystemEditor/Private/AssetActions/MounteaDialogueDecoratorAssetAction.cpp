#include "MounteaDialogueDecoratorAssetAction.h"

#include "Decorators/MounteaDialogueDecoratorBase.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecoratorAssetAction"

FMounteaDialogueDecoratorAssetAction::FMounteaDialogueDecoratorAssetAction()
{}

FText FMounteaDialogueDecoratorAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueDecoratorAssetAction_Name", "Mountea Dialogue Node Decorators");
}

FColor FMounteaDialogueDecoratorAssetAction::GetTypeColor() const
{
	return FColor::Green;
}

UClass* FMounteaDialogueDecoratorAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueDecoratorBase::StaticClass();
}

uint32 FMounteaDialogueDecoratorAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	}
	
	return  EAssetTypeCategories::Misc;
}

const TArray<FText>& FMounteaDialogueDecoratorAssetAction::GetSubMenus() const
{
	static const TArray<FText> AssetTypeActionSubMenu
	{
		FText::FromString("1. Node Decorators")
	};
	return AssetTypeActionSubMenu;
};

#undef LOCTEXT_NAMESPACE