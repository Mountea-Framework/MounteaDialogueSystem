// Copyright Dominik Pavlicek 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Log category declaration
MOUNTEADIALOGUESYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogMounteaDialogueSystem, Log, All);

MOUNTEADIALOGUESYSTEM_API void PrintDialogueLog(const ELogVerbosity::Type Verbosity, const FString& Message, FLinearColor Color, float Duration);

// Logging macro definitions
#define LOG_INFO(Format, ...) \
{ \
FString FormattedMessage = FString::Printf(Format, ##__VA_ARGS__); \
PrintDialogueLog(ELogVerbosity::Log, FormattedMessage, FLinearColor(0.0f, 1.0f, 0.0f), 5.0f); \
}

#define LOG_WARNING(Format, ...) \
{ \
FString FormattedMessage = FString::Printf(Format, ##__VA_ARGS__); \
PrintDialogueLog(ELogVerbosity::Warning, FormattedMessage, FLinearColor(1.0f, 1.0f, 0.0f), 10.0f); \
}

#define LOG_ERROR(Format, ...) \
{ \
FString FormattedMessage = FString::Printf(Format, ##__VA_ARGS__); \
PrintDialogueLog(ELogVerbosity::Error, FormattedMessage, FLinearColor(1.0f, 0.0f, 0.0f), 15.0f); \
}