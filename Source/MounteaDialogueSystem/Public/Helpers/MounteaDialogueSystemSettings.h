// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueSystemSettings.generated.h"

/**
 * 
 */
UCLASS(config = MounteaSettings)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	UMounteaDialogueSystemSettings();

protected:

	/**
	 * User Widget class to be set as default one if requested.
	 * ❗Must implement MounteaDialogueWBPInterface❗
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "UserInterface", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface"))
	TSoftClassPtr<UUserWidget> DefaultDialogueWidgetClass;

#if WITH_EDITOR
	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("MounteaDialogueSystem", "MounteaSettingsSection", "Mountea Dialogue System");
	}

	virtual FText GetSectionDescription() const override
	{
		return NSLOCTEXT("MounteaDialogueSystem", "MounteaSettingsDescription", "Default values for Mountea Plugins.");
	}

	virtual FName GetContainerName() const override
	{
		return "Project";
	}
#endif

public:

	/**
	 * Returns Default Dialogue Widget if any is defined.
	 * ❗Might return Null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	TSoftClassPtr<UUserWidget> GetDefaultDialogueWidget() const
	{
		if (DefaultDialogueWidgetClass.IsNull())
		{
			return nullptr;
		}

		return  DefaultDialogueWidgetClass;
	}
};