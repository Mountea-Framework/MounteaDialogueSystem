// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MounteaDialogueDataTable.generated.h"

/**
 * Mountea Dialogue Data Table
 *
 * A specialized subclass of UDataTable used within the Mountea Dialogue System.
 * This data table utilizes the `FDialogueRow` structure to store and manage dialogue entries.
 *
 * Each entry in this table represents a dialogue row containing dialogue text, associated sound, duration, and other metadata 
 * essential for constructing dynamic and interactive dialogue sequences within the game.
 *
 * The dialogue data is highly flexible, allowing for integration with multiple participants, customized execution modes, and 
 * support for various gameplay scenarios, such as quest dialogues, cutscenes, or branching narrative paths.
 *
 * This class provides a clean and structured way to organize dialogue content, making it easy to reference and manage through 
 * gameplay code or Blueprint logic.
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDataTable : public UDataTable
{
	GENERATED_BODY()


};
