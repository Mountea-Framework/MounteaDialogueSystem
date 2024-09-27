// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphExtraDataTypes.generated.h"

USTRUCT(BlueprintType)
struct FDialogueImportData
{
	GENERATED_BODY()

	FDialogueImportData()
	{
	};

	FDialogueImportData(const FString& Key, const FString& Value) : JsonFile(Key), JsonData(Value)
	{
	};

public:
	UPROPERTY(VisibleAnywhere, meta=(NoResetToDefault), Category="Import")
	FString JsonFile;

	UPROPERTY(VisibleAnywhere, meta=(NoResetToDefault, MultiLine), Category="Import")
	FString JsonData;

	bool operator ==(const FDialogueImportData& Other) const
	{
		return JsonFile == Other.JsonFile;
	}
};