// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
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
#include "Engine/EngineTypes.h"
#include "MounteaDialogueEditorPageTypes.generated.h"

USTRUCT(BlueprintType)
struct FDialogueEditorPageConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Config")
	FText PageTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Config",
		meta = (FilePathFilter = "html"))
	FFilePath PageFile;

	FDialogueEditorPageConfig()
	{
	}

	FDialogueEditorPageConfig(const FText& InPageTitle, const FString& InPageFile)
	{
		PageTitle = InPageTitle;
		PageFile = FFilePath(InPageFile);
	}
};
