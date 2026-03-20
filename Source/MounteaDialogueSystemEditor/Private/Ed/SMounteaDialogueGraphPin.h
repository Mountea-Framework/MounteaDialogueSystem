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
#include "SGraphPin.h"

class SMounteaDialogueGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SMounteaDialogueGraphPin) {}
		SLATE_ARGUMENT(FLinearColor, AccentColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

protected:
	virtual FSlateColor GetPinColor() const override;
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

private:
	const FSlateBrush* GetPinBorder() const;

private:
	FLinearColor AccentColor = FLinearColor::White;
};

