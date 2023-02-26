#include "MounteaDialogueDecoratorAssetAction.h"

#include "Decorators/MounteaDialogueGraphNodeDecoratorBase.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecoratorAssetAction"

FMounteaDialogueDecoratorAssetAction::FMounteaDialogueDecoratorAssetAction()
{}

FText FMounteaDialogueDecoratorAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueDecoratorAssetAction_Name", "3. Mountea Dialogue Node Decorators");
}

FColor FMounteaDialogueDecoratorAssetAction::GetTypeColor() const
{
	return FColor::Green;
}

UClass* FMounteaDialogueDecoratorAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueGraphNodeDecoratorBase::StaticClass();
}

uint32 FMounteaDialogueDecoratorAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	}
	
	return  EAssetTypeCategories::Misc;
};

#undef LOCTEXT_NAMESPACE