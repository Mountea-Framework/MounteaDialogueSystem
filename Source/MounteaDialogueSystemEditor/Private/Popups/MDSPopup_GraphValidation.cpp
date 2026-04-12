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

#include "MDSPopup_GraphValidation.h"

#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Slate/SMounteaDialogueHtmlView.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Containers/Ticker.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SWindow.h"

namespace
{
	constexpr float PopupWidthRatio = 0.3f;
	constexpr float PopupHeightRatio = 0.8f;
	const TCHAR* ValidationTemplateRelativePath = TEXT("Resources/Help/graph_validation.html");

	FVector2f GetPopupClientSize(const TSharedPtr<SWindow>& Window)
	{
		FDisplayMetrics displayMetrics;
		FSlateApplication::Get().GetDisplayMetrics(displayMetrics);

		FPlatformRect workArea = displayMetrics.PrimaryDisplayWorkAreaRect;
		if(Window.IsValid())
		{
			const FSlateRect windowRect = Window->GetRectInScreen();
			const FVector2D windowCenter(
				(windowRect.Left + windowRect.Right) * 0.5,
				(windowRect.Top + windowRect.Bottom) * 0.5
			);

			workArea = displayMetrics.GetMonitorWorkAreaFromPoint(windowCenter);
		}

		const float workAreaWidth = static_cast<float>(workArea.Right - workArea.Left);
		const float workAreaHeight = static_cast<float>(workArea.Bottom - workArea.Top);
		return FVector2f(workAreaWidth * PopupWidthRatio, workAreaHeight * PopupHeightRatio);
	}

	FString EscapeHtml(const FString& InputValue)
	{
		FString outputValue = InputValue;
		outputValue.ReplaceInline(TEXT("&"), TEXT("&amp;"));
		outputValue.ReplaceInline(TEXT("<"), TEXT("&lt;"));
		outputValue.ReplaceInline(TEXT(">"), TEXT("&gt;"));
		outputValue.ReplaceInline(TEXT("\""), TEXT("&quot;"));
		outputValue.ReplaceInline(TEXT("'"), TEXT("&#39;"));
		return outputValue;
	}

	FString ToHtmlTextPreserveLines(const FText& SourceText)
	{
		FString htmlSafeText = EscapeHtml(SourceText.ToString());
		htmlSafeText.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
		htmlSafeText.ReplaceInline(TEXT("\r"), TEXT("\n"));
		htmlSafeText.ReplaceInline(TEXT("\n"), TEXT("<br />"));
		return htmlSafeText;
	}

	FString BuildValidationIssueItems(const TArray<FText>& Messages, const FString& ItemClass)
	{
		FString itemsHtml;
		for(const FText& message : Messages)
		{
			itemsHtml += FString::Printf(TEXT("<li class=\"validation-item %s\">%s</li>"), *ItemClass, *ToHtmlTextPreserveLines(message));
		}
		return itemsHtml;
	}

	FString BuildValidationSection(const FString& Title, int32 Count, const TArray<FText>& Messages, const FString& SectionClass, const FString& ItemClass)
	{
		if(Messages.Num() == 0)
			return FString();

		const FString itemsHtml = BuildValidationIssueItems(Messages, ItemClass);
		return FString::Printf(
			TEXT("<section class=\"validation-section %s\"><div class=\"validation-section-header\"><span class=\"validation-section-title\">%s</span><span class=\"validation-section-count\">%d</span></div><ol class=\"validation-list validation-list--scroll\">%s</ol></section>"),
			*SectionClass,
			*EscapeHtml(Title),
			Count,
			*itemsHtml
		);
	}

	FString BuildValidationResultsHtml(const TArray<FText>& Errors, const TArray<FText>& Warnings)
	{
		const bool bHasErrors = Errors.Num() > 0;
		const bool bHasWarnings = Warnings.Num() > 0;

		if(!bHasErrors && !bHasWarnings)
			return TEXT("<section class=\"validation-empty\"><p>No validation errors or warnings were reported for this graph.</p></section>");

		FString resultsHtml;
		if(bHasErrors)
			resultsHtml += BuildValidationSection(TEXT("Errors"), Errors.Num(), Errors, TEXT("validation-section--error"), TEXT("validation-item--error"));

		if(bHasWarnings)
			resultsHtml += BuildValidationSection(TEXT("Warnings"), Warnings.Num(), Warnings, TEXT("validation-section--warning"), TEXT("validation-item--warning"));

		return resultsHtml;
	}

	FString GetValidationTemplatePath()
	{
		const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
		if(!plugin.IsValid())
			return FString();

		const FString templatePath = FPaths::Combine(plugin->GetBaseDir(), ValidationTemplateRelativePath);
		return FPaths::ConvertRelativePathToFull(templatePath);
	}

	bool BuildValidationHtmlFromTemplate(const TArray<FText>& Errors, const TArray<FText>& Warnings, FString& OutHtml, FString& OutSourcePath)
	{
		OutHtml.Empty();
		OutSourcePath = GetValidationTemplatePath();
		if(OutSourcePath.IsEmpty())
			return false;

		if(!FPaths::FileExists(OutSourcePath))
			return false;

		FString templateHtml;
		if(!FFileHelper::LoadFileToString(templateHtml, *OutSourcePath))
			return false;

		const int32 errorCount = Errors.Num();
		const int32 warningCount = Warnings.Num();
		const bool bHasErrors = errorCount > 0;
		const bool bHasWarnings = warningCount > 0;

		const FString statusClass = bHasErrors
			? TEXT("validation-chip--error")
			: (bHasWarnings ? TEXT("validation-chip--warning") : TEXT("validation-chip--valid"));
		const FString statusText = bHasErrors
			? TEXT("Invalid")
			: (bHasWarnings ? TEXT("Warnings") : TEXT("Valid"));
		const FString resultsHtml = BuildValidationResultsHtml(Errors, Warnings);

		templateHtml.ReplaceInline(TEXT("__MDS_ERROR_COUNT__"), *FString::FromInt(errorCount), ESearchCase::CaseSensitive);
		templateHtml.ReplaceInline(TEXT("__MDS_WARNING_COUNT__"), *FString::FromInt(warningCount), ESearchCase::CaseSensitive);
		templateHtml.ReplaceInline(TEXT("__MDS_STATUS_CLASS__"), *statusClass, ESearchCase::CaseSensitive);
		templateHtml.ReplaceInline(TEXT("__MDS_STATUS_TEXT__"), *EscapeHtml(statusText), ESearchCase::CaseSensitive);

		if(templateHtml.Contains(TEXT("__MDS_RESULTS_SECTION__")))
			templateHtml.ReplaceInline(TEXT("__MDS_RESULTS_SECTION__"), *resultsHtml, ESearchCase::CaseSensitive);
		else
		{
			const int32 actionSectionPos = templateHtml.Find(TEXT("<section class=\"card validation-actions\""), ESearchCase::IgnoreCase);
			if(actionSectionPos != INDEX_NONE)
				templateHtml.InsertAt(actionSectionPos, resultsHtml);
			else
			{
				const int32 bodyClosePos = templateHtml.Find(TEXT("</body>"), ESearchCase::IgnoreCase);
				if(bodyClosePos != INDEX_NONE)
					templateHtml.InsertAt(bodyClosePos, resultsHtml);
				else
					templateHtml += resultsHtml;
			}
		}

		OutHtml = MoveTemp(templateHtml);
		return true;
	}

	FString BuildFallbackHtml(const TArray<FText>& Errors, const TArray<FText>& Warnings)
	{
		FString body = TEXT("<h1>Graph Validation</h1>");
		if(Errors.Num() == 0 && Warnings.Num() == 0)
			body += TEXT("<p>No validation errors or warnings were reported for this graph.</p>");
		else
		{
			if(Errors.Num() > 0)
			{
				body += TEXT("<h2>Errors</h2><ul>");
				body += BuildValidationIssueItems(Errors, TEXT(""));
				body += TEXT("</ul>");
			}
			if(Warnings.Num() > 0)
			{
				body += TEXT("<h2>Warnings</h2><ul>");
				body += BuildValidationIssueItems(Warnings, TEXT(""));
				body += TEXT("</ul>");
			}
		}

		return FString::Printf(
			TEXT("<!DOCTYPE html><html><head><meta charset=\"utf-8\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><title>Graph Validation</title></head><body>%s</body></html>"),
			*body
		);
	}
}

TSharedPtr<SWindow> MDSPopup_GraphValidation::Open(const TArray<FText>& Errors, const TArray<FText>& Warnings)
{
	if(!FSlateApplication::Get().CanDisplayWindows())
		return nullptr;

	const FVector2f popupClientSize = GetPopupClientSize(TSharedPtr<SWindow>());

	const TSharedRef<SBorder> windowContent = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.Padding(FMargin(0.0f, 0.0f));

	TSharedPtr<SWindow> window = SNew(SWindow)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize)
		.ClientSize(popupClientSize)
		.Title(FText::FromString("Mountea Dialogue System - Graph Validation"))
		.IsTopmostWindow(true)
		[
			windowContent
		];

	TSharedPtr<SMounteaDialogueHtmlView> htmlView;
	windowContent->SetContent(
		SNew(SBorder)
		.Padding(0)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(FLinearColor::Transparent)
		[
			SAssignNew(htmlView, SMounteaDialogueHtmlView)
			.OnConsoleMessage(FOnMounteaDialogueConsoleMessage::CreateStatic(&MDSPopup_GraphValidation::HandleConsoleMessage))
		]
	);

	window = FSlateApplication::Get().AddWindow(window.ToSharedRef());

	const TWeakPtr<SWindow> weakWindow = window;
	const FTSTicker::FDelegateHandle resizeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([weakWindow](float)
		{
			const TSharedPtr<SWindow> pinnedWindow = weakWindow.Pin();
			if(!pinnedWindow.IsValid())
				return false;

			pinnedWindow->Resize(GetPopupClientSize(pinnedWindow));
			return true;
		})
	);

	window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([resizeTickerHandle](const TSharedRef<SWindow>&)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(resizeTickerHandle);
	}));

	if(htmlView.IsValid())
	{
		FString validationHtml;
		FString htmlSourcePath;
		if(!BuildValidationHtmlFromTemplate(Errors, Warnings, validationHtml, htmlSourcePath))
		{
			validationHtml = BuildFallbackHtml(Errors, Warnings);
			htmlSourcePath.Empty();
		}

		htmlView->LoadHtmlString(validationHtml, htmlSourcePath);
	}

	return window;
}

void MDSPopup_GraphValidation::HandleConsoleMessage(const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity)
{
	(void)Source;
	(void)Line;
	(void)Severity;

	if(!Message.StartsWith(TEXT("MDS_LINK:")))
		return;

	const FString data = Message.RightChop(9);
	FString dataType;
	FString dataTarget;
	if(!data.Split(TEXT(":"), &dataType, &dataTarget))
		return;

	if(dataType == TEXT("external"))
		FPlatformProcess::LaunchURL(*dataTarget, nullptr, nullptr);
}
