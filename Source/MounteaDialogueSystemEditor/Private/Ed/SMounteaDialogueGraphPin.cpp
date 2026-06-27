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

#include "Ed/SMounteaDialogueGraphPin.h"

#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SMounteaDialogueGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	AccentColor = InArgs._AccentColor;
	bShowLabel = false;
	SetCursor(EMouseCursor::Default);
	GraphPinObj = InPin;
	check(GraphPinObj != nullptr);
	SetVisibility(EVisibility::Visible);

	SBorder::Construct(
		SBorder::FArguments()
		.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
		.OnMouseButtonDown(this, &SMounteaDialogueGraphPin::OnPinMouseDown)
		.Cursor(this, &SMounteaDialogueGraphPin::GetPinCursor)
		.Padding(FMargin(0.0f))
		[
			SNew(SBox)
			.WidthOverride(16.0f)
			.HeightOverride(16.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(this, &SMounteaDialogueGraphPin::GetPinBorder)
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SOverlay::Slot()
				.Padding(FMargin(3.0f))
				[
					SNew(SImage)
					.Image(this, &SMounteaDialogueGraphPin::GetPinBorder)
					.ColorAndOpacity(this, &SMounteaDialogueGraphPin::GetPinColor)
				]
			]
		]
	);
}

FSlateColor SMounteaDialogueGraphPin::GetPinColor() const
{
	return AccentColor;
}

TSharedRef<SWidget> SMounteaDialogueGraphPin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SMounteaDialogueGraphPin::GetPinBorder() const
{
	static TUniquePtr<FSlateImageBrush> pinCircleBrush;
	if (!pinCircleBrush.IsValid())
	{
		FString pinBrushPath;
		const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
		if (plugin.IsValid())
			pinBrushPath = plugin->GetBaseDir() / TEXT("Resources/CircleBox.png");
		else
			pinBrushPath = TEXT("Plugins/MounteaDialogueSystem/Resources/CircleBox.png");

		pinCircleBrush = MakeUnique<FSlateImageBrush>(pinBrushPath, FVector2D(16.0f, 16.0f));
	}

	return pinCircleBrush.Get();
}
