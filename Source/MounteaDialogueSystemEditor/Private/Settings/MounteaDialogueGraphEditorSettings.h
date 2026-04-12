// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Definitions/MounteaDialogueEditorPageTypes.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueGraphEditorSettings.generated.h"

#pragma region Enums

UENUM(BlueprintType)
enum class EAutoLayoutStrategyType : uint8
{
	EALS_Tree				UMETA(DisplayName="Tree"),
	EALS_ForceDirected		UMETA(DisplayName="Force Directed"),

	Default					UMETA(Hidden)
};

#pragma endregion 

/**
 * Mountea Dialogue System global settings.
 */
UCLASS(config = MounteaSettings, DefaultConfig, ProjectUserConfig)
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueGraphEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphEditorSettings();

public:

	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("MounteaDialogueEditorSystem", "MounteaSettingsEditorSection", "Mountea Dialogue System (Editor)");
	}

	virtual FText GetSectionDescription() const override
	{
		return NSLOCTEXT("MounteaDialogueEditorSystem", "MounteaSettingsEditorDescription", "Default values for Mountea Plugins (Editor).");
	}

	virtual FName GetContainerName() const override
	{
		return "Project";
	}

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

protected:

#pragma region GraphNodes

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	bool bDisplayAutomaticNames;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	bool bAllowRenameNodes;

	/**
	 * URL for the Nodes Replacement configuration file.
	 * 
	 * This URL points to a remote file containing base Node Replacement definitions.
	 * The system will use this URL to download and apply the tags if allowed.
	 * Default: @link https://raw.githubusercontent.com/Mountea-Framework/MounteaDialogueSystem/refs/heads/master/Config/node_replacements.json
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings", AdvancedDisplay, meta=(ConfigRestartRequired=true))
	FString NodeReplacementURL = FString("https://raw.githubusercontent.com/Mountea-Framework/MounteaDialogueSystem/refs/heads/master/Config/node_replacements.json");

#pragma endregion

#pragma region GraphDecorators

	// Enables 'Edit' button for Native Code Decorators
	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	bool bAllowNativeDecoratorsEdit;

#pragma endregion 

#pragma region BlueprintNodes

	/**
	 * ❗ EXPERIMENTAL FEATURE
	 * If case of any compile issues TURN THIS ON
	 *
	 * If turned on custom K2 Nodes will contain additional details, which might break visual appearance
	 * and might not work well with plugins like `BlueprintAssist`.
	 */
	UPROPERTY(config, EditDefaultsOnly,  Category = "BlueprintNodes", meta=(ConfigRestartRequired=true))
	bool bDisplayStandardNodes;
	
#pragma endregion
	
#pragma region GraphArrange
	
	UPROPERTY(config, EditDefaultsOnly, Category = "AutoArrange")
	float OptimalHorizontalDistance;

	UPROPERTY(config, EditDefaultsOnly, Category = "AutoArrange")
	float OptimalVerticalDistance;
	
	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	EAutoLayoutStrategyType AutoLayoutStrategy;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	int32 MaxIteration;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	bool bFirstPassOnly;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	bool bRandomInit;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	float InitTemperature;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "AutoArrange")
	float CoolDownRate;

#pragma endregion

#pragma region GameplayTags

	/**
	 * Allow automatic gameplay tag checks.
	 * 
	 * If set to true, the system will automatically verify and update gameplay tags on engine startup.
	 * Default is True.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "GameplayTags", meta=(ConfigRestartRequired=true))
	uint8 bAllowAutoGameplayTagsCheck : 1;

	/**
	 * URL for the Gameplay Tags configuration file.
	 * 
	 * This URL points to a remote file containing base gameplay tag definitions.
	 * The system will use this URL to download and apply the tags if allowed.
	 * Default: @link https://raw.githubusercontent.com/Mountea-Framework/MounteaDialogueSystem/master/Config/Tags/MounteaDialogueSystemTags.ini
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "GameplayTags", AdvancedDisplay, meta=(ConfigRestartRequired=true))
	FString GameplayTagsURL = FString("https://raw.githubusercontent.com/Mountea-Framework/MounteaDialogueSystem/master/Config/Tags/MounteaDialogueSystemTags.ini");

#pragma endregion

#pragma region HelpPages

	/**
	 * Shared styling for editor HTML pages.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "HelpPages", AdvancedDisplay, meta = (FilePathFilter = "css"))
	FFilePath SharedStylesheetPath;

	/**
	 * Shared script for editor HTML pages.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "HelpPages", AdvancedDisplay, meta = (FilePathFilter = "js"))
	FFilePath SharedScriptPath;

	/**
	 * Help pages displayed in tutorial windows.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "HelpPages", AdvancedDisplay)
	TMap<int32, FDialogueEditorPageConfig> EditorTemplatePages;

#pragma endregion

public:

#pragma region GraphNodes_Getters
	
	bool ShowAutomaticNames() const
	{ return bDisplayAutomaticNames; };

	bool AllowRenameNodes() const
	{ return bAllowRenameNodes; };

	FString GetNodeReplacementURL() const
	{ return NodeReplacementURL; };

	FString GetNodeReplacementLocalPath() const;

#pragma endregion


#pragma region GraphDecorators_Getters

	bool IsNativeDecoratorsEditAllowed() const
	{ return bAllowNativeDecoratorsEdit; };

#pragma endregion 

#pragma region GraphArrange_Getters
	
	EAutoLayoutStrategyType GetAutoLayoutStrategy() const
	{ return AutoLayoutStrategy; };

	float GetOptimalHorizontalDistance() const
	{ return OptimalHorizontalDistance; };

	float GetOptimalVerticalDistance() const
	{ return OptimalVerticalDistance; };

	int32 GetMaxIteration() const
	{ return MaxIteration; };
	
	bool IsFirstPassOnly() const
	{ return bFirstPassOnly; };

	bool IsRandomInit() const
	{ return bRandomInit; };

	float GetInitTemperature() const
	{ return InitTemperature; };

	float GetCoolDownRate() const
	{ return CoolDownRate; };

#pragma endregion 

#pragma region BlueprintNodes_Getters

	bool DisplayStandardNodes() const
	{ return bDisplayStandardNodes; };;
	
#pragma endregion

#pragma region GameplayTags_Getters

	bool AllowCheckTagUpdate() const
	{ return bAllowAutoGameplayTagsCheck; };

	FString GetGameplayTagsURL() const
	{ return GameplayTagsURL; };
	
#pragma endregion

#pragma region HelpPages_Getters

	FString GetSharedStylesheetPath() const
	{ return SharedStylesheetPath.FilePath; };

	FString GetSharedScriptPath() const
	{ return SharedScriptPath.FilePath; };

	const TMap<int32, FDialogueEditorPageConfig>& GetEditorTemplatePages() const
	{ return EditorTemplatePages; };

	FString GetEditorTemplatePagePath(const int32 PageId) const;

	FText GetEditorTemplatePageTitle(const int32 PageId) const;

	FString GetOfflineChangelogPath() const;
	FString GetGeneratedChangelogPath() const;

#pragma endregion

private:

	void ReportLegacyVisualSettings() const;
 
};
