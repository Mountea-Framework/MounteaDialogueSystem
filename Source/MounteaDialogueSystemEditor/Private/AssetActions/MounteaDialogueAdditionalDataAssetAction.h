// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class  FMounteaDialogueAdditionalDataAssetAction : public FAssetTypeActions_Base
{
public:
	FMounteaDialogueAdditionalDataAssetAction();

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual const TArray<FText>& GetSubMenus() const override;

	/*
	virtual FText GetAssetDescription(const FAssetData& AssetData) const override;
	*/
};
