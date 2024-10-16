// Copyright Dominik Pavlicek 2022. All Rights Reserved.

#include "Helpers/MounteaDialogueGraphHelpers.h"

#include "MounteaDialogueSystemSettings.h"
#include "Kismet/KismetSystemLibrary.h"

// Log category definition
DEFINE_LOG_CATEGORY(LogMounteaDialogueSystem);

void PrintLog(const ELogVerbosity::Type Verbosity, const FString& Message, FLinearColor Color, float Duration)
{
	if (!GWorld) return;

	bool isVerbosityAllowed = false;

	if (const UMounteaDialogueSystemSettings* DialogueSettings = GetDefault<UMounteaDialogueSystemSettings>())
	{
		const EMounteaDialogueLoggingVerbosity AllowedLogging = DialogueSettings->GetAllowedLoggVerbosity();
		
		switch (Verbosity)
		{
			case ELogVerbosity::Fatal:
			case ELogVerbosity::Error:
				isVerbosityAllowed = EnumHasAnyFlags(AllowedLogging, EMounteaDialogueLoggingVerbosity::Error);
				break;
			case ELogVerbosity::Warning:
			case ELogVerbosity::Verbose:
				isVerbosityAllowed = EnumHasAnyFlags(AllowedLogging, EMounteaDialogueLoggingVerbosity::Warning);
				break;
			case ELogVerbosity::Display:
			case ELogVerbosity::Log:
				isVerbosityAllowed = EnumHasAnyFlags(AllowedLogging, EMounteaDialogueLoggingVerbosity::Info);
				break;
			case ELogVerbosity::VeryVerbose:
				isVerbosityAllowed = false;
				break;
			default:
				isVerbosityAllowed = false;
				break;
		}
	}

	if (isVerbosityAllowed)
	{
		FString PrefixedMessage = Message;
		ENetMode NetMode = GWorld->GetNetMode();
		if (NetMode != NM_Standalone)
		{
			if (NetMode == NM_DedicatedServer || (NetMode == NM_ListenServer && GWorld->GetFirstPlayerController() == nullptr))
			{
				PrefixedMessage = FString::Printf(TEXT("[Server] %s"), *Message);
			}
			else
			{
				int32 ClientIndex = 0;
				if (const ULocalPlayer* LocalPlayer = GWorld->GetFirstLocalPlayerFromController())
				{
					ClientIndex = LocalPlayer->GetControllerId();
				}
				PrefixedMessage = FString::Printf(TEXT("[Client %d] %s"), ClientIndex, *Message);
			}
		}

		FMsg::Logf(__FILE__, __LINE__, LogMounteaDialogueSystem.GetCategoryName(), Verbosity, TEXT("%s"), *PrefixedMessage);
		UKismetSystemLibrary::PrintString(GWorld, PrefixedMessage, true, true, Color, Duration);
	}
}
