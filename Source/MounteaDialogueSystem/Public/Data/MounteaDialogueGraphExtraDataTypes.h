// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphExtraDataTypes.generated.h"

#if WITH_EDITORONLY_DATA

USTRUCT(BlueprintType)
struct FDialogueImportData
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, meta=(NoResetToDefault), Category="Import")
	FString JsonFile;

	UPROPERTY(VisibleAnywhere, meta=(NoResetToDefault, MultiLine), Category="Import")
	FString JsonData;
};

#endif
