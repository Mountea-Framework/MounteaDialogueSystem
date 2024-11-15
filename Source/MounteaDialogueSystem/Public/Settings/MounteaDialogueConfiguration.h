// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Engine/DataAsset.h"
#include "MounteaDialogueConfiguration.generated.h"

/**
 * 
 */
UCLASS(DisplayName="Mountea Dialogue Config")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueConfiguration : public UDataAsset
{
	GENERATED_BODY()

public:

	UMounteaDialogueConfiguration();

public:

	/**
	 * User Widget class to be set as default one if requested.
	 * ❗ Must implement MounteaDialogueWBPInterface❗
	 */
	UPROPERTY(EditDefaultsOnly, Category = "UserInterface", meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface"))
	TSoftClassPtr<UUserWidget> DefaultDialogueWidgetClass;

	/**
	 * Sets Input mode when in Dialogue.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "UserInterface")
	EInputMode InputMode;

	/**
	 * Defines whether whole Dialogue Row is skipped when audio skip is requested.
	 * This setting defines behaviour for all Nodes. Each Node allows different behaviour, so in special cases Node inversion can be used.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	uint8 bSkipRowWithAudioSkip : 1;

	/**
	 * Defines coefficient of speed per 100 characters for `Automatic` `RowDurationMode`.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "UserInterface")
	float DurationCoefficient = 8.f;
	
	/**
	 * Defines how often Dialogue Widgets update per second.
	 * Effectively can replaces Tick.
	 * ❔ Units: seconds
	 * ❗Lower the value higher the performance impact❗
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Subtitles", meta=(UIMin=0.01f, ClampMin=0.01f, UIMax=1.f, ClampMax=1.f, Units="seconds"))
	float UpdateFrequency = 0.05f;

	/**
	 * Defines fading duration to naturally stop voice when anything is playing.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Subtitles", meta=(UIMin=0.01f, ClampMin=0.01f, UIMax=1.f, ClampMax=1.f, Units="seconds"))
	float SkipFadeDuration = 0.01f;
	
};
