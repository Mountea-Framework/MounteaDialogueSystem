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

#include "MDSSetupDefaultsPopup.h"

#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "Slate/SMounteaDialogueHtmlView.h"
#include "Styling/CoreStyle.h"
#include "Containers/Ticker.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SWindow.h"

namespace
{
	constexpr float PopupWidthRatio = 0.4f;
	constexpr float PopupHeightRatio = 0.6f;

	FVector2f GetPopupClientSize(const TSharedPtr<SWindow>& Window)
	{
		FDisplayMetrics displayMetrics;
		FSlateApplication::Get().GetDisplayMetrics(displayMetrics);

		FPlatformRect workArea = displayMetrics.PrimaryDisplayWorkAreaRect;
		if (Window.IsValid())
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

	FString GetStatusBadgeClass(ESetupItemStatus Status)
	{
		switch (Status)
		{
		case ESetupItemStatus::AlreadyPresent: return TEXT("setup-badge setup-badge--present");
		case ESetupItemStatus::Added:          return TEXT("setup-badge setup-badge--added");
		case ESetupItemStatus::CppClass:       return TEXT("setup-badge setup-badge--cpp");
		case ESetupItemStatus::Failed:         return TEXT("setup-badge setup-badge--failed");
		case ESetupItemStatus::Skipped:        return TEXT("setup-badge setup-badge--skipped");
		}
		return TEXT("setup-badge");
	}

	FString GetStatusLabel(ESetupItemStatus Status)
	{
		switch (Status)
		{
		case ESetupItemStatus::AlreadyPresent: return TEXT("&#x2705; Already set");
		case ESetupItemStatus::Added:          return TEXT("&#x2705; Added");
		case ESetupItemStatus::CppClass:       return TEXT("&#x274C; C++ class");
		case ESetupItemStatus::Failed:         return TEXT("&#x274C; Failed");
		case ESetupItemStatus::Skipped:        return TEXT("&mdash; Skipped");
		}
		return TEXT("Unknown");
	}
}

void MDSSetupDefaultsPopup::Open(const FSetupDefaultsReport& Report)
{
	if (!FSlateApplication::Get().CanDisplayWindows())
		return;

	const TSharedRef<SBorder> windowContent = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.Padding(FMargin(0.0f, 0.0f));

	TSharedPtr<SWindow> window = SNew(SWindow)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize)
		.ClientSize(GetPopupClientSize(TSharedPtr<SWindow>()))
		.Title(FText::FromString("Mountea Dialogue System - Setup Defaults"))
		.IsTopmostWindow(true)
		[
			windowContent
		];

	TSharedPtr<SMounteaDialogueHtmlView> htmlView;
	const TWeakPtr<SWindow> weakWindow = window;

	windowContent->SetContent(
		SNew(SBorder)
		.Padding(0)
		.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
		.BorderBackgroundColor(FLinearColor::Transparent)
		[
			SAssignNew(htmlView, SMounteaDialogueHtmlView)
			.OnConsoleMessage(FOnMounteaDialogueConsoleMessage::CreateLambda(
				[weakWindow](const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity)
				{
					MDSSetupDefaultsPopup::HandleConsoleMessage(weakWindow, Message, Source, Line, Severity);
				}
			))
		]
	);

	window = FSlateApplication::Get().AddWindow(window.ToSharedRef());

	const TWeakPtr<SWindow> resizeWeakWindow = window;
	const FTSTicker::FDelegateHandle resizeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([resizeWeakWindow](float)
		{
			const TSharedPtr<SWindow> pinnedWindow = resizeWeakWindow.Pin();
			if (!pinnedWindow.IsValid())
				return false;

			pinnedWindow->Resize(GetPopupClientSize(pinnedWindow));
			return true;
		})
	);

	window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([resizeTickerHandle](const TSharedRef<SWindow>&)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(resizeTickerHandle);
	}));

	if (htmlView.IsValid())
	{
		const UMounteaDialogueGraphEditorSettings* settings = GetDefault<UMounteaDialogueGraphEditorSettings>();
		const FString htmlSourcePath = settings ? settings->GetSetupDefaultsHtmlPath() : FString();

		const FString resultHtml = BuildResultHtml(Report);
		htmlView->LoadHtmlString(resultHtml, htmlSourcePath);
	}
}

FString MDSSetupDefaultsPopup::BuildResultHtml(const FSetupDefaultsReport& Report)
{
	const UMounteaDialogueGraphEditorSettings* settings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	const FString htmlSourcePath = settings ? settings->GetSetupDefaultsHtmlPath() : FString();

	FString templateHtml;
	if (!htmlSourcePath.IsEmpty() && FPaths::FileExists(htmlSourcePath))
		FFileHelper::LoadFileToString(templateHtml, *htmlSourcePath);

	if (templateHtml.IsEmpty())
	{
		return FString::Printf(
			TEXT("<!DOCTYPE html><html><head><meta charset=\"utf-8\"/></head><body><h1>Setup Defaults</h1><p>Template not found at: %s</p></body></html>"),
			*EscapeHtml(htmlSourcePath)
		);
	}

	return BuildResultHtml_WithTemplate(Report, templateHtml);
}

FString MDSSetupDefaultsPopup::BuildResultHtml_WithTemplate(const FSetupDefaultsReport& Report, const FString& TemplateHtml)
{
	FString rowsHtml;
	for (const FSetupItemResult& item : Report.Items)
	{
		const FString roleCell = EscapeHtml(FMounteaDialogueSetupUtilities::GetRoleDisplayName(item.Role));
		const FString classCell = EscapeHtml(item.ClassName);
		const FString badgeClass = GetStatusBadgeClass(item.Status);
		const FString statusLabel = GetStatusLabel(item.Status);
		const FString detailsCell = EscapeHtml(item.Details);

		rowsHtml += FString::Printf(
			TEXT("<tr><td>%s</td><td class=\"cell-class\">%s</td><td><span class=\"%s\">%s</span></td><td>%s</td></tr>"),
			*roleCell, *classCell, *badgeClass, *statusLabel, *detailsCell
		);
	}

	FString summaryMessage;
	FString summaryClass;
	FString exampleLevelButton;

	if (Report.bIsDefaultGameMode)
	{
		summaryMessage = TEXT("&#x274C; Default GameMode detected. Assign a custom GameMode in World Settings first.");
		summaryClass = TEXT("setup-summary--failed");
		exampleLevelButton = TEXT("<a class=\"setup-action\" href=\"#\" data-mds-type=\"level\" data-mds-target=\"/MounteaDialogueSystem/Example/M_DialogueExample\">Open Example Level</a>");
	}
	else if (Report.AllSucceeded())
	{
		summaryMessage = TEXT("&#x1F389; All components configured. Your project is ready for Mountea Dialogue.");
		summaryClass = TEXT("setup-summary--success");
	}
	else if (!Report.AnySucceeded())
	{
		summaryMessage = TEXT("&#x274C; All checks failed. Check the Example Level for a reference setup.");
		summaryClass = TEXT("setup-summary--failed");
		exampleLevelButton = TEXT("<a class=\"setup-action\" href=\"#\" data-mds-type=\"level\" data-mds-target=\"/MounteaDialogueSystem/Example/M_DialogueExample\">Open Example Level</a>");
	}
	else
	{
		summaryMessage = TEXT("&#x26A0; Some items could not be configured automatically. See details above.");
		summaryClass = TEXT("setup-summary--partial");
	}

	const FString statusChipClass = Report.AllSucceeded()
		? TEXT("setup-chip--success")
		: (Report.AnySucceeded() ? TEXT("setup-chip--partial") : TEXT("setup-chip--failed"));
	const FString statusChipText = Report.AllSucceeded()
		? TEXT("All Set")
		: (Report.AnySucceeded() ? TEXT("Partial") : TEXT("Failed"));

	FString resultHtml = TemplateHtml;
	resultHtml.ReplaceInline(TEXT("__MDS_GAMEMODE_CLASS__"), *EscapeHtml(Report.GameModeClassName), ESearchCase::CaseSensitive);
	resultHtml.ReplaceInline(TEXT("__MDS_STATUS_CHIP_CLASS__"), *statusChipClass, ESearchCase::CaseSensitive);
	resultHtml.ReplaceInline(TEXT("__MDS_STATUS_CHIP_TEXT__"), *statusChipText, ESearchCase::CaseSensitive);
	resultHtml.ReplaceInline(TEXT("__MDS_STATUS_ROWS__"), *rowsHtml, ESearchCase::CaseSensitive);
	resultHtml.ReplaceInline(TEXT("__MDS_SUMMARY_MESSAGE__"), *summaryMessage, ESearchCase::CaseSensitive);
	resultHtml.ReplaceInline(TEXT("__MDS_SUMMARY_CLASS__"), *summaryClass, ESearchCase::CaseSensitive);
	resultHtml.ReplaceInline(TEXT("__MDS_EXAMPLE_LEVEL_BUTTON__"), *exampleLevelButton, ESearchCase::CaseSensitive);

	return resultHtml;
}

void MDSSetupDefaultsPopup::HandleConsoleMessage(TWeakPtr<SWindow> WeakWindow, const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity)
{
	(void)Source;
	(void)Line;
	(void)Severity;

	if (!Message.StartsWith(TEXT("MDS_LINK:")))
		return;

	const FString data = Message.RightChop(9);
	FString dataType;
	FString dataTarget;
	if (!data.Split(TEXT(":"), &dataType, &dataTarget))
		return;

	if (dataType == TEXT("external"))
	{
		FPlatformProcess::LaunchURL(*dataTarget, nullptr, nullptr);
	}
	else if (dataType == TEXT("close"))
	{
		const TSharedPtr<SWindow> pinnedWindow = WeakWindow.Pin();
		if (pinnedWindow.IsValid())
			pinnedWindow->RequestDestroyWindow();
	}
	else if (dataType == TEXT("level"))
	{
		const TSharedPtr<SWindow> pinnedWindow = WeakWindow.Pin();
		if (pinnedWindow.IsValid())
			pinnedWindow->RequestDestroyWindow();

		if (FPackageName::DoesPackageExist(dataTarget))
			FEditorFileUtils::LoadMap(dataTarget, false, true);
	}
}
