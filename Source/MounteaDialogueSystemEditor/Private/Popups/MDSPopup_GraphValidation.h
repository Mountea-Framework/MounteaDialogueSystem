#pragma once
#include "Framework/Text/SlateHyperlinkRun.h"

class SScrollBox;

class MDSPopup_GraphValidation
{
public:
	static TSharedPtr<SWindow> Open(const TArray<FText> ValidationMessages);
	static void OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata);
};
