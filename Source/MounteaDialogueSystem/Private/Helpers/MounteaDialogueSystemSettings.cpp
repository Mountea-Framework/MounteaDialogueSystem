// All rights reserved Dominik Pavlicek 2023


#include "Helpers/MounteaDialogueSystemSettings.h"

#include "Engine/Font.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemSettings"

UMounteaDialogueSystemSettings::UMounteaDialogueSystemSettings()
{
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System");

	InputMode = EInputMode::EIM_UIOnly;
	bAllowSubtitles = true;

	UpdateFrequency = 0.05f;

	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CreateDialogueWidget);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CloseDialogueWidget);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::ShowDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::UpdateDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::HideDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::AddDialogueOptions);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::RemoveDialogueOptions);

	SubtitlesSettings.SubtitlesFont = SetupDefaultFontSettings();
	if (SubtitlesSettings.SettingsGUID.IsValid() == false)	SubtitlesSettings.SettingsGUID = FGuid::NewGuid();
}

#if WITH_EDITOR

void UMounteaDialogueSystemSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueSystemSettings, DialogueWidgetCommands))
	{
		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::CreateDialogueWidget) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CreateDialogueWidget);

		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::CloseDialogueWidget) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CloseDialogueWidget);
		
		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::ShowDialogueRow) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::ShowDialogueRow);

		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::UpdateDialogueRow) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::UpdateDialogueRow);

		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::HideDialogueRow) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::HideDialogueRow);

		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::AddDialogueOptions) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::AddDialogueOptions);

		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::RemoveDialogueOptions) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::RemoveDialogueOptions);
	}
}

FSlateFontInfo UMounteaDialogueSystemSettings::SetupDefaultFontSettings() const
{
	FSlateFontInfo ReturnFontInfo;
	ReturnFontInfo.FontObject = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto"));

	ReturnFontInfo.TypefaceFontName = FName("Regular");
	ReturnFontInfo.Size = 16;
	ReturnFontInfo.OutlineSettings = FFontOutlineSettings(1);

	return ReturnFontInfo;
}

void UMounteaDialogueSystemSettings::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueSystemSettings, SubtitlesSettings))
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

	if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueSystemSettings, SubtitlesSettingsOverrides))
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

#undef LOCTEXT_NAMESPACE
