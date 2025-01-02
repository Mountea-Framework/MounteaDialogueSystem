// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueSystemSettings.generated.h"

class UMounteaDialogueConfiguration;

namespace MounteaDialogueWidgetCommands
{
	const FString CreateDialogueWidget			(TEXT("CreateDialogueWidget"));
	const FString CloseDialogueWidget			(TEXT("CloseDialogueWidget"));
	const FString ShowDialogueRow				(TEXT("ShowDialogueRow"));
	const FString UpdateDialogueRow				(TEXT("UpdateDialogueRow"));
	const FString HideDialogueRow					(TEXT("HideDialogueRow"));
	const FString AddDialogueOptions				(TEXT("AddDialogueOptions"));
	const FString RemoveDialogueOptions		(TEXT("RemoveDialogueOptions"));
	const FString ShowSkipUI							(TEXT("ShowSkipUI"));
	const FString HideSkipUI								(TEXT("HideSkipUI"));
}

/**
 * Mountea Dialogue System Runtime Settigns.
 * 
 * Holds a list of settings that are used to further improve and tweak Dialogues.
 */
UCLASS(config = MounteaSettings, defaultconfig,ProjectUserConfig)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	UMounteaDialogueSystemSettings();

protected:
	/**
	 * Mountea Dialogue Configuration.
	 * Contains all gameplay 
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Configuration")
	TSoftObjectPtr<UMounteaDialogueConfiguration> DialogueConfiguration;

	/**
	 * List of Dialogue commands.
	 * Dialogue Commands are used to provide information what action should happen.
	 * ❔ Some values are hardcoded and cannot be deleted, thos are used for C++ requests
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles")
	TSet<FString> DialogueWidgetCommands;

	/**
	 * Defines logging level that is allowed to be shown.
	 * Affects on-screen messages.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Logging", meta=(Bitmask, BitmaskEnum="/Script/MounteaDialogueSystem.EMounteaDialogueLoggingVerbosity"))
	uint8 LogVerbosity;

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Getter"))
	TSoftObjectPtr<UMounteaDialogueConfiguration> GetDialogueConfiguration() const;

	/**
	 * Returns Default Dialogue Widget if any is defined.
	 * ❗ Might return Null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Getter"))
	TSoftClassPtr<UUserWidget> GetDefaultDialogueWidget() const;

	/**
	 * Returns whether skipping a dialogue row skips the whole row or only the audio.
	 * 
	 * @return True if skipping finishes the entire row, false if it only skips the audio.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Validate"))
	bool CanSkipWholeRow() const;

	/**
	 * Returns the current input mode used during dialogue.
	 * 
	 * @return The input mode (e.g., Game Only, UI Only, or Game and UI) set for dialogue interaction.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Getter"))
	EMounteaInputMode GetDialogueInputMode() const;

	/**
	 * Retrieves the duration coefficient used for automatic dialogue row progression.
	 * 
	 * @return The speed coefficient per 100 characters for the `Automatic` RowDurationMode.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Getter"))
	float GetDurationCoefficient() const;
	
	/**
	 * Returns whether subtitles are allowed or not.
	 * 
	 * @return True if subtitles are allowed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Validate"))
	bool SubtitlesAllowed() const;

	/**
	 * Returns the update frequency of the dialogue widgets.
	 * 
	 * @return The frequency, in seconds, at which the widgets are updated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CompactNodeTitle="Update Frequency", Keywords="update, refresh, tick, frequency"), meta=(CustomTag="MounteaK2Getter"))
	float GetWidgetUpdateFrequency() const;

	/**
	 * Retrieves the fade duration when skipping voice or audio in the dialogue.
	 * 
	 * @return The duration, in seconds, of the fade-out effect when audio is skipped.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CompactNodeTitle="SkipFadeDuration", Keywords="skip, fade, easy, smooth"), meta=(CustomTag="MounteaK2Getter"))
	float GetSkipFadeDuration() const;

	/**
	 * Returns Subtitles Settings.
	 * If given 'RowID' then it will search for Subtitles Settings override for this class, if any is specified.
	 * 
	 * If 'SubtitlesSettingsOverrides' are specified but invalid, 'SubtitlesSettings' are returned instead like no optional filters were provided.
	 * 
	 * @param RowID Optional row ID of the UserWidget for which to search for override settings.
	 * @return The subtitles settings for the given row or the default settings if no override is found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Getter"))
	FSubtitlesSettings GetSubtitlesSettings(const FUIRowID& RowID) const;

	/**
	 * Sets new subtitles settings for a specific widget or applies them globally.
	 * 
	 * If a valid `RowID` is provided, the new settings will override the subtitles settings for that widget. 
	 * If no `RowID` is specified or it is invalid, the new settings are applied globally to all widgets.
	 * 
	 * @param NewSettings The new subtitles settings to apply.
	 * @param RowID The row ID of the UserWidget for which to apply the settings. If not provided, the settings apply globally.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Setter"))
	void SetSubtitlesSettings(const FSubtitlesSettings& NewSettings, FUIRowID& RowID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Getter"))
	EMounteaDialogueLoggingVerbosity GetAllowedLoggVerbosity() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Settings", meta=(CustomTag="MounteaK2Getter"))
	float GetSkipDuration() const;
	
protected:

#if WITH_EDITOR
	static FSlateFontInfo SetupDefaultFontSettings();
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

#endif
	
};