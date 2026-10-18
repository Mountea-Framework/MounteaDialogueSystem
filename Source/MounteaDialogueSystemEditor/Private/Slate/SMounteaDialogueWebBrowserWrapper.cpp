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

#include "Async/Async.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "IWebBrowserWindow.h"
#include "InputCoreTypes.h"
#include "SWebBrowser.h"

namespace MounteaDialogueWebBrowserWrapper
{
	// Pages request editor actions by navigating a hidden iframe to this URL (see DialogueEditorHelp.js).
	// The navigation is always cancelled, so nothing is ever fetched from the reserved .invalid domain.
	// Console messages are not used for this, because link messages sent that way are not acted on on Mac.
	static const FString LinkBridgePrefix = TEXT("https://mds-link.invalid/");
}

void SMounteaDialogueWebBrowserWrapper::Construct(const FArguments& InArgs)
{
	OnConsoleMessage = InArgs._OnConsoleMessage;

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
		.OnBeforeNavigation(this, &SMounteaDialogueWebBrowserWrapper::HandleBeforeNavigation)
	];
}

bool SMounteaDialogueWebBrowserWrapper::HandleBeforeNavigation(const FString& Url, const FWebNavigationRequest& Request)
{
	(void)Request;

	if(!Url.StartsWith(MounteaDialogueWebBrowserWrapper::LinkBridgePrefix))
		return false;

	FString payload = Url.RightChop(MounteaDialogueWebBrowserWrapper::LinkBridgePrefix.Len());
	// Drop the "?t=" query the page appends so that repeated clicks on the same link still navigate.
	int32 queryIndex = INDEX_NONE;
	if(payload.FindChar(TEXT('?'), queryIndex))
		payload.LeftInline(queryIndex);

	FString encodedType;
	FString encodedTarget;
	if(!payload.Split(TEXT("/"), &encodedType, &encodedTarget))
		encodedType = payload;

	// Re-emit as the same "MDS_LINK:<type>:<target>" message the console handlers already parse.
	const FString message = FString::Printf(TEXT("MDS_LINK:%s:%s"),
		*FGenericPlatformHttp::UrlDecode(encodedType),
		*FGenericPlatformHttp::UrlDecode(encodedTarget));

	UE_LOG(LogTemp, Warning, TEXT("[MDS DEBUG] Link bridge: %s"), *message); // TEMPORARY

	// Run outside the browser callback, since handlers may load a new page into this browser.
	const TWeakPtr<SMounteaDialogueWebBrowserWrapper> weakThis = StaticCastSharedRef<SMounteaDialogueWebBrowserWrapper>(AsShared());
	AsyncTask(ENamedThreads::GameThread, [weakThis, message]()
	{
		if(const TSharedPtr<SMounteaDialogueWebBrowserWrapper> pinnedThis = weakThis.Pin())
			pinnedThis->OnConsoleMessage.ExecuteIfBound(message, MounteaDialogueWebBrowserWrapper::LinkBridgePrefix, 0, EWebBrowserConsoleLogSeverity::Info);
	});

	return true;
}

void SMounteaDialogueWebBrowserWrapper::SetParentWindow(TSharedPtr<SWindow> Window) const
{
  if(!WebBrowser.IsValid())
    return;

  WebBrowser->SetParentWindow(Window);
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
