// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorSettings"

UMounteaDialogueGraphEditorSettings::UMounteaDialogueGraphEditorSettings() : bAllowAutoGameplayTagsCheck(true)
{
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System (Editor)");
	
	AutoLayoutStrategy = EAutoLayoutStrategyType::EALS_Tree;

	bFirstPassOnly = false;
	bRandomInit = false;
	OptimalHorizontalDistance = 300.f;
	OptimalVerticalDistance = 200.f;
	MaxIteration = 100;
	InitTemperature = 10.f;
	CoolDownRate = 10.f;

	bAllowRenameNodes = true;
	bDisplayAutomaticNames = false;

	bAllowNativeDecoratorsEdit = false;

	bDisplayStandardNodes = true;

	ReportLegacyVisualSettings();
}

void UMounteaDialogueGraphEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

FString UMounteaDialogueGraphEditorSettings::GetNodeReplacementLocalPath() const
{
	FString PluginBaseDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("MounteaDialogueSystem"))
	);
	
	FString ConfigPath = FPaths::Combine(
		PluginBaseDir,
		TEXT("Config"),
		TEXT("node_replacements.json")
	);
	
	// Convert to full path and normalize
	ConfigPath = FPaths::ConvertRelativePathToFull(ConfigPath);
	FPaths::NormalizeFilename(ConfigPath);

	return ConfigPath;
}

void UMounteaDialogueGraphEditorSettings::ReportLegacyVisualSettings() const
{
	if (!GConfig)
		return;

	const FString configFilename = GetDefaultConfigFilename();
	const TCHAR* section = TEXT("/Script/MounteaDialogueSystemEditor.MounteaDialogueGraphEditorSettings");
	const TArray<const TCHAR*> deprecatedKeys =
	{
		TEXT("NodeType"),
		TEXT("NodeTheme"),
		TEXT("DecoratorsInfoStyle"),
		TEXT("bShowDetailedInfo_NumDecorators"),
		TEXT("bShowDetailedInfo_InheritsDecorators"),
		TEXT("OverrideNodeBackgroundColours"),
		TEXT("WireWidth"),
		TEXT("ArrowType"),
		TEXT("bUseAdvancedWiring"),
		TEXT("AdvancedWiringConnectionTangent"),
		TEXT("ControlPointDistance")
	};

	for (const TCHAR* key : deprecatedKeys)
	{
		FString deprecatedValue;
		if (GConfig->GetString(section, key, deprecatedValue, configFilename))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[MounteaDialogueGraphEditorSettings] Deprecated visual key '%s' found in '%s'. This value is ignored."),
				key,
				*configFilename
			);
		}
	}
}

#undef LOCTEXT_NAMESPACE
