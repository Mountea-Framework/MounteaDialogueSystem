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
#include "LevelEditor.h"
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

	const UMounteaDialogueGraphEditorSettings* editorSettings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if(editorSettings)
	{
		TArray<int32> pageIds;
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

	SwitchToPage(0);
}

TSharedRef<SWidget> SDialogueSystemTutorialPage::CreateNavigationButton(const FText& Label, int32 PageId)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(10.f, 8.f))
		.OnClicked_Lambda([this, PageId]()
		{
			SwitchToPage(PageId);
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
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
	if(pagePath.IsEmpty())
		return;

	CurrentPageId = PageId;
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
