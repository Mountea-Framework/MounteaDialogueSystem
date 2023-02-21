// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueSystemSettings.generated.h"

namespace MounteaDialogueWidgetCommands
{
	const FString CreateDialogueWidget		(TEXT("CreateDialogueWidget"));
	const FString ShowDialogueRow			(TEXT("ShowDialogueRow"));
	const FString UpdateDialogueRow			(TEXT("UpdateDialogueRow"));
	const FString HideDialogueRow				(TEXT("HideDialogueRow"));
	const FString AddDialogueOptions			(TEXT("AddDialogueOptions"));
	const FString RemoveDialogueOptions	(TEXT("RemoveDialogueOptions"));
}

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

	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles", meta=(UIMin=0.01f, ClampMin=0.01f, UIMax=1.f, ClampMax=1.f))
	float UpdateFrequency = 0.05f;

	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles")
	TSet<FString> DialogueWidgetCommands;

	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles")
	uint8 bAllowSubtitles : 1;
	
	UPROPERTY(config, EditDefaultsOnly, Category = "Subtitles", meta=(UIMin=8.f, ClampMin=8.f, UIMax=32.f, ClampMax=32.f))
	int32  DialogueSubtitlesSize = 22;

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
	bool SubtitlesAllowed() const
	{ return bAllowSubtitles; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	int32 GetDialogueSubtitlesSize() const
	{ return DialogueSubtitlesSize; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	float GetWidgetUpdateFrequency() const
	{ return UpdateFrequency; };

protected:

#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif
	
};