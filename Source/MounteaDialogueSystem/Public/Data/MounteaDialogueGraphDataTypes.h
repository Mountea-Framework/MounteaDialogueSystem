// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphDataTypes.generated.h"

UENUM(BlueprintType)
enum class ERowDurationMode : uint8
{
	ERDM_Duration				UMETA(DisplayName="Duration",			Tooltip="Uses either duration of 'Row Sound' or value from 'Duration'."),
	EDRM_Override				UMETA(DisplayName="Override",			Tooltip="Uses 'Duration Override' value."),
	EDRM_Add					UMETA(DisplayName="Add Time",			Tooltip="Adds 'Duration Override' value to 'Duration'."),
	ERDM_AutoCalculate	UMETA(DisplayName="Calculate",			Tooltip="Calculates Duration automatically. Base value is: 100 characters per 8 seconds.")
};

#define LOCTEXT_NAMESPACE "FDialogueRow"
USTRUCT(BlueprintType)
struct FDialogueRowData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	FText RowText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	USoundBase* RowSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	ERowDurationMode RowDurationMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(EditCondition="RowSound==nullptr", UIMin=0.f, ClampMin=0.f, ExposeOnSpawn = true))
	float RowDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	float RowDurationOverride;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay)
	FGuid RowGUID;

public:

	FDialogueRowData()
		: RowText(LOCTEXT("FDialogueRowData_RowText", "Dialogue Example")), RowSound(nullptr), RowDurationMode(ERowDurationMode::ERDM_Duration), RowDuration(0), RowDurationOverride(0)
	{
		RowGUID = FGuid::NewGuid();
	}

	FDialogueRowData
	(
		const FText& InText, USoundBase* InSound, const ERowDurationMode InRowDurationMode, const float InDuration, const float InDurationOverride
	)
	: RowText(InText), RowSound(InSound), RowDurationMode(InRowDurationMode), RowDuration(InDuration), RowDurationOverride(InDurationOverride)
	{
		RowGUID = FGuid::NewGuid();
	}

public:

	bool operator==(const FDialogueRowData& Other) const
	{
		return Other.RowGUID == this->RowGUID;
	}
	
	static friend uint32 GetTypeHash(const FDialogueRowData& ActionKeyData)
	{
		return FCrc::MemCrc32(&ActionKeyData.RowGUID, sizeof(FGuid));
	}
};

USTRUCT(BlueprintType)
struct FDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FText DialogueParticipant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(TitleProperty="RowText"))
	TSet<FDialogueRowData> DialogueRows;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay)
	FGuid RowGUID;

public:

	FDialogueRow()
		: DialogueParticipant(LOCTEXT("FDialogueRow_Participant", "Dialogue Participant"))
	{
		RowGUID = FGuid::NewGuid();
	};

	FDialogueRow(const FText& InParticipant, const TSet<FDialogueRowData>& InData)
		: DialogueParticipant(InParticipant), DialogueRows(InData)
	{
		RowGUID = FGuid::NewGuid();
	}
};
#undef LOCTEXT_NAMESPACE

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphDataTypes : public UObject
{
	GENERATED_BODY()
};
