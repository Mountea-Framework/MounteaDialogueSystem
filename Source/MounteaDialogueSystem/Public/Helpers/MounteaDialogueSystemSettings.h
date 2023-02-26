// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueSystemSettings.generated.h"

namespace MounteaDialogueWidgetCommands
{
	const FString CreateDialogueWidget		(TEXT("CreateDialogueWidget"));
	const FString CloseDialogueWidget		(TEXT("CloseDialogueWidget"));
	const FString ShowDialogueRow				(TEXT("ShowDialogueRow"));
	const FString UpdateDialogueRow			(TEXT("UpdateDialogueRow"));
	const FString HideDialogueRow				(TEXT("HideDialogueRow"));
	const FString AddDialogueOptions			(TEXT("AddDialogueOptions"));
	const FString RemoveDialogueOptions	(TEXT("RemoveDialogueOptions"));
}

/**
 * 
 */
UENUM(BlueprintType)
enum class EInputMode : uint8
{
	EIM_UIOnly			UMETA(DisplayName="UI Only"),
	EIM_UIAndGame	UMETA(DisplayName="UI & Game")
};

/**
 * Mountea Dialogue System Runtime Settigns.
 * 
 * Holds a list of settings that are used to further improve and tweak Dialogues.
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

	/**
	 * Sets Input mode when in Dialogue.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "UserInterface")
	EInputMode InputMode;
	
	/**
	 * Defines how often Dialogue Widgets update per second.
	 * Effectively can replaces Tick.
	 * ❔Units: seconds
	 * ❗Lower the value higher the performance impact❗
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles", meta=(UIMin=0.01f, ClampMin=0.01f, UIMax=1.f, ClampMax=1.f, Units="seconds"))
	float UpdateFrequency = 0.05f;

	/**
	 * List of Dialogue commands.
	 * Dialogue Commands are used to provide information what action should happen.
	 * ❔Some values are hardcoded and cannot be deleted, thos are used for C++ requests
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles")
	TSet<FString> DialogueWidgetCommands;

	/**
	 * Whether subtitles are allowed or not.
	 * If subtitles are not allowed, C++ requests won't request showing subtitles.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles")
	uint8 bAllowSubtitles : 1;

	/**
	 * List of General Dialogue Settings.
	 * Defines font, sizes etc. for all subtitles.
	 * If any Widget is supposed to be overriden and use different setup for subtitles, just add that override to 'SubtitlesSettingsOverrides'.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles")
	FSubtitlesSettings SubtitlesSettings;

	/**
	 * Map of Widget Classes and their Subtitles Settings.
	 * Used for overriding General Defaults.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles")
	TMap<FUIRowID, FSubtitlesSettings> SubtitlesSettingsOverrides;

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	EInputMode GetDialogueInputMode() const
	{ return InputMode; };
	
	/**
	 * Returns whether Subtitles are allowed or not.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	bool SubtitlesAllowed() const
	{ return bAllowSubtitles; };

	/**
	 * Returns Widget Update Frequency in seconds.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Update Frequency", Keywords="update, refresh, tick, frequency"))
	float GetWidgetUpdateFrequency() const
	{ return UpdateFrequency; };

	/**
	 * Returns Subtitles Settings.
	 * If given 'OptionalClassFilter' then it will search for Subtitles Settings override for this class, if any is specified.
	 * 
	 * @param RowID	Optional Class and Row ID of UserWidget to filter out the override Settings
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	FSubtitlesSettings GetSubtitlesSettings(const FUIRowID& RowID) const
	{ 
		if (SubtitlesSettingsOverrides.Contains(RowID))
		{
			return SubtitlesSettingsOverrides[RowID];
		}

		return SubtitlesSettings;
	};

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	void SetSubtitlesSettings(const FSubtitlesSettings& NewSettings, FUIRowID& RowID)
	{
		if (RowID.RowWidgetClass == nullptr)
		{
			SubtitlesSettings = NewSettings;
			return;
		}

		if (SubtitlesSettingsOverrides.Contains(RowID))
		{
			SubtitlesSettingsOverrides[RowID] = NewSettings;
			return;
		}

		SubtitlesSettingsOverrides.Add(RowID, NewSettings);
	}

protected:

#if WITH_EDITOR
	FSlateFontInfo SetupDefaultFontSettings() const;
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

#endif
	
};