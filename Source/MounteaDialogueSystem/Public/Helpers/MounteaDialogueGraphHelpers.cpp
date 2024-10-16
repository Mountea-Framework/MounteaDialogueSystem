// Copyright Dominik Pavlicek 2022. All Rights Reserved.

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Engine/World.h"

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
#if WITH_EDITOR
		FMsg::Logf(__FILE__, __LINE__, LogMounteaDialogueSystem.GetCategoryName(), Verbosity, TEXT("%s"), *Message);
#endif
		
		UKismetSystemLibrary::PrintString(GWorld, Message, true, true, Color, Duration);
	}
}
