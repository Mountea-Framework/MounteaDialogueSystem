// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.

#include "Slate/SMounteaDialogueNodeHeader.h"

#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Nodes/MounteaDialogueGraphNode.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SMounteaDialogueNodeHeader::Construct(const FArguments& InArgs)
{
	const FName cornerBrush   = InArgs._CornerBrushName;
	const float hPad          = InArgs._HorizontalPadding;
	const float vPad          = InArgs._VerticalPadding;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush(cornerBrush))
		.BorderBackgroundColor(InArgs._AccentColor)
		.Padding(FMargin(hPad, vPad))
		[
			SNew(SHorizontalBox)

			// Icon
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
			[
				SNew(SImage)
				.Image(InArgs._Icon)
				.ColorAndOpacity(InArgs._TextColor)
				.DesiredSizeOverride(FVector2D(14.f, 14.f))
			]

			// Title
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(InArgs._Title)
				.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
				.ColorAndOpacity(InArgs._TextColor)
			]
		]
	];
}

TSharedRef<SMounteaDialogueNodeHeader> SMounteaDialogueNodeHeader::FromNode(
	const UMounteaDialogueGraphNode* Node,
	FName CornerBrushName,
	float HorizontalPadding,
	float VerticalPadding)
{
	return SNew(SMounteaDialogueNodeHeader)
		.AccentColor(FSlateColor(FMounteaDialogueGraphVisualTokens::GetNodeAccentColor(Node)))
		.Icon(FMounteaDialogueGraphVisualTokens::GetNodeIconBrush(Node))
		.Title(Node ? Node->GetNodeTitle() : FText::FromString(TEXT("INVALID")))
		.TextColor(FSlateColor(FMounteaDialogueGraphVisualTokens::GetNodeHeaderForeground(Node)))
		.CornerBrushName(CornerBrushName)
		.HorizontalPadding(HorizontalPadding)
		.VerticalPadding(VerticalPadding);
}
