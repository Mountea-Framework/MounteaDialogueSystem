// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphDataTypes.generated.h"

USTRUCT(BlueprintType)
struct FDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category="Dialogue")
	FText Participant;
	
};

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphDataTypes : public UObject
{
	GENERATED_BODY()
};
