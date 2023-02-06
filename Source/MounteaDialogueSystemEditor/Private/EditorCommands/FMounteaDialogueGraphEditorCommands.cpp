#include "FMounteaDialogueGraphEditorCommands.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorCommands"

void FMounteaDialogueGraphEditorCommands::RegisterCommands()
{
	UI_COMMAND(GraphSettings, "Graph Settings", "Graph Settings", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AutoArrange, "Auto Arrange", "Auto Arrange", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE