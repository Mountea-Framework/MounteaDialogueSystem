// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"

class UMounteaDialogueGraphNode;

/**
 * Reusable node header strip: accent-coloured background, type icon, and title text.
 *
 * Corner style is controlled by CornerBrushName:
 *   "MDSStyleSet.Node.Flat.RoundTop"  — top corners rounded (default, used in preview cards)
 *   "MDSStyleSet.Node.Flat.RoundLeft" — left corners rounded (used in picker dropdown rows)
 *   "MDSStyleSet.Node.Flat.RoundAll"  — all corners rounded
 */
class SMounteaDialogueNodeHeader : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMounteaDialogueNodeHeader)
		: _AccentColor(FSlateColor(FLinearColor::Black))
		, _TextColor(FSlateColor(FLinearColor::White))
		, _CornerBrushName("MDSStyleSet.Node.Flat.RoundTop")
		, _HorizontalPadding(10.f)
		, _VerticalPadding(6.f)
		{}

		/** Background / accent colour of the header band */
		SLATE_ATTRIBUTE(FSlateColor, AccentColor)

		/** Icon shown to the left of the title — use FMounteaDialogueGraphVisualTokens::GetNodeIconBrush() */
		SLATE_ATTRIBUTE(const FSlateBrush*, Icon)

		/** Node title text */
		SLATE_ATTRIBUTE(FText, Title)

		/** Foreground colour for both the icon and title text */
		SLATE_ATTRIBUTE(FSlateColor, TextColor)

		/** Name of the brush that controls which corners are rounded */
		SLATE_ARGUMENT(FName, CornerBrushName)

		/** Left/right inner padding */
		SLATE_ARGUMENT(float, HorizontalPadding)

		/** Top/bottom inner padding */
		SLATE_ARGUMENT(float, VerticalPadding)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Convenience factory: build a header directly from a runtime node pointer */
	static TSharedRef<SMounteaDialogueNodeHeader> FromNode(
		const UMounteaDialogueGraphNode* Node,
		FName CornerBrushName = "MDSStyleSet.Node.Flat.RoundTop",
		float HorizontalPadding = 10.f,
		float VerticalPadding   = 6.f);
};
