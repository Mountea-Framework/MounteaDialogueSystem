// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class  FMounteaDialogueGraphAssetAction : public FAssetTypeActions_Base
{
public:
	FMounteaDialogueGraphAssetAction();

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;

	/*
	virtual FText GetAssetDescription(const FAssetData& AssetData) const override;
	*/
};
