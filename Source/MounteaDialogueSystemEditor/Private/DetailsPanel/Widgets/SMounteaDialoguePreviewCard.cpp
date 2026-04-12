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

#include "DetailsPanel/Widgets/SMounteaDialoguePreviewCard.h"

#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "SMounteaDialoguePreviewCard"

namespace
{
	constexpr float CardInnerBorder = 2.0f;
	constexpr float CardOuterRing = 2.0f;
	constexpr float HeaderHorizontalPadding = 14.0f;
	constexpr float HeaderVerticalPadding = 9.0f;
	constexpr float BodyHorizontalPadding = 14.0f;
	constexpr float BodyTopPadding = 12.0f;
	constexpr float BodyBottomPadding = 10.0f;

	FText BuildPreviewHeaderText(const int32 RowIndex)
	{
		return FText::FromString(FString::Printf(TEXT("[%d]"), RowIndex));
	}
}

void SMounteaDialoguePreviewCard::Construct(const FArguments& InArgs)
{
	RowIndex = InArgs._RowIndex;
	RowText = InArgs._RowText;
	bExpanded = InArgs._InitiallyExpanded;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
		.Padding(FMargin(CardOuterRing))
		.BorderBackgroundColor(this, &SMounteaDialoguePreviewCard::GetOuterBackgroundColor)
		[
			SNew(SBorder)
			.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
			.Padding(FMargin(CardInnerBorder))
			.BorderBackgroundColor(this, &SMounteaDialoguePreviewCard::GetInnerBorderColor)
			[
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
				.BorderBackgroundColor(this, &SMounteaDialoguePreviewCard::GetBodyBackgroundColor)
				.Padding(FMargin(0.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.ContentPadding(FMargin(0.f))
						.OnClicked(this, &SMounteaDialoguePreviewCard::OnHeaderClicked)
						[
							SNew(SBorder)
							.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundTop"))
							.Padding(FMargin(HeaderHorizontalPadding, HeaderVerticalPadding))
							.BorderBackgroundColor(this, &SMounteaDialoguePreviewCard::GetHeaderBackgroundColor)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(this, &SMounteaDialoguePreviewCard::GetHeaderGlyph)
									.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
									.ColorAndOpacity(this, &SMounteaDialoguePreviewCard::GetHeaderTextColor)
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
								[
									SNew(STextBlock)
									.Text(this, &SMounteaDialoguePreviewCard::GetHeaderText)
									.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
									.ColorAndOpacity(this, &SMounteaDialoguePreviewCard::GetHeaderTextColor)
								]
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.Visibility(this, &SMounteaDialoguePreviewCard::GetBodyVisibility)
						.Padding(FMargin(BodyHorizontalPadding, BodyTopPadding, BodyHorizontalPadding, BodyBottomPadding))
						[
							SNew(STextBlock)
							.Text(RowText)
							.AutoWrapText(true)
							.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeBody")
							.ColorAndOpacity(this, &SMounteaDialoguePreviewCard::GetBodyTextColor)
						]
					]
				]
			]
		]
	];
}

FReply SMounteaDialoguePreviewCard::OnHeaderClicked()
{
	bExpanded = !bExpanded;
	return FReply::Handled();
}

EVisibility SMounteaDialoguePreviewCard::GetBodyVisibility() const
{
	return bExpanded ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SMounteaDialoguePreviewCard::GetHeaderGlyph() const
{
	return bExpanded ? LOCTEXT("CollapseGlyph", "⌄") : LOCTEXT("ExpandGlyph", "›");
}

FText SMounteaDialoguePreviewCard::GetHeaderText() const
{
	return BuildPreviewHeaderText(RowIndex);
}

FSlateColor SMounteaDialoguePreviewCard::GetHeaderTextColor() const
{
	return FMounteaDialogueGraphVisualTokens::GetCardForeground();
}

FSlateColor SMounteaDialoguePreviewCard::GetBodyTextColor() const
{
	return FMounteaDialogueGraphVisualTokens::GetCardForeground().CopyWithNewOpacity(0.95f);
}

FSlateColor SMounteaDialoguePreviewCard::GetHeaderBackgroundColor() const
{
	return FAppStyle::Get().GetSlateColor("Colors.Header");
}

FSlateColor SMounteaDialoguePreviewCard::GetBodyBackgroundColor() const
{
	return FMounteaDialogueGraphVisualTokens::GetCardBackground();
}

FSlateColor SMounteaDialoguePreviewCard::GetOuterBackgroundColor() const
{
	return FAppStyle::Get().GetSlateColor("Colors.Panel");
}

FSlateColor SMounteaDialoguePreviewCard::GetInnerBorderColor() const
{
	return FAppStyle::Get().GetSlateColor("Colors.Recessed");
}

#undef LOCTEXT_NAMESPACE
