// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Settings/MounteaDialogueConfiguration.h"

UMounteaDialogueConfiguration::UMounteaDialogueConfiguration() :
	InputMode(EInputMode::EIM_UIAndGame),
	bAllowSubtitles(true),
	bSkipRowWithAudioSkip(false)
{
#if WITH_EDITOR
	if (SubtitlesSettings.SettingsGUID.IsValid() == false)	SubtitlesSettings.SettingsGUID = FGuid::NewGuid();
#endif
}

#if WITH_EDITOR
FSlateFontInfo UMounteaDialogueConfiguration::SetupDefaultFontSettings()
{
	FSlateFontInfo ReturnFontInfo;
	ReturnFontInfo.FontObject = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto"));

	ReturnFontInfo.TypefaceFontName = FName("Regular");
	ReturnFontInfo.Size = 16;
	ReturnFontInfo.OutlineSettings = FFontOutlineSettings(1);

	return ReturnFontInfo;
}

void UMounteaDialogueConfiguration::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueConfiguration, SubtitlesSettings))
	{
		if (SubtitlesSettings.SubtitlesFont.FontObject == nullptr)
		{
			SubtitlesSettings.SettingsGUID.Invalidate();
			SubtitlesSettings.SubtitlesFont = SetupDefaultFontSettings();
			if (SubtitlesSettings.SettingsGUID.IsValid() == false)	SubtitlesSettings.SettingsGUID = FGuid::NewGuid();
		}
		else
		{
			if (SubtitlesSettings.SettingsGUID.IsValid() == false)
			{
				SubtitlesSettings.SettingsGUID = FGuid::NewGuid();
			}
		}
	}

	if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueConfiguration, SubtitlesSettingsOverrides))
	{
		for (auto& Itr : SubtitlesSettingsOverrides)
		{
			if (Itr.Value.SubtitlesFont.FontObject == nullptr)
			{
				Itr.Value.SettingsGUID.Invalidate();
				Itr.Value.SubtitlesFont = SetupDefaultFontSettings();
				if (Itr.Value.SettingsGUID.IsValid() == false)	Itr.Value.SettingsGUID = FGuid::NewGuid();
			}
			else
			{
				if (Itr.Value.SettingsGUID.IsValid() == false)
				{
					Itr.Value.SettingsGUID = FGuid::NewGuid();
				}
			}
		}
	}
}
#endif
