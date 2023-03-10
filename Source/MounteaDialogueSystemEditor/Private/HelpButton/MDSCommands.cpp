// All rights reserved Dominik Pavlicek 2022.


#include "MDSCommands.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemEditorCommands"

void FMDSCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "Support", "Opens Mountea Framework Support channel", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control|EModifierKey::Shift|EModifierKey::Alt, EKeys::X));
	//UI_COMMAND(WikiAction, "Wiki", "Opens Mountea Dialogue System Wiki page", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control|EModifierKey::Shift|EModifierKey::Alt, EKeys::C));
}

#undef LOCTEXT_NAMESPACE