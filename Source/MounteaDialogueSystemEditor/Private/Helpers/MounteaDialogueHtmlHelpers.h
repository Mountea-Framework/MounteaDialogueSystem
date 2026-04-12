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

class FMounteaDialogueHtmlHelpers
{
public:
	static FString BuildBaseUrl(const FString& SourcePath);
	static FString InjectSharedAssets(const FString& HtmlContent);
	static bool LoadHtmlFile(const FString& FilePath, FString& OutHtml);
	static bool LoadOfflineChangelogHtml(FString& OutHtml, FString& OutPath);
	static bool SaveHtmlFile(const FString& FilePath, const FString& HtmlContent);

	static FString BuildChangelogDocument(const FString& ChangelogBodyHtml);
	static FString BuildChangelogFallbackMessage();
};
