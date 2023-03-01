// All rights reserved Dominik Pavlicek 2023


#include "MounteaDialogueGraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorSettings"

UMounteaDialogueGraphEditorSettings::UMounteaDialogueGraphEditorSettings()
{
	NodeType = ENodeType::ENT_SoftCorners;
	NodeTheme = ENodeTheme::ENT_DarkTheme;
	ArrowType = EArrowType::ERT_SimpleArrow;
	
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System (Editor)");
	
	AutoLayoutStrategy = EAutoLayoutStrategyType::EALS_Tree;

	bFirstPassOnly = false;
	bRandomInit = false;
	OptimalDistance = 100.f;
	MaxIteration = 50;
	InitTemperature = 10.f;
	CoolDownRate = 10.f;

	WireWidth = 1.5f;
	WireStyle = EWiringStyle::EWS_Simple;
	BubbleDrawRule = EBubbleDrawRule::EBDR_OnSelected;

	bAllowRenameNodes = true;
	bDisplayAutomaticNames = false;

	bShowDetailedInfo_InheritsDecorators = false;
	bShowDetailedInfo_NumDecorators = true;
	DecoratorsInfoStyle = EDecoratorsInfoStyle::EDIS_Unified;
}

#undef LOCTEXT_NAMESPACE