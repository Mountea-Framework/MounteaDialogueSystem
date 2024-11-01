// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

class FMounteaDialogueScriptedAction
{
public:
	static void Register();
	static void Unregister();

	static void ExecutePythonFixer();
	static bool CanExecute();
};
