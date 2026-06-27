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
#include "Slate/SMounteaDialogueWebBrowserWrapper.h"
#include "Widgets/SCompoundWidget.h"

class SMounteaDialogueHtmlView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMounteaDialogueHtmlView)
		: _InitialURL(TEXT("about:blank"))
	{
	}
		SLATE_ARGUMENT(FString, InitialURL)
		SLATE_EVENT(FSimpleDelegate, OnLoadStarted)
		SLATE_EVENT(FSimpleDelegate, OnLoadCompleted)
		SLATE_EVENT(FOnMounteaDialogueConsoleMessage, OnConsoleMessage)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void LoadHtmlFile(const FString& FilePath);
	void LoadHtmlString(const FString& HtmlContent, const FString& SourcePath = FString());
	void ExecuteJavascript(const FString& ScriptText) const;

private:
	TSharedPtr<SMounteaDialogueWebBrowserWrapper> WebBrowser;
	bool IsInputBlocked = false;
};
