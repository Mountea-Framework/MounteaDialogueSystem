// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FMounteaDialogueGraphEditorStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static const FSlateBrush * GetBrush(FName PropertyName, const ANSICHAR* Specifier = NULL);
	static const FName& GetStyleSetName();

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
