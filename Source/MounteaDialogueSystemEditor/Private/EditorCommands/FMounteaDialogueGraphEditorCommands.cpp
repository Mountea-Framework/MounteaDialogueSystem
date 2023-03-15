#include "FMounteaDialogueGraphEditorCommands.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorCommands"

void FMounteaDialogueGraphEditorCommands::RegisterCommands()
{
	UI_COMMAND
	(
		AutoArrange, 
		"Auto Arrange", 
		"Auto Arrange",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	
	UI_COMMAND
	(
		ValidateGraph, 
		"Validate Graph", 
		"Validate Graph",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND
	(
		FindInDialogue,
		"Find",
		"Find references to Nodes and their Decorators in the current Dialogue.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Control, EKeys::F)
	);
}

#undef LOCTEXT_NAMESPACE