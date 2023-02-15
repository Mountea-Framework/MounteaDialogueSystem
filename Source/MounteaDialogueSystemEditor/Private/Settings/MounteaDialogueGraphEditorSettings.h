// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphEditorSettings.generated.h"

UENUM(BlueprintType)
enum class EAutoLayoutStrategyType : uint8
{
	EALS_Tree				UMETA(DisplayName="Tree"),
	EALS_ForceDirected	UMETA(DisplayName="Force Directed"),

	Default						UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EWiringStyle : uint8
{
	EWS_Vanilla				UMETA(DisplayName="Vanilla"),
	EWS_Simple			UMETA(DisplayName="90° Angle"),
	EWS_Complex			UMETA(DisplayName="45° Angle"),

	Default						UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EBubbleDrawRule : uint8
{
	EBDR_Always			UMETA(DisplayName="Always"),
	EBDR_OnSelected	UMETA(DisplayName="When Selected")
};

UENUM(BlueprintType)
enum class ENodeTheme : uint8
{
	ENT_DarkTheme			UMETA(DisplayName="Dark Theme"),
	ENT_LightTheme			UMETA(DisplayName="Light Theme")
};

/**
 * Mountea Dialogue System global settings.
 */
UCLASS(config = MounteaSettings, meta = (DisplayName = "Mountea Dialogue System"))
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueGraphEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphEditorSettings();

private:

#pragma region GraphNodes

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	ENodeTheme NodeTheme;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	bool bDisplayAutomaticNames;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	bool bAllowRenameNodes;

	/**
	 * Select a Node Class and specify Override Colour for this Node type.
	 * Only non-abstract classes are allowed!
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings", meta=(ShowTreeView))
	TMap<TSoftClassPtr<UMounteaDialogueGraphNode>, FLinearColor> OverrideNodeBackgroundColours;

#pragma endregion 

#pragma region GraphWiring
	
	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings")
	float WireWidth;

	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta=(ToolTip="Work in Progress!"))
	bool bUseAdvancedWiring;
	
	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	EWiringStyle WireStyle;

	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	uint32 HorizontalOffset = 16;

	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	EBubbleDrawRule BubbleDrawRule;

	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	int32 BubbleZoomThreshold;

	/* Space between bubbles on the wires. Default: 20.0 */
	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta = (ClampMin = "10.0", ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	float BubbleSpace = 20.0f;

	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta = (ClampMin = "10.0", ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	float BubbleSize = 2.0f;

	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta = (ClampMin = "10.0", ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	float BubbleSpeed = 2.0f;
	
	/* Disable the offset for pins. Default: false */
	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	bool DisablePinOffset = false;

	/* Fix default zoomed-out wire displacement. Default: true */
	UPROPERTY(config, EditDefaultsOnly, Category = "GraphSettings", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	bool FixZoomDisplacement = true;

#pragma endregion

#pragma region GraphArrange
	
	UPROPERTY(config, EditDefaultsOnly, Category = "AutoArrange")
	float OptimalDistance;
	
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

#if WITH_EDITOR
	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("MounteaDialogueSystem", "MounteaSettingsDescription", "Mountea Dialogue System");
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

#pragma region GraphNodes_Getters

	ENodeTheme GetNodeTheme() const
	{ return NodeTheme; };
	
	bool ShowAutomaticNames() const
	{ return bDisplayAutomaticNames; };

	bool AllowRenameNodes() const
	{ return bAllowRenameNodes; };

	bool FindNodeBackgroundColourOverride(const TSoftClassPtr<UMounteaDialogueGraphNode> NodeClass, FLinearColor& BackgroundColour)
	{
		if (OverrideNodeBackgroundColours.Contains(NodeClass))
		{
			BackgroundColour = *OverrideNodeBackgroundColours.Find(NodeClass);
			return true;
		}

		return false;
	}

#pragma endregion 

#pragma region GraphWiring_Getters

	float GetWireWidth() const
	{ return WireWidth; };
	
	bool AllowAdvancedWiring() const
	{ return bUseAdvancedWiring; };
	
	EWiringStyle GetWireStyle() const
	{ return WireStyle; };

	int32 GetHorizontalOffset() const
	{ return HorizontalOffset; };

	EBubbleDrawRule GetBubbleDrawRule() const
	{ return  BubbleDrawRule; };

	int32 GetBubbleZoomThreshold() const
	{ return BubbleZoomThreshold; };

	float GetBubbleSpace() const
	{ return BubbleSpace; };

	float GetBubbleSpeed() const
	{ return BubbleSpeed; };

	float GetBubbleSize() const
	{ return BubbleSize; };

#pragma endregion 

#pragma region GraphArrange_Getters
	
	EAutoLayoutStrategyType GetAutoLayoutStrategy() const
	{ return AutoLayoutStrategy; };

	float GetOptimalDistance() const
	{ return OptimalDistance; };

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
};

