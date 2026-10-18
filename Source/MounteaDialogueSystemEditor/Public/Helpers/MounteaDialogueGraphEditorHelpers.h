// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not 
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

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