// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Log category definition
MOUNTEADIALOGUESYSTEMEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogMounteaDialogueSystemEditor, Display, All);

#define EditorLOG_INFO(Format, ...) \
{ \
	UE_LOG(LogMounteaDialogueSystemEditor, Log, Format, ##__VA_ARGS__); \
}
#define EditorLOG_WARNING(Format, ...) \
{ \
	UE_LOG(LogMounteaDialogueSystemEditor, Warning, Format, ##__VA_ARGS__); \
}
#define EditorLOG_ERROR(Format, ...) \
{ \
	UE_LOG(LogMounteaDialogueSystemEditor, Error, Format, ##__VA_ARGS__); \
}