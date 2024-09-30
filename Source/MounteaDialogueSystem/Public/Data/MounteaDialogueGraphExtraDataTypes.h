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

USTRUCT()
struct FDialogueImportSourceData
{
	GENERATED_BODY()

	// Path to saved Graph asset - reimport might try to import to different folder and we don't want to allow that!
	UPROPERTY(VisibleAnywhere, meta=(NoResetToDefault), Category="Import")
	FString DialogueAssetPath;

	// Path to Graph Source .mntedlg file
	UPROPERTY(VisibleAnywhere, meta=(NoResetToDefault), Category="Import")
	FString DialogueSourcePath;

	// TODO: Maybe add FDate as well?
	// Asset Path x Source Data
	UPROPERTY(VisibleAnywhere, meta=(NoResetToDefault), Category="Import")
	TMap<FString,FDialogueImportData> ImportData;
};