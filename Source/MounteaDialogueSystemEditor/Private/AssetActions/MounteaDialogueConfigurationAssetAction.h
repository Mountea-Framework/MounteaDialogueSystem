﻿// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UMounteaDialogueConfigurationAssetAction : public FAssetTypeActions_Base
{

public:
	UMounteaDialogueConfigurationAssetAction();

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual const TArray<FText>& GetSubMenus() const override;

};
