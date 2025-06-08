// All rights reserved Dominik Pavlicek 2023


#include "Settings/MounteaDialogueSystemSettings.h"

#include "Engine/Font.h"
#include "Settings/MounteaDialogueConfiguration.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemSettings"

UMounteaDialogueSystemSettings::UMounteaDialogueSystemSettings()
{
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System");

	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CreateDialogueWidget);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CloseDialogueWidget);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::ShowDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::UpdateDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::HideDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::AddDialogueOptions);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::RemoveDialogueOptions);

	LogVerbosity = 14; // hack it

#if WITH_EDITOR
	SubtitlesSettings.SubtitlesFont = SetupDefaultFontSettings();
	if (SubtitlesSettings.SettingsGUID.IsValid() == false)	SubtitlesSettings.SettingsGUID = FGuid::NewGuid();
#endif

}

TSoftObjectPtr<UMounteaDialogueConfiguration> UMounteaDialogueSystemSettings::GetDialogueConfiguration() const
{
	return DialogueConfiguration;
}

TSoftClassPtr<UUserWidget> UMounteaDialogueSystemSettings::GetDefaultDialogueWidget() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->DefaultDialogueWidgetClass : nullptr;
}

bool UMounteaDialogueSystemSettings::CanSkipWholeRow() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->bSkipRowWithAudioSkip : false;
}

EMounteaInputMode UMounteaDialogueSystemSettings::GetDialogueInputMode() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->InputMode : EMounteaInputMode::EIM_UIAndGame;
}

float UMounteaDialogueSystemSettings::GetDurationCoefficient() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->DurationCoefficient : 8.f;
}

bool UMounteaDialogueSystemSettings::SubtitlesAllowed() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->bAllowSubtitles : true;
}

float UMounteaDialogueSystemSettings::GetWidgetUpdateFrequency() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->UpdateFrequency : 0.2f;
}

float UMounteaDialogueSystemSettings::GetSkipFadeDuration() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->SkipFadeDuration : 0.05f;
}

FSubtitlesSettings UMounteaDialogueSystemSettings::GetSubtitlesSettings(const FUIRowID& RowID) const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	if (!dialogueConfig)
		return FSubtitlesSettings();
	
	if (dialogueConfig->SubtitlesSettingsOverrides.Contains(RowID))
	{
		return dialogueConfig->SubtitlesSettingsOverrides[RowID].SettingsGUID.IsValid() ? 
		dialogueConfig->SubtitlesSettingsOverrides[RowID] :
		dialogueConfig->SubtitlesSettings;
	}

	return dialogueConfig->SubtitlesSettings;
}

void UMounteaDialogueSystemSettings::SetSubtitlesSettings(const FSubtitlesSettings& NewSettings, FUIRowID& RowID)
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	if (!dialogueConfig)
		return;
	
	if (RowID.RowWidgetClass == nullptr)
		dialogueConfig->SubtitlesSettings = NewSettings;
	else
	{
		if (SubtitlesSettingsOverrides.Contains(RowID))
			dialogueConfig->SubtitlesSettingsOverrides[RowID] = NewSettings;
		else
			dialogueConfig->SubtitlesSettingsOverrides.Add(RowID, NewSettings);
	}

	SaveConfig();
}

EMounteaDialogueLoggingVerbosity UMounteaDialogueSystemSettings::GetAllowedLoggVerbosity() const
{
	return static_cast<EMounteaDialogueLoggingVerbosity>(LogVerbosity);
}

float UMounteaDialogueSystemSettings::GetSkipDuration() const
{
	auto dialogueConfig = DialogueConfiguration.LoadSynchronous();
	return dialogueConfig ? dialogueConfig->SkipDuration : 1.f;
}

#if WITH_EDITOR

FSlateFontInfo UMounteaDialogueSystemSettings::SetupDefaultFontSettings()
{
	FSlateFontInfo ReturnFontInfo;
	ReturnFontInfo.FontObject = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto"));

	ReturnFontInfo.TypefaceFontName = FName("Regular");
	ReturnFontInfo.Size = 16;
	ReturnFontInfo.OutlineSettings = FFontOutlineSettings(1);

	return ReturnFontInfo;
}

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
