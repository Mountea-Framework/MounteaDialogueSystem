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

#include "Slate/SMounteaDialogueHtmlView.h"

#include "Helpers/MounteaDialogueHtmlHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"

void SMounteaDialogueHtmlView::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SAssignNew(WebBrowser, SMounteaDialogueWebBrowserWrapper)
			.InitialURL(InArgs._InitialURL)
			.OnConsoleMessage(InArgs._OnConsoleMessage)
			.OnLoadStarted_Lambda([this, InArgs]()
			{
				IsInputBlocked = true;
				if(InArgs._OnLoadStarted.IsBound())
					InArgs._OnLoadStarted.Execute();
			})
			.OnLoadCompleted_Lambda([this, InArgs]()
			{
				if(InArgs._OnLoadCompleted.IsBound())
					InArgs._OnLoadCompleted.Execute();
				
				WebBrowser->ExecuteJavascript(TEXT(
							"(function() {"
							"  var start = window.pageYOffset || document.documentElement.scrollTop;"
							"  var startTime = null;"
							"  var duration = 100;"
							"  function animation(currentTime) {"
							"    if (startTime === null) startTime = currentTime;"
							"    var timeElapsed = currentTime - startTime;"
							"    var progress = Math.min(timeElapsed / duration, 1);"
							"    var ease = 1 - Math.pow(1 - progress, 3);"
							"    window.scrollTo(0, start * (1 - ease));"
							"    if (timeElapsed < duration) requestAnimationFrame(animation);"
							"  }"
							"  requestAnimationFrame(animation);"
							"})();"						
						));				

				RegisterActiveTimer(0.2f, FWidgetActiveTimerDelegate::CreateLambda(
					[this](double CurrentTime, float DeltaTime) -> EActiveTimerReturnType
					{
						(void)CurrentTime;
						(void)DeltaTime;
						IsInputBlocked = false;
						return EActiveTimerReturnType::Stop;
					}
				));
			})
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				return IsInputBlocked ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("NoBorder"))
				.Padding(0)
			]
		]
	];
}

void SMounteaDialogueHtmlView::LoadHtmlFile(const FString& FilePath)
{
	if(!WebBrowser.IsValid())
		return;

	FString rawHtml;
	if(!FMounteaDialogueHtmlHelpers::LoadHtmlFile(FilePath, rawHtml))
		return;

	const FString htmlWithAssets = FMounteaDialogueHtmlHelpers::InjectSharedAssets(rawHtml);
	const FString baseUrl = FMounteaDialogueHtmlHelpers::BuildBaseUrl(FilePath);
	WebBrowser->LoadString(htmlWithAssets, baseUrl);
}

void SMounteaDialogueHtmlView::LoadHtmlString(const FString& HtmlContent, const FString& SourcePath)
{
	if(!WebBrowser.IsValid())
		return;

	const FString htmlWithAssets = FMounteaDialogueHtmlHelpers::InjectSharedAssets(HtmlContent);
	const FString baseUrl = FMounteaDialogueHtmlHelpers::BuildBaseUrl(SourcePath);
	WebBrowser->LoadString(htmlWithAssets, baseUrl);
}

void SMounteaDialogueHtmlView::ExecuteJavascript(const FString& ScriptText) const
{
	if(!WebBrowser.IsValid())
		return;

	WebBrowser->ExecuteJavascript(ScriptText);
}
