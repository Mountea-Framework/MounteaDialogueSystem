// All rights reserved Dominik Morse (Pavlicek) 2026

#include "MDSPopup.h"

#include "Interfaces/IPluginManager.h"
#include "MDSPopupConfig.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Slate/SMounteaDialogueHtmlView.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

FPluginVersion MDSPopup::GetPluginVersion()
{
	const FName pluginName = TEXT("MounteaDialogueSystem");
	const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(pluginName.ToString());
	if(!plugin.IsValid())
		return FPluginVersion("1", "0.0.0.1");

	const FString pluginFilePath = plugin->GetBaseDir() / FString::Printf(TEXT("%s.uplugin"), *pluginName.ToString());

	FString fileContents;
	if(!FFileHelper::LoadFileToString(fileContents, *pluginFilePath))
		return FPluginVersion("1", "0.0.0.1");

	TSharedPtr<FJsonObject> jsonObject;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(fileContents);
	if(!FJsonSerializer::Deserialize(reader, jsonObject) || !jsonObject.IsValid())
		return FPluginVersion("1", "0.0.0.1");

	const int32 versionNumber = jsonObject->GetIntegerField(TEXT("Version"));
	const FString versionName = jsonObject->GetStringField(TEXT("VersionName"));
	const FString version = FString::Printf(TEXT("%d"), versionNumber);

	return FPluginVersion(version, versionName);
}

bool MDSPopup::IsVersionGreater(const FString& NewVersion, const FString& OldVersion)
{
	TArray<FString> newParts;
	TArray<FString> oldParts;
	NewVersion.ParseIntoArray(newParts, TEXT("."), true);
	OldVersion.ParseIntoArray(oldParts, TEXT("."), true);

	while(newParts.Num() < 4)
		newParts.Add(TEXT("0"));

	while(oldParts.Num() < 4)
		oldParts.Add(TEXT("0"));

	for(int32 index = 0; index < 4; index++)
	{
		const int32 newValue = FCString::Atoi(*newParts[index]);
		const int32 oldValue = FCString::Atoi(*oldParts[index]);
		if(newValue > oldValue)
			return true;

		if(newValue < oldValue)
			return false;
	}

	return false;
}

bool MDSPopup::ExtractVersionFromMarkdown(const FString& ChangelogMarkdown, FString& OutVersion)
{
	OutVersion.Empty();
	if(ChangelogMarkdown.IsEmpty())
		return false;

	const FRegexPattern versionPattern(TEXT("([0-9]+\\.[0-9]+\\.[0-9]+(?:\\.[0-9]+)?)"));
	TArray<FString> lines;
	ChangelogMarkdown.ParseIntoArrayLines(lines, true);

	for(const FString& line : lines)
	{
		const FString trimmedLine = line.TrimStartAndEnd();
		if(!trimmedLine.StartsWith(TEXT("#")))
			continue;

		FRegexMatcher matcher(versionPattern, trimmedLine);
		if(!matcher.FindNext())
			continue;

		OutVersion = matcher.GetCaptureGroup(1);
		return !OutVersion.IsEmpty();
	}

	FRegexMatcher fallbackMatcher(versionPattern, ChangelogMarkdown);
	if(!fallbackMatcher.FindNext())
		return false;

	OutVersion = fallbackMatcher.GetCaptureGroup(1);
	return !OutVersion.IsEmpty();
}

void MDSPopup::Register(const FString& ChangelogMarkdown, const FString& ChangelogHtml, const FString& ChangelogHtmlPath)
{
	const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	if(!plugin.IsValid())
		return;

	const FString pluginDirectory = plugin->GetBaseDir();
	const FString configFilePath = pluginDirectory + "/Config/UpdateConfig.ini";
	const FString normalizedConfigFilePath = FConfigCacheIni::NormalizeConfigIniPath(configFilePath);

	FString currentPluginVersion = GetPluginVersion().PluginVersionName;
	UMDSPopupConfig* mdsPopupConfig = GetMutableDefault<UMDSPopupConfig>();

	if(FPaths::FileExists(normalizedConfigFilePath))
		mdsPopupConfig->LoadConfig(nullptr, *normalizedConfigFilePath);
	else
		mdsPopupConfig->SaveConfig(CPF_Config, *normalizedConfigFilePath);

	FString changelogVersion;
	if(ExtractVersionFromMarkdown(ChangelogMarkdown, changelogVersion))
		currentPluginVersion = changelogVersion;

	if(!IsVersionGreater(currentPluginVersion, mdsPopupConfig->PluginVersionUpdate))
		return;

	mdsPopupConfig->PluginVersionUpdate = currentPluginVersion;
	mdsPopupConfig->SaveConfig(CPF_Config, *normalizedConfigFilePath);

	Open(ChangelogHtml, ChangelogHtmlPath);
}

void MDSPopup::Open(const FString& ChangelogHtml, const FString& ChangelogHtmlPath)
{
	if(!FSlateApplication::Get().CanDisplayWindows())
		return;

	const TSharedRef<SBorder> windowContent = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(8.0f, 8.0f));

	TSharedPtr<SWindow> window = SNew(SWindow)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize)
		.ClientSize(FVector2D(920, 660))
		.Title(FText::FromString("Mountea Dialogue System"))
		.IsTopmostWindow(true)
		[
			windowContent
		];

	TSharedPtr<SMounteaDialogueHtmlView> htmlView;

	const TSharedRef<SVerticalBox> innerContent = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 24))
			.Text(FText::FromString("Mountea Dialogue System"))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(10)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			[
				SAssignNew(htmlView, SMounteaDialogueHtmlView)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([]()
				{
					const FString url = TEXT("https://www.unrealengine.com/marketplace/en-US/product/ea38ae1f87b24807a66fdf4fa65ef521");
					FPlatformProcess::LaunchURL(*url, nullptr, nullptr);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString("Leave a review"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8, 0)
			[
				SNew(SButton)
				.OnClicked_Lambda([]()
				{
					const FString url = TEXT("https://discord.gg/c2WQ658V44");
					FPlatformProcess::LaunchURL(*url, nullptr, nullptr);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString("Support Discord"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.OnClicked_Lambda([window]()
				{
					window->RequestDestroyWindow();
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString("Close"))
				]
			]
		];

	windowContent->SetContent(innerContent);
	window = FSlateApplication::Get().AddWindow(window.ToSharedRef());

	if(!htmlView.IsValid())
		return;

	if(!ChangelogHtmlPath.IsEmpty() && FPaths::FileExists(ChangelogHtmlPath))
	{
		htmlView->LoadHtmlFile(ChangelogHtmlPath);
		return;
	}

	htmlView->LoadHtmlString(ChangelogHtml, ChangelogHtmlPath);
}
