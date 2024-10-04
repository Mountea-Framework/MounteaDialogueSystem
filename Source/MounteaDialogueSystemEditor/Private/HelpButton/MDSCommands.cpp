// All rights reserved Dominik Pavlicek 2022.


#include "MDSCommands.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemEditorCommands"

void FMDSCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Support", "🆘 Open Mountea Framework Support channel", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control|EModifierKey::Shift|EModifierKey::Alt, EKeys::X));
	UI_COMMAND(DialoguerAction, "Dialoguer", "🧭 Open Mountea Dialoguer Standalone Tool", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE