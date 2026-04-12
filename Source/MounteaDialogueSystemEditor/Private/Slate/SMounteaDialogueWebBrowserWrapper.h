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
#include "Widgets/SCompoundWidget.h"

class SWebBrowser;
enum class EWebBrowserConsoleLogSeverity;

DECLARE_DELEGATE_FourParams(FOnMounteaDialogueConsoleMessage, const FString&, const FString&, int32, EWebBrowserConsoleLogSeverity);

class SMounteaDialogueWebBrowserWrapper : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMounteaDialogueWebBrowserWrapper)
		: _InitialURL(TEXT("about:blank"))
	{
	}
		SLATE_ARGUMENT(FString, InitialURL)
		SLATE_EVENT(FSimpleDelegate, OnLoadStarted)
		SLATE_EVENT(FSimpleDelegate, OnLoadCompleted)
		SLATE_EVENT(FOnMounteaDialogueConsoleMessage, OnConsoleMessage)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void LoadString(const FString& Contents, const FString& DummyURL) const;
	void ExecuteJavascript(const FString& ScriptText) const;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TSharedPtr<SWebBrowser> WebBrowser;
};
