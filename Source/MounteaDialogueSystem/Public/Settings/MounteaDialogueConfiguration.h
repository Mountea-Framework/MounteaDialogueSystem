// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Engine/DataAsset.h"
#include "MounteaDialogueConfiguration.generated.h"

USTRUCT(BlueprintType)
struct FMounteaDialogueGraphNodeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configuration",
		meta=(AllowAbstract))
	TSet<TSoftClassPtr<UMounteaDialogueGraphNode>> AllowedInputClasses;
};

/**
 * UMounteaDialogueConfiguration
 *
 * A data asset that provides configuration settings for the Mountea Dialogue System.
 * This includes settings for the default dialogue widget, input mode, dialogue skip behavior,
 * timing coefficients, and various other user interface and subtitle parameters.
 *
 * This configuration asset is designed to be customizable within the editor, allowing fine-tuned
 * control over dialogue behavior, including skip fade durations, update frequencies, and input handling.
 */
UCLASS(DisplayName="Mountea Dialogue Config")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueConfiguration : public UDataAsset
{
	GENERATED_BODY()

public:

	UMounteaDialogueConfiguration();

public:
	
	/**
	 * A DataTable asset that holds information about the dialogue participants in the Mountea Dialogue System.
	 *
	 * The asset must conform to the row structure specified by the `DialogueParticipant` structure to ensure proper data handling.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "🗣 Participants", 
		meta=(RequiredAssetDataTags = "RowStructure=/Script/MounteaDialogueSystem.DialogueParticipant"),
		meta=(NoResetToDefault))
	TArray<TSoftObjectPtr<UDataTable>> DialogueParticipantsTables;

	/**
	 * User Widget class to be set as default one if requested.
	 * ❗ Must implement MounteaDialogueWBPInterface❗
	 */
	UPROPERTY(EditDefaultsOnly, Category = "🖥 UserInterface", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface"))
	TSoftClassPtr<UUserWidget> DefaultDialogueWidgetClass;

	/**
	 * Default Z-order for the dialogue widget when added to screen via
	 * UMounteaDialogueParticipantUserInterfaceComponent.
	 * Higher values render on top of elements with lower Z-orders.
	 */
	UPROPERTY(EditDefaultsOnly,
		Category = "🖥 UserInterface",
		meta=(UIMin=0, ClampMin=0))
	int32 DefaultDialogueWidgetZOrder = 12;

	/**
	 * Sets Input mode when in Dialogue.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "🖥 UserInterface")
	EMounteaInputMode InputMode;
	
	/**
	 * Whether subtitles are allowed or not.
	 * If subtitles are not allowed, C++ requests won't request showing subtitles.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "💬 Subtitles")
	uint8 bAllowSubtitles : 1;

	// Defines how long a Skip Key must be held in order to start the Skip
	UPROPERTY(EditDefaultsOnly, Category = "🖥 UserInterface")
	float SkipDuration = 1.f;

	/**
	 * Defines whether whole Dialogue Row is skipped when audio skip is requested.
	 * This setting defines behaviour for all Nodes. Each Node allows different behaviour, so in special cases Node inversion can be used.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "🔊 Audio")
	uint8 bSkipRowWithAudioSkip : 1;

	/**
	 * Defines coefficient of speed per 100 characters for `Automatic` `RowDurationMode`.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "🖥 UserInterface")
	float DurationCoefficient = 8.f;
	
	/**
	 * Defines how often Dialogue Widgets update per second.
	 * Effectively can replaces Tick.
	 * ❔ Units: seconds
	 * ❗Lower the value higher the performance impact❗
	 */
	UPROPERTY(EditDefaultsOnly, Category = "💬 Subtitles", meta=(UIMin=0.01f, ClampMin=0.01f, UIMax=1.f, ClampMax=1.f, Units="seconds"))
	float UpdateFrequency = 0.05f;

	/**
	 * Defines fading duration to naturally stop voice when anything is playing.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "💬 Subtitles", meta=(UIMin=0.01f, ClampMin=0.01f, UIMax=1.f, ClampMax=1.f, Units="seconds"))
	float SkipFadeDuration = 0.01f;

	/**
	 * Defines the per-node type configuration of allowed classes.
	 * The list of per-node type configurations will be enhanced in future.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "⚙ Configuration",
		meta=(AllowAbstract))
	TMap<TSoftClassPtr<UMounteaDialogueGraphNode>, FMounteaDialogueGraphNodeConfig> NodesConfiguration;

	/**
	 * List of General Dialogue Settings.
	 * Defines font, sizes etc. for all subtitles.
	 * If any Widget is supposed to be overriden and use different setup for subtitles, just add that override to 'SubtitlesSettingsOverrides'.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "💬 Subtitles")
	FMounteaSubtitlesSettings SubtitlesSettings;

	/**
	 * Map of Widget Classes and their Subtitles Settings.
	 * Used for overriding General Defaults.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "💬 Subtitles")
	TMap<FUIRowID, FMounteaSubtitlesSettings> SubtitlesSettingsOverrides;

public:

	int32 GetDefaultDialogueWidgetZOrder() const { return DefaultDialogueWidgetZOrder; }

protected:

#if WITH_EDITOR
	static FSlateFontInfo SetupDefaultFontSettings();
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif


};
