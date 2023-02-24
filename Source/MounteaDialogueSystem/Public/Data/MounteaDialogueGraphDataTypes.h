// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"

#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"

#include "MounteaDialogueGraphDataTypes.generated.h"

class USoundBase;

/**
 * Row Duration Mode
 * 
 * Used for calculation of the Row Duration.
 */
UENUM(BlueprintType)
enum class ERowDurationMode : uint8
{
	ERDM_Duration				UMETA(DisplayName="Duration",			Tooltip="Uses either duration of 'Row Sound' or value from 'Duration'."),
	EDRM_Override				UMETA(DisplayName="Override",			Tooltip="Uses 'Duration Override' value."),
	EDRM_Add					UMETA(DisplayName="Add Time",			Tooltip="Adds 'Duration Override' value to 'Duration'."),
	ERDM_AutoCalculate			UMETA(DisplayName="Calculate",			Tooltip="Calculates Duration automatically. Base value is: 100 characters per 8 seconds.")
};

/**
 * Helper structure exposing Widget text settings to set them globally.
 */
USTRUCT(BlueprintType)
struct FSubtitlesSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles", meta=(DisplayName="Color and Oppacity"))
	FLinearColor FontColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles", meta=(ForceShowEngineContent))
	FSlateFontInfo SubtitlesFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles")
	FVector2D ShadowOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles")
	FLinearColor ShadowColor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subtitles")
	FGuid SettingsGUID;

public:

	FSubtitlesSettings() : FontColor(FLinearColor::White), ShadowOffset(1.5f, 1.25f), ShadowColor(FLinearColor::Black)
	{
		SubtitlesFont = FCoreStyle::GetDefaultFontStyle("Regular", 16, FFontOutlineSettings(1));
	};
};

#define LOCTEXT_NAMESPACE "FDialogueRow"

/**
 * Dialogue Row Data
 * 
 * Structure which contains Dialogue Row data, which will be displayed in UI and/or played.
 */
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

	inline FDialogueRowData& operator =(const FDialogueRowData& Other)
	{
		RowText = Other.RowText;
		RowSound = Other.RowSound;
		RowDurationMode = Other.RowDurationMode;
		RowDuration = Other.RowDuration;
		RowDurationOverride = Other.RowDurationOverride;
		RowGUID = FGuid::NewGuid();
		
		return *this;
	}

	bool operator==(const FDialogueRowData& Other) const
	{
		return Other.RowGUID == this->RowGUID;
	}

	bool operator!=(const FDialogueRowData& Other) const
	{
		return !(*this == Other);
	}
	
	static friend uint32 GetTypeHash(const FDialogueRowData& ActionKeyData)
	{
		return FCrc::MemCrc32(&ActionKeyData.RowGUID, sizeof(FGuid));
	}
};

/**
 * Dialogue Row
 * 
 * Data Table compatible Dialogue Row.
 * Contains name of Participant and Row Data.
 */
USTRUCT(BlueprintType)
struct FDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	UTexture* RowOptionalIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FText DialogueParticipant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FText RowTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(TitleProperty="RowText"))
	TSet<FDialogueRowData> DialogueRowData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FSubtitlesSettings TitleSettingsOverride;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay, meta=(NoExport, IgnoreForMemberInitializationTest, NoElementDuplicate))
	FGuid RowGUID;

public:

	FDialogueRow()
		:  RowOptionalIcon(nullptr), DialogueParticipant(LOCTEXT("FDialogueRow_Participant", "Dialogue Participant")), RowTitle(LOCTEXT("FDialogueRow_Title", "Selectable Option"))
	{
		RowGUID = FGuid::NewGuid();
	};

	FDialogueRow(UTexture* InRowIcon, const FText& InText, const FText& InParticipant, const TSet<FDialogueRowData>& InData)
		: RowOptionalIcon(InRowIcon), DialogueParticipant(InParticipant), RowTitle(InText), DialogueRowData(InData)
	{
		RowGUID = FGuid::NewGuid();
	}
	
public:

	inline FDialogueRow& operator=(const FDialogueRow& Other)
	{
		RowOptionalIcon = Other.RowOptionalIcon;
		DialogueParticipant = Other.DialogueParticipant;
		RowTitle = Other.RowTitle;
		DialogueRowData = Other.DialogueRowData;
		TitleSettingsOverride = Other.TitleSettingsOverride;
		RowGUID = FGuid::NewGuid();
		
		return *this;
	}

	bool operator==(const FDialogueRow& Other) const
	{
		return Other.RowGUID == this->RowGUID;
	}

	bool operator!=(const FDialogueRow& Other) const
	{
		return !(*this == Other);
	}
	
	static friend uint32 GetTypeHash(const FDialogueRow& Row)
	{
		return FCrc::MemCrc32(&Row.RowGUID, sizeof(FGuid));
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
