// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "DialogueAdditionalData.generated.h"

/**
 * Predefined list of Additional Data for Mountea Dialogue Rows.
 * Provides easy to use way to enhance Dialogues with extra details, like Animation to play etc.
 *
 * To make variable for this DataAsset, create a new Blueprint Class using `DialogueAdditionalData` as Parent class.
 * Add variables to that new Blueprint Class.
 * Then you can create this Dialogue Additional Data using the newly defined Blueprint class.
 */
UCLASS( Blueprintable, meta=(UsesHierarchy=true), BlueprintType, EditInlineNew, ClassGroup="Mountea|Dialogue", AutoExpandCategories=("Mountea, Dialogue"), DisplayName="Dialogue Additional Data")
class MOUNTEADIALOGUESYSTEM_API UDialogueAdditionalData : public UObject
{
	GENERATED_BODY()
};
