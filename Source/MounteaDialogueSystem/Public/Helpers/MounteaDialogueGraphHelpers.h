// Copyright Dominik Pavlicek 2022. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Log category definition
MOUNTEADIALOGUESYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogMounteaDialogueSystem, Display, All);

#define LOG_INFO(Format, ...) \
{ \
UE_LOG(LogMounteaDialogueSystem, Log, Format, ##__VA_ARGS__); \
}
#define LOG_WARNING(Format, ...) \
{ \
UE_LOG(LogMounteaDialogueSystem, Warning, Format, ##__VA_ARGS__); \
}
#define LOG_ERROR(Format, ...) \
{ \
UE_LOG(LogMounteaDialogueSystem, Error, Format, ##__VA_ARGS__); \
}
