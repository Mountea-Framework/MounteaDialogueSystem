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

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SMounteaDialoguePreviewCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMounteaDialoguePreviewCard)
		: _RowIndex(INDEX_NONE)
		, _InitiallyExpanded(false)
	{}
		SLATE_ARGUMENT(int32, RowIndex)
		SLATE_ARGUMENT(FText, RowText)
		SLATE_ARGUMENT(bool, InitiallyExpanded)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnHeaderClicked();
	EVisibility GetBodyVisibility() const;
	FText GetHeaderGlyph() const;
	FText GetHeaderText() const;
	FSlateColor GetHeaderTextColor() const;
	FSlateColor GetBodyTextColor() const;
	FSlateColor GetHeaderBackgroundColor() const;
	FSlateColor GetBodyBackgroundColor() const;
	FSlateColor GetOuterBackgroundColor() const;
	FSlateColor GetInnerBorderColor() const;

private:
	bool bExpanded = false;
	int32 RowIndex = INDEX_NONE;
	FText RowText;
};
