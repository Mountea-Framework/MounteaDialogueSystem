// Copyright Dominik Pavlicek 2022. All Rights Reserved.

#include "Helpers/MounteaDialogueGraphHelpers.h"

// Log category definition
DEFINE_LOG_CATEGORY(LogMounteaDialogueSystem);

void PrintLog(const FString& Message, FLinearColor Color, float Duration)
{
	// Ensure GWorld is available
	if (GWorld)
	{
		UKismetSystemLibrary::PrintString(GWorld, Message, true, true, Color, Duration);
	}
}
