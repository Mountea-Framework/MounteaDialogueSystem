// Copyright Dominik Pavlicek 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetSystemLibrary.h"

// Log category declaration
MOUNTEADIALOGUESYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogMounteaDialogueSystem, Log, All);

// Logging macro definitions
#define LOG_INFO(Format, ...) \
{ \
UE_LOG(LogMounteaDialogueSystem, Log, Format, ##__VA_ARGS__); \
UKismetSystemLibrary::PrintString(GWorld, FString::Printf(Format, ##__VA_ARGS__), true, true, FLinearColor(0.0f, 1.0f, 0.0f), 5.0f); \
}
#define LOG_WARNING(Format, ...) \
{ \
UE_LOG(LogMounteaDialogueSystem, Warning, Format, ##__VA_ARGS__); \
UKismetSystemLibrary::PrintString(GWorld, FString::Printf(Format, ##__VA_ARGS__), true, true, FLinearColor(1.0f, 1.0f, 0.0f), 10.0f); \
}
#define LOG_ERROR(Format, ...) \
{ \
UE_LOG(LogMounteaDialogueSystem, Error, Format, ##__VA_ARGS__); \
UKismetSystemLibrary::PrintString(GWorld, FString::Printf(Format, ##__VA_ARGS__), true, true, FLinearColor(1.0f, 0.0f, 0.0f), 15.0f); \
}
