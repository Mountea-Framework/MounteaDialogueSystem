// All rights reserved Dominik Pavlicek 2023


#include "Helpers/MounteaDialogueSystemSettings.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemSettings"

UMounteaDialogueSystemSettings::UMounteaDialogueSystemSettings()
{
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System");

	bAllowSubtitles = true;
	DialogueSubtitlesSize = 22;

	UpdateFrequency = 0.05f;

	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CreateDialogueWidget);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::ShowDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::UpdateDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::HideDialogueRow);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::AddDialogueOptions);
	DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::RemoveDialogueOptions);
}

#if WITH_EDITOR

void UMounteaDialogueSystemSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueSystemSettings, DialogueWidgetCommands))
	{
		if (DialogueWidgetCommands.Contains(MounteaDialogueWidgetCommands::CreateDialogueWidget) == false)
			DialogueWidgetCommands.Add(MounteaDialogueWidgetCommands::CreateDialogueWidget);
		
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

#endif

#undef LOCTEXT_NAMESPACE
