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

#include "Helpers/MounteaDialogueHtmlHelpers.h"

#include "Interfaces/IPluginManager.h"
#include "Logging/LogMacros.h"
#include "Misc/FileHelper.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

FString FMounteaDialogueHtmlHelpers::BuildBaseUrl(const FString& SourcePath)
{
	if(SourcePath.IsEmpty())
		return TEXT("about:blank");

	FString normalizedPath = FPaths::ConvertRelativePathToFull(SourcePath);
	normalizedPath = FPaths::GetPath(normalizedPath);
	normalizedPath = normalizedPath.Replace(TEXT("\\"), TEXT("/"));

	return FString::Printf(TEXT("file:///%s/"), *normalizedPath);
}

FString FMounteaDialogueHtmlHelpers::InjectSharedAssets(const FString& HtmlContent)
{
	const UMounteaDialogueGraphEditorSettings* settings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if(!settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Editor settings are not available."));
		return HtmlContent;
	}

	FString result = HtmlContent;

	if(!settings->GetSharedStylesheetPath().IsEmpty())
	{
		FString cssContent;
		const FString cssPath = FPaths::ConvertRelativePathToFull(settings->GetSharedStylesheetPath());
		if(FFileHelper::LoadFileToString(cssContent, *cssPath))
		{
			const FString styleTag = FString::Printf(TEXT("<style>%s</style>"), *cssContent);
			if(!result.Contains(styleTag))
			{
				const int32 headEndPos = result.Find(TEXT("</head>"), ESearchCase::IgnoreCase);
				if(headEndPos != INDEX_NONE)
					result.InsertAt(headEndPos, styleTag);
				else
					result = styleTag + result;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Failed to load shared stylesheet: %s"), *cssPath);
		}
	}

	if(!settings->GetSharedScriptPath().IsEmpty())
	{
		FString scriptContent;
		const FString scriptPath = FPaths::ConvertRelativePathToFull(settings->GetSharedScriptPath());
		if(FFileHelper::LoadFileToString(scriptContent, *scriptPath))
		{
			const FString scriptTag = FString::Printf(TEXT("<script>%s</script>"), *scriptContent);
			if(!result.Contains(scriptTag))
			{
				const int32 bodyEndPos = result.Find(TEXT("</body>"), ESearchCase::IgnoreCase);
				if(bodyEndPos != INDEX_NONE)
					result.InsertAt(bodyEndPos, scriptTag);
				else
					result += scriptTag;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Failed to load shared script: %s"), *scriptPath);
		}
	}

	return result;
}

bool FMounteaDialogueHtmlHelpers::LoadHtmlFile(const FString& FilePath, FString& OutHtml)
{
	OutHtml.Empty();
	if(FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Cannot load HTML file. File path is empty."));
		return false;
	}

	const FString normalizedPath = FPaths::ConvertRelativePathToFull(FilePath);
	const bool wasLoaded = FFileHelper::LoadFileToString(OutHtml, *normalizedPath);
	if(!wasLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Failed to load HTML file: %s"), *normalizedPath);
	}

	return wasLoaded;
}

bool FMounteaDialogueHtmlHelpers::LoadOfflineChangelogHtml(FString& OutHtml, FString& OutPath)
{
	OutHtml.Empty();
	OutPath.Empty();

	const UMounteaDialogueGraphEditorSettings* settings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if(!settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Editor settings are not available for offline changelog."));
		return false;
	}

	OutPath = settings->GetOfflineChangelogPath();
	if(OutPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Offline changelog path is empty."));
		return false;
	}

	const bool wasLoaded = FFileHelper::LoadFileToString(OutHtml, *OutPath);
	if(!wasLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MounteaDialogueHtmlHelpers] Failed to load offline changelog: %s"), *OutPath);
	}

	return wasLoaded;
}

FString FMounteaDialogueHtmlHelpers::BuildChangelogDocument(const FString& ChangelogBodyHtml)
{
	const FString headerHtml = TEXT(
		"<section class=\"doc-page changelog-page\">"
		"<header>"
		"<span class=\"version\">Mountea Dialogue System</span>"
		"<h1>What is New</h1>"
		"<p>Thanks for using Mountea Dialogue System. Here are the latest changes.</p>"
		"</header>"
	);

	const FString footerHtml = TEXT("</section>");

	return FString::Printf(
		TEXT("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>%s%s%s</body></html>"),
		*headerHtml,
		*ChangelogBodyHtml,
		*footerHtml
	);
}

FString FMounteaDialogueHtmlHelpers::BuildChangelogFallbackMessage()
{
	return TEXT(
		"<section class=\"doc-page changelog-page\">"
		"<header><h1>Offline Mode</h1><p>Unable to download the latest changelog right now.</p></header>"
		"<p>You can continue working normally. When online, the changelog updates automatically.</p>"
		"<p><a href=\"https://github.com/Mountea-Framework/MounteaDialogueSystem/blob/master/CHANGELOG.md\">Open changelog on GitHub</a></p>"
		"</section>"
	);
}
