#pragma once

#include "Engine/DeveloperSettings.h"
#include "MDSPopupConfig.generated.h"

UCLASS(config = EditorPerProjectUserSettings)
class UMDSPopupConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMDSPopupConfig()
	{
	}

	UPROPERTY(config)
	FString PluginVersionUpdate = "";
	
};
