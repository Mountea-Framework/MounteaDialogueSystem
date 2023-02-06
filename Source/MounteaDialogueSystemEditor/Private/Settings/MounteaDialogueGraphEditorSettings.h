// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphEditorSettings.generated.h"

UENUM(BlueprintType)
enum class EAutoLayoutStrategyType : uint8
{
	EALS_Tree				UMETA(DisplayName="Tree"),
	EALS_ForceDirected	UMETA(DisplayName="Force Directed"),

	Default						UMETA(Hidden)
};

/**
 * Mountea Dialogue System global settings.
 */
UCLASS(config = MounteaSettings, meta = (DisplayName = "Mountea Dialogue System"))
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueGraphEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphEditorSettings();

private:	

	UPROPERTY(config, EditDefaultsOnly, Category = "AutoArrange")
	float OptimalDistance;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	EAutoLayoutStrategyType AutoLayoutStrategy;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	int32 MaxIteration;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	bool bFirstPassOnly;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	bool bRandomInit;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	float InitTemperature;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	float CoolDownRate;

#if WITH_EDITOR
	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("MounteaDialogueSystem", "MounteaSettingsDescription", "Mountea Dialogue System");
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
	
	float GetOptimalDistance() const
	{ return OptimalDistance; };

	EAutoLayoutStrategyType GetAutoLayoutStrategy() const
	{ return AutoLayoutStrategy; };

	int32 GetMaxIteration() const
	{ return MaxIteration; };
	
	bool IsFirstPassOnly() const
	{ return bFirstPassOnly; };

	bool IsRandomInit() const
	{ return bRandomInit; };

	float GetInitTemperature() const
	{ return InitTemperature; };

	float GetCoolDownRate() const
	{ return CoolDownRate; };
};

