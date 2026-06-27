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
#include "Setup/MounteaDialogueSetupUtilities.h"

enum class EWebBrowserConsoleLogSeverity;
class SWindow;

class MDSSetupDefaultsPopup
{
public:
	static void Open(const FSetupDefaultsReport& Report);

private:
	static FString BuildResultHtml(const FSetupDefaultsReport& Report);
	static FString BuildResultHtml_WithTemplate(const FSetupDefaultsReport& Report, const FString& TemplateHtml);
	static void HandleConsoleMessage(TWeakPtr<SWindow> WeakWindow, const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity);
	
	static FString GetStatusBadgeClass(ESetupItemStatus Status);
	static FString EscapeHtml(const FString& Text);
	static FString GetStatusLabel(ESetupItemStatus Status);
	static FVector2f GetPopupClientSize(const TSharedPtr<SWindow>& Window);
};
