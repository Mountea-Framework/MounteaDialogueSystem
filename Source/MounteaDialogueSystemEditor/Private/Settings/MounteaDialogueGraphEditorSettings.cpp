// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphEditorSettings.h"

#include "Consts/MounteaDialogueEditorConsts.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorSettings"

UMounteaDialogueGraphEditorSettings::UMounteaDialogueGraphEditorSettings() : bAllowAutoGameplayTagsCheck(true)
{
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System (Editor)");

	auto resolvePluginPath = [](const FString& RelativePath) -> FString
	{
		const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
		if(!plugin.IsValid())
			return RelativePath;

		const FString fullPath = FPaths::Combine(plugin->GetBaseDir(), RelativePath);
		return FPaths::ConvertRelativePathToFull(fullPath);
	};
	
	AutoLayoutStrategy = EAutoLayoutStrategyType::EALS_Tree;

	bFirstPassOnly = false;
	bRandomInit = false;
	OptimalHorizontalDistance = MounteaGraphLayoutStrategy::OptimalHorizontalDistance;
	OptimalVerticalDistance = MounteaGraphLayoutStrategy::OptimalVerticalDistance;
	MaxIteration = MounteaGraphLayoutStrategy::MaxIterations;
	InitTemperature = 10.f;
	CoolDownRate = 10.f;

	bAllowRenameNodes = true;
	bDisplayAutomaticNames = false;

	bAllowNativeDecoratorsEdit = false;

	bDisplayStandardNodes = true;

	SharedStylesheetPath.FilePath = resolvePluginPath(TEXT("Resources/Help/DialogueEditorHelp.css"));
	SharedScriptPath.FilePath = resolvePluginPath(TEXT("Resources/Help/DialogueEditorHelp.js"));

	FDialogueEditorPageConfig welcomePage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_Welcome", "Welcome & Learning Path"),
		resolvePluginPath(TEXT("Resources/Help/page_0.html"))
	);

	FDialogueEditorPageConfig installationPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_Installation", "Installation & Plugin Enablement"),
		resolvePluginPath(TEXT("Resources/Help/page_1.html"))
	);

	FDialogueEditorPageConfig projectSettingsPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_ProjectSettings", "Project Settings Deep Dive"),
		resolvePluginPath(TEXT("Resources/Help/page_2.html"))
	);

	FDialogueEditorPageConfig gameplayTagsPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_GameplayTags", "Gameplay Tags Setup & Verification"),
		resolvePluginPath(TEXT("Resources/Help/page_3.html"))
	);

	FDialogueEditorPageConfig worldSetupPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_WorldSetup", "World Setup (GameMode/HUD/Participants)"),
		resolvePluginPath(TEXT("Resources/Help/page_4.html"))
	);

	FDialogueEditorPageConfig firstGraphPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_FirstGraph", "Building Your First Dialogue Graph"),
		resolvePluginPath(TEXT("Resources/Help/page_5.html"))
	);

	FDialogueEditorPageConfig uiIntegrationPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_UIIntegration", "UI Integration & Participant UI Flow"),
		resolvePluginPath(TEXT("Resources/Help/page_6.html"))
	);

	FDialogueEditorPageConfig advancedAuthoringPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_AdvancedAuthoring", "Advanced Authoring (Decorators, Conditions, Events)"),
		resolvePluginPath(TEXT("Resources/Help/page_7.html"))
	);

	FDialogueEditorPageConfig debuggingPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_Debugging", "Debugging & Troubleshooting"),
		resolvePluginPath(TEXT("Resources/Help/page_8.html"))
	);

	FDialogueEditorPageConfig productionChecklistPage(
		NSLOCTEXT("MounteaDialogueGraphEditorSettings", "EditorTemplatePages_ProductionChecklist", "Production Checklist & Useful Links"),
		resolvePluginPath(TEXT("Resources/Help/page_9.html"))
	);

	EditorTemplatePages.Add(0, welcomePage);
	EditorTemplatePages.Add(1, installationPage);
	EditorTemplatePages.Add(2, projectSettingsPage);
	EditorTemplatePages.Add(3, gameplayTagsPage);
	EditorTemplatePages.Add(4, worldSetupPage);
	EditorTemplatePages.Add(5, firstGraphPage);
	EditorTemplatePages.Add(6, uiIntegrationPage);
	EditorTemplatePages.Add(7, advancedAuthoringPage);
	EditorTemplatePages.Add(8, debuggingPage);
	EditorTemplatePages.Add(9, productionChecklistPage);

	ReportLegacyVisualSettings();
}

void UMounteaDialogueGraphEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

FString UMounteaDialogueGraphEditorSettings::GetNodeReplacementLocalPath() const
{
	FString pluginBaseDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("MounteaDialogueSystem"))
	);
	
	FString configPath = FPaths::Combine(
		pluginBaseDir,
		TEXT("Config"),
		TEXT("node_replacements.json")
	);
	
	// Convert to full path and normalize
	configPath = FPaths::ConvertRelativePathToFull(configPath);
	FPaths::NormalizeFilename(configPath);

	return configPath;
}

FString UMounteaDialogueGraphEditorSettings::GetEditorTemplatePagePath(const int32 PageId) const
{
	const FDialogueEditorPageConfig* found = EditorTemplatePages.Find(PageId);
	if(!found)
		return FString();

	return FPaths::ConvertRelativePathToFull(found->PageFile.FilePath);
}

FText UMounteaDialogueGraphEditorSettings::GetEditorTemplatePageTitle(const int32 PageId) const
{
	const FDialogueEditorPageConfig* found = EditorTemplatePages.Find(PageId);
	if(!found)
		return FText::GetEmpty();

	return found->PageTitle;
}

FString UMounteaDialogueGraphEditorSettings::GetOfflineChangelogPath() const
{
	const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	if(!plugin.IsValid())
		return FString();

	const FString offlinePath = FPaths::Combine(plugin->GetBaseDir(), TEXT("Resources/Help/changelog_offline.html"));
	return FPaths::ConvertRelativePathToFull(offlinePath);
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
			EditorLOG_WARNING(
				TEXT("[MounteaDialogueGraphEditorSettings] Deprecated visual key '%s' found in '%s'. This value is ignored."),
				key,
				*configFilename
			);
		}
	}
}

#undef LOCTEXT_NAMESPACE
