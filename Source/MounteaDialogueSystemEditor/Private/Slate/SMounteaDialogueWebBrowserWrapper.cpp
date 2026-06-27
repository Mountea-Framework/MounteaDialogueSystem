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

#include "Slate/SMounteaDialogueWebBrowserWrapper.h"

#include "InputCoreTypes.h"
#include "SWebBrowser.h"

void SMounteaDialogueWebBrowserWrapper::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SAssignNew(WebBrowser, SWebBrowser)
		.InitialURL(InArgs._InitialURL)
		.ShowControls(false)
		.ShowAddressBar(false)
		.ShowInitialThrobber(true)
		.SupportsTransparency(true)
		.SupportsThumbMouseButtonNavigation(false)
		.OnLoadStarted(InArgs._OnLoadStarted)
		.OnLoadCompleted(InArgs._OnLoadCompleted)
		.OnConsoleMessage(InArgs._OnConsoleMessage)
	];
}

void SMounteaDialogueWebBrowserWrapper::LoadString(const FString& Contents, const FString& DummyURL) const
{
	if(!WebBrowser.IsValid())
		return;

	WebBrowser->LoadString(Contents, DummyURL);
}

void SMounteaDialogueWebBrowserWrapper::ExecuteJavascript(const FString& ScriptText) const
{
	if(!WebBrowser.IsValid())
		return;

	WebBrowser->ExecuteJavascript(ScriptText);
}

FReply SMounteaDialogueWebBrowserWrapper::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if(MouseEvent.GetEffectingButton() == EKeys::ThumbMouseButton || MouseEvent.GetEffectingButton() == EKeys::ThumbMouseButton2)
		return FReply::Handled();

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}
