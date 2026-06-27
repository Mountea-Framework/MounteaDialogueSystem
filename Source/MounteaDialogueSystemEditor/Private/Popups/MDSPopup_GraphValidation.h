// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"

enum class EWebBrowserConsoleLogSeverity;
class SWindow;

class MDSPopup_GraphValidation
{
public:
	static TSharedPtr<SWindow> Open(const TArray<FText>& Errors, const TArray<FText>& Warnings);

private:
	static void HandleConsoleMessage(const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity);
};
