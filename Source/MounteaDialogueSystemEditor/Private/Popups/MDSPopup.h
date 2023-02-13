#pragma once
#include "Framework/Text/SlateHyperlinkRun.h"

class MDSPopup
{
public:
	static void Register(const FString& Changelog);
	static void Open(const FString& Changelog);
	static void OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata);

	static void FormatChangelog(FString& InChangelog);
};
