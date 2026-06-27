#include "FMounteaDialogueGraphEditorCommands.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphEditorCommands"

void FMounteaDialogueGraphEditorCommands::RegisterCommands()
{
	UI_COMMAND
	(
		AutoArrange, 
		"Auto Arrange", 
		"📊 Tries its best to arrange Graph Nodes. Don't judge too harshly please.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND
	(
		RecenterGraph,
		"Recenter",
		"Focuses Start node in the viewport.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	UI_COMMAND
	(
		FitGraphToView,
		"Fit to View",
		"Frames the full graph in the viewport.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);

	
	UI_COMMAND
	(
		ValidateGraph, 
		"Validate Graph", 
		"🚨️ Validates Graph if there are any invalid connections or broken data.",
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

	UI_COMMAND
	(
		ExportGraph, 
		"Export Graph", 
		"📤Export the Dialogue Graph as a file containing MNTEADLG data",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE
