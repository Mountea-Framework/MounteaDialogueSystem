// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueGraphEditorSettings.generated.h"

#pragma region Enums

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

UENUM(BlueprintType)
enum class EDecoratorsInfoStyle : uint8
{
	EDSI_Stack				UMETA(DisplayName="Stack"),
	EDIS_Unified				UMETA(DisplayName="Unified")
};

UENUM(BlueprintType)
enum class ENodeType : uint8
{
	ENT_SoftCorners			UMETA(DisplayName="Soft Corners"),
	ENT_HardCorners			UMETA(DisplayName="Hard Corners")
};

UENUM(BlueprintType)
enum class EArrowType : uint8
{
	ERT_SimpleArrow			UMETA(DisplayName="Simple Arrow"),
	ERT_HollowArrow			UMETA(DisplayName="Hollow Arrow"),
	ERT_FancyArrow			UMETA(DisplayName="Fancy Arrow"),
	ERT_Bubble					UMETA(DisplayName="Bubble"),
	ERT_None					UMETA(DisplayName="Nothing")
};

#pragma endregion 

class UMounteaDialogueGraphNode;

/**
 * Mountea Dialogue System global settings.
 */
UCLASS(config = MounteaSettings)
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueGraphEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphEditorSettings();

private:

#pragma region GraphNodes

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	ENodeType NodeType;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	ENodeTheme NodeTheme;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings|DecoratorsInfo")
	uint8 bShowDetailedInfo_NumDecorators : 1;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings|DecoratorsInfo")
	uint8 bShowDetailedInfo_InheritsDecorators : 1;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodesSettings")
	EDecoratorsInfoStyle DecoratorsInfoStyle;
	
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
	 * If turned off, then standard Nodes will be hidden and Blueprint Graphs will display only custom K2 Nodes.
	 * Might cause issues if you create Blueprint Classes that directly implement Dialogue Interfaces.
	 */
	UPROPERTY(config, EditDefaultsOnly,  Category = "BlueprintNodes")
	bool bDisplayStandardNodes;
	
#pragma endregion
	
#pragma region GraphWiring
	
	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", meta=(UIMin=0.1f, ClampMin=0.1f, UIMax=1.5f, ClampMax=1.5f))
	float WireWidth;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring") //, meta=(ConfigRestartRequired=true))
	EArrowType ArrowType;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", meta=(ToolTip="[BETA] Feature]"))
	bool bUseAdvancedWiring;

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "NodeWiring", meta=(ToolTip="[BETA] Feature]", EditCondition="bUseAdvancedWiring"))
	FVector2D AdvancedWiringConnectionTangent = FVector2D(0.0f, 220.f);

	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category = "NodeWiring", meta=(ToolTip="[BETA] Feature]", EditCondition="bUseAdvancedWiring"))
	float ControlPointDistance = 150.0f;
	
	/* Advanced Wiring doesn't work now
	
	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	EWiringStyle WireStyle;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	uint32 HorizontalOffset = 16;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	EBubbleDrawRule BubbleDrawRule;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	int32 BubbleZoomThreshold;

	// Space between bubbles on the wires. Default: 20.0
	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta = (ClampMin = "10.0", ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	float BubbleSpace = 20.0f;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta = (ClampMin = "10.0", ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	float BubbleSize = 2.0f;

	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta = (ClampMin = "10.0", ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	float BubbleSpeed = 2.0f;
	
	// Disable the offset for pins. Default: false
	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	bool DisablePinOffset = false;

	// Fix default zoomed-out wire displacement. Default: true
	UPROPERTY(config, EditDefaultsOnly, Category = "NodeWiring", AdvancedDisplay, meta=(ToolTip="Work in Progress!", EditCondition="bUseAdvancedWiring"))
	bool FixZoomDisplacement = true;

	*/

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
#endif

public:

#pragma region GraphNodes_Getters
	
	ENodeTheme GetNodeTheme() const
	{ return NodeTheme; };

	ENodeType GetNodeType() const
	{ return NodeType; };

	bool ShowDetailedInfo_NumDecorators() const
	{ return bShowDetailedInfo_NumDecorators; };

	bool ShowDetailedInfo_InheritsDecorators() const
	{ return bShowDetailedInfo_InheritsDecorators; };
	
	bool ShowAutomaticNames() const
	{ return bDisplayAutomaticNames; };

	EDecoratorsInfoStyle GetDecoratorsStyle() const
	{ return DecoratorsInfoStyle; };

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

#pragma region GraphDecorators_Getters

	bool IsNativeDecoratorsEditAllowed() const
	{ return bAllowNativeDecoratorsEdit; };

#pragma endregion 
	

#pragma region GraphWiring_Getters

	float GetWireWidth() const
	{ return WireWidth; };

	EArrowType GetArrowType() const
	{ return ArrowType; };

	bool AllowAdvancedWiring() const
	{ return bUseAdvancedWiring; };

	FVector2D GetAdvancedWiringConnectionTangent() const
	{ return AdvancedWiringConnectionTangent; };

	float GetControlPointDistance() const
	{ return ControlPointDistance; };
	
	/*
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
	*/
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

#pragma region BlueprintNodes_Getters

	bool DisplayStandardNodes() const
	{ return bDisplayStandardNodes; };;
	
#pragma endregion
	
#pragma region EDITOR
	
#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
#endif
	
#pragma endregion 
};