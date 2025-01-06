// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorSettings"

UMounteaDialogueGraphEditorSettings::UMounteaDialogueGraphEditorSettings() : bAllowAutoGameplayTagsCheck(true)
{
	NodeType = ENodeCornerType::ENT_SoftCorners;
	NodeTheme = ENodeTheme::ENT_DarkTheme;
	ArrowType = EArrowType::ERT_HollowArrow;
	
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System (Editor)");
	
	AutoLayoutStrategy = EAutoLayoutStrategyType::EALS_Tree;

	bFirstPassOnly = false;
	bRandomInit = false;
	OptimalDistance = 100.f;
	MaxIteration = 50;
	InitTemperature = 10.f;
	CoolDownRate = 10.f;

	WireWidth = 0.8f;

	bAllowRenameNodes = true;
	bDisplayAutomaticNames = false;

	bShowDetailedInfo_InheritsDecorators = true;
	bShowDetailedInfo_NumDecorators = true;
	DecoratorsInfoStyle = EDecoratorsInfoStyle::EDIS_Unified;

	bAllowNativeDecoratorsEdit = false;
	bUseAdvancedWiring = true;

	bDisplayStandardNodes = true;
}

void UMounteaDialogueGraphEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphEditorSettings, AdvancedWiringConnectionTangent))
	{
		if (FMath::IsNearlyZero(AdvancedWiringConnectionTangent.Y, 0.01f))
			AdvancedWiringConnectionTangent.Y = 1.f;
	}
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

#undef LOCTEXT_NAMESPACE
