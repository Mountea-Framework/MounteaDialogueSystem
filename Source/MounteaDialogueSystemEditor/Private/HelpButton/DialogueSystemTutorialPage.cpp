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

#include "DialogueSystemTutorialPage.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ISettingsModule.h"
#include "Interfaces/IPluginManager.h"
#include "LevelEditor.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "Slate/SMounteaDialogueHtmlView.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString EscapeHtml(const FString& InputValue)
	{
		FString outputValue = InputValue;
		outputValue.ReplaceInline(TEXT("&"), TEXT("&amp;"));
		outputValue.ReplaceInline(TEXT("<"), TEXT("&lt;"));
		outputValue.ReplaceInline(TEXT(">"), TEXT("&gt;"));
		outputValue.ReplaceInline(TEXT("\""), TEXT("&quot;"));
		return outputValue;
	}

	FString BuildMissingPageHtml(const int32 PageId, const FString& PagePath)
	{
		const FString escapedPath = EscapeHtml(PagePath);

		return FString::Printf(
			TEXT("<!DOCTYPE html><html><head><meta charset=\"utf-8\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><title>Mountea Dialogue System - Missing Tutorial Page</title></head><body><section class=\"doc-page\"><header><span class=\"version\">Mountea Dialogue System</span><h1>Tutorial Page Is Missing</h1><p class=\"muted\">The requested chapter could not be loaded from disk.</p></header><div class=\"card\"><h2>Details</h2><p><strong>Page ID:</strong> %d</p><p><strong>Configured Path:</strong> <code>%s</code></p><p>Please verify the file path in Project Settings under Mountea Dialogue System (Editor) - HelpPages.</p></div><div class=\"card\"><h2>Quick Actions</h2><p><a href=\"#\" data-mds-type=\"settings\" data-mds-target=\"Mountea\">Open Mountea Dialogue Settings</a></p><p><a href=\"#\" data-mds-type=\"page\" data-mds-target=\"0\">Go To Welcome Chapter</a></p></div></section></body></html>"),
			PageId,
			*escapedPath
		);
	}

	FString GetFallbackTutorialPagePath()
	{
		const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
		if(!plugin.IsValid())
			return FString();

		const FString fallbackPath = FPaths::Combine(plugin->GetBaseDir(), TEXT("Resources/Help/page_missing.html"));
		return FPaths::ConvertRelativePathToFull(fallbackPath);
	}

	void OpenSettingsPage(const FString& SettingsCategory)
	{
		ISettingsModule& settingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");

		if(SettingsCategory == TEXT("Mountea"))
		{
			settingsModule.ShowViewer("Project", TEXT("Mountea Framework"), TEXT("Mountea Dialogue System"));
			return;
		}

		if(SettingsCategory == TEXT("GameplayTags"))
		{
			settingsModule.ShowViewer("Project", TEXT("Project"), TEXT("GameplayTags"));
		}
	}

	void OpenContentBrowserFolder(const FString& FolderName)
	{
		FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		contentBrowserModule.Get().SyncBrowserToFolders({FolderName});
	}

	void OpenWorldSettings()
	{
		if(!FModuleManager::Get().IsModuleLoaded("LevelEditor"))
			return;

		FLevelEditorModule& levelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<FTabManager> tabManager = levelEditor.GetLevelEditorTabManager();
		if(!tabManager.IsValid())
			return;

		tabManager->TryInvokeTab(FName("WorldSettingsTab"));
	}
}

void SDialogueSystemTutorialPage::Construct(const FArguments& InArgs)
{
	(void)InArgs;

	TSharedRef<SVerticalBox> navigationBox = SNew(SVerticalBox);
	TArray<int32> pageIds;

	const UMounteaDialogueGraphEditorSettings* editorSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if(editorSettings)
	{
		editorSettings->GetEditorTemplatePages().GetKeys(pageIds);
		pageIds.Sort();

		for(const int32 pageId : pageIds)
		{
			const FText pageTitle = editorSettings->GetEditorTemplatePageTitle(pageId);
			navigationBox->AddSlot()
			.AutoHeight()
			[
				CreateNavigationButton(pageTitle, pageId)
			];
		}
	}

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.24f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Brushes.Background"))
			.Padding(20.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					navigationBox
				]
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.76f)
		.Padding(4.f, 0.f)
		[
			SAssignNew(HtmlView, SMounteaDialogueHtmlView)
			.OnConsoleMessage_Raw(this, &SDialogueSystemTutorialPage::HandleConsoleMessage)
		]
	];

	if(pageIds.Num() > 0)
	{
		SwitchToPage(pageIds[0]);
		return;
	}

	SwitchToPage(0);
}

TSharedRef<SWidget> SDialogueSystemTutorialPage::CreateNavigationButton(const FText& Label, int32 PageId)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(10.f, 8.f))
		.ButtonColorAndOpacity_Lambda([this, PageId]() -> FSlateColor
		{
			if(CurrentPageId == PageId)
				return FSlateColor(FLinearColor(0.10f, 0.42f, 0.90f, 0.45f));

			return FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
		})
		.OnClicked_Lambda([this, PageId]()
		{
			SwitchToPage(PageId);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
			.ColorAndOpacity_Lambda([this, PageId]() -> FSlateColor
			{
				if(CurrentPageId == PageId)
					return FSlateColor(FLinearColor(0.98f, 0.99f, 1.f, 1.f));

				return FSlateColor::UseForeground();
			})
		];
}

void SDialogueSystemTutorialPage::SwitchToPage(int32 PageId)
{
	if(!HtmlView.IsValid())
		return;

	const UMounteaDialogueGraphEditorSettings* editorSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if(!editorSettings)
		return;

	const FString pagePath = editorSettings->GetEditorTemplatePagePath(PageId);
	CurrentPageId = PageId;
	Invalidate(EInvalidateWidgetReason::Paint);

	if(pagePath.IsEmpty() || !FPaths::FileExists(pagePath))
	{
		const FString fallbackPath = GetFallbackTutorialPagePath();
		if(!fallbackPath.IsEmpty() && FPaths::FileExists(fallbackPath))
		{
			HtmlView->LoadHtmlFile(fallbackPath);
			return;
		}

		const FString fallbackHtml = BuildMissingPageHtml(PageId, pagePath);
		HtmlView->LoadHtmlString(fallbackHtml, FString());
		return;
	}

	HtmlView->LoadHtmlFile(pagePath);
}

void SDialogueSystemTutorialPage::HandleConsoleMessage(const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity)
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
	{
		FPlatformProcess::LaunchURL(*dataTarget, nullptr, nullptr);
		return;
	}

	if(dataType == TEXT("page"))
	{
		const int32 pageId = FCString::Atoi(*dataTarget);
		SwitchToPage(pageId);
		return;
	}

	if(dataType == TEXT("settings"))
	{
		OpenSettingsPage(dataTarget);
		return;
	}

	if(dataType == TEXT("folder"))
	{
		OpenContentBrowserFolder(dataTarget);
		return;
	}

	if(dataType == TEXT("worldsettings"))
	{
		OpenWorldSettings();
	}
}
