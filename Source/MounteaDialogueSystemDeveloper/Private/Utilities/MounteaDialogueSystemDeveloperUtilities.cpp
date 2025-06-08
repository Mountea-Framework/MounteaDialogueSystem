// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueSystemDeveloperUtilities.h"
#include "HAL/PlatformApplicationMisc.h"

void UMounteaDialogueSystemDeveloperUtilities::CopyToClipboard(const FString& Input)
{
	if (Input.IsEmpty())
	{
		return;
	}
#if PLATFORM_WINDOWS
	FWindowsPlatformApplicationMisc::ClipboardCopy(*Input);
#elif PLATFORM_LINUX
	FLinuxPlatformApplicationMisc::ClipboardCopy(*Input);
#elif PLATFORM_MAC
	FMacPlatformApplicationMisc::ClipboardCopy(*Input);
#endif
}