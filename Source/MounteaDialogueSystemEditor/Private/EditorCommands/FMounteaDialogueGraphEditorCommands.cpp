#include "FMounteaDialogueGraphEditorCommands.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorCommands"

void FMounteaDialogueGraphEditorCommands::RegisterCommands()
{
	UI_COMMAND(AutoArrange, "Auto Arrange", "Auto Arrange", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ValidateGraph, "Validate Graph", "Validate Graph", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE