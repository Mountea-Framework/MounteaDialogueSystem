// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueAdditionalData.generated.h"

/**
 * Predefined list of Additional Data for Mountea Dialogue Rows.
 * Provides easy to use way to enhance Dialogues with extra details, like Animation to play etc.
 */
UCLASS(BlueprintType, meta=(UsesHierarchy=true))
class MOUNTEADIALOGUESYSTEM_API UDialogueAdditionalData : public UDataAsset
{
	GENERATED_BODY()
};
