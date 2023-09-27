// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "DialogueAdditionalData.generated.h"

/**
 * Predefined list of Additional Data for Mountea Dialogue Rows.
 * Provides easy to use way to enhance Dialogues with extra details, like Animation to play etc.
 */
UCLASS( Blueprintable, meta=(UsesHierarchy=true), BlueprintType, EditInlineNew, ClassGroup="Mountea|Dialogue", AutoExpandCategories=("Mountea, Dialogue"), DisplayName="Dialogue Additional Data")
class MOUNTEADIALOGUESYSTEM_API UDialogueAdditionalData : public UObject
{
	GENERATED_BODY()
};
