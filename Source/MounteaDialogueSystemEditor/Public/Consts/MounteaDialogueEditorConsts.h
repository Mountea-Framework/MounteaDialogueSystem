// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

namespace MounteaDialogueConsts
{
	constexpr const TCHAR* Submenu_Decorators =		TEXT("1. Node Decorators");
	constexpr const TCHAR* Submenu_Conditions =		TEXT("2. Edge Conditions");
	constexpr const TCHAR* Submenu_Configurations =	TEXT("3. Configuration");
	constexpr const TCHAR* Submenu_DialogueData =	TEXT("4. Dialogue Data");
	constexpr const TCHAR* Submenu_NodeTemplates =	TEXT("5. Node Templates");
}

namespace MounteaAdvancedDialogueToolbar
{
	static const FName MounteaSharedMenuName("MounteaFramework.SharedMenu");
	static const FName MounteaSharedSectionName("MounteaFramework");
	static const FName MounteaSharedButtonName("MounteaFrameworkButton");
}

namespace MounteaDialogueWireConsts
{
	// Wire rendering
	constexpr float WireThickness     = 2.0f;
	constexpr float WireGridSize      = 1.0f;
	constexpr float MinStyleDistance  = 24.0f;
	constexpr float RoundRadius       = 10.0f;

	// Edge bubble sizing
	constexpr float BubbleSize        = 38.0f;
	constexpr float BubbleHalfSize    = BubbleSize * 0.5f;
	constexpr float BubbleClearance   = 16.0f;

	// Stub length = bubble height + clearance below bubble before arrowhead
	constexpr float WireStubOffset    = BubbleSize + BubbleClearance;

	// Y offset applied when placing the bubble widget above the end node
	constexpr float BubbleYOffset     = WireStubOffset;
}

namespace MounteaGraphLayoutStrategy
{
	constexpr int32 MaxIterations = 200;
	constexpr int32 OptimalHorizontalDistance = 300;
	constexpr int32 OptimalVerticalDistance = 200;
}

namespace MounteaPopup
{
	constexpr float PopupWidthRatio = 0.4f;
	constexpr float PopupHeightRatio = 0.6f;
}
