// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"

#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"

#include "MounteaDialogueGraphDataTypes.generated.h"

class USoundBase;
class UTexture;

/**
 * Dialogue Manager State
 * 
 * Define state of Dialogue Manager.
 */
UENUM(BlueprintType)
enum class EDialogueManagerState : uint8
{
	EDMS_Disabled		UMETA(DisplayName="Disabled",			Tooltip="Disabled. Dialogue cannot start."),
	EDMS_Enabled			UMETA(DisplayName="Enabled",			Tooltip="Enabled. Dialogue can start."),
	EDMS_Active			UMETA(DisplayName="Active",				Tooltip="Active. Is in Diaologue."),
};

/**
 * Dialogue Participant State
 * 
 * Define state of Dialogue Participant. 
 */
UENUM(BlueprintType)
enum class EDialogueParticipantState : uint8
{
	EDPS_Disabled		UMETA(DisplayName="Disabled",			Tooltip="Disabled. Dialogue cannot start."),
	EDPS_Enabled		UMETA(DisplayName="Enabled",			Tooltip="Enabled. Dialogue can start."),
	EDPS_Active		UMETA(DisplayName="Active",				Tooltip="Active. Is in Diaologue."),
};

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
	ERDM_AutoCalculate	UMETA(DisplayName="Calculate",			Tooltip="Calculates Duration automatically. Base value is: 100 characters per 8 seconds.")
};

/**
 * Input Mode type.
 * When Dialogue starts, this value defines what effect it has on Player.
 * Player can be stopped and forced to dialogue 'cinematic' or not affected at all.
 */
UENUM(BlueprintType)
enum class EInputMode : uint8
{
	EIM_UIOnly			UMETA(DisplayName="UI Only"),
	EIM_UIAndGame	UMETA(DisplayName="UI & Game")
};

/**
 * Helper structure exposing Widget text settings to set them globally in Dialogue Settings.
 */
USTRUCT(BlueprintType)
struct FSubtitlesSettings
{
	GENERATED_BODY()

	/**
	 * Slate Color Settings.
	 * Defines color of the Font for UI.
	 * Default:
	 * * Color: White
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles", meta=(DisplayName="Color and Oppacity"))
	FSlateColor FontColor;
	/**
	 * Slate Font Info settings.
	 * Should be filled by default with:
	 * * Font: Roboot
	 * * Size 16
	 * * Typeface: 'Regular'
	 * * Outline: 1
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles", meta=(ForceShowEngineContent))
	FSlateFontInfo SubtitlesFont;
	/**
	 * Shadow Offset Settings.
	 * Defines shadow offset on X and Y axis.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles")
	FVector2D ShadowOffset;
	/**
	 * Shadow Color Settings.
	 * Default:
	 * * Color: Black
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles")
	FLinearColor ShadowColor;

	/**
	 * Settings GUID.
	 * Defines whether settings are valid or not.
	 * Invalid settings are:
	 * * With no Font
	 * 
	 * Invalid settings are ignored!
	 */
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
	
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	FText RowText;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	USoundBase* RowSound = nullptr;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	ERowDurationMode RowDurationMode;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(EditCondition="RowSound==nullptr", UIMin=0.f, ClampMin=0.f, ExposeOnSpawn = true))
	float RowDuration;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	float RowDurationOverride;
	/**
	 * 
	 */
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
	/**
	 * Optional Row type ID.
	 * 
	 * ❗Optional value.
	 * ❔Could be used to visually differentiate rows which are using same UI Class.
	 * Example: Quest dialogue options are yellow or bold, while optional dialogue options are white or regular.
	 * 
	 * Range:
	 * * Min: 0
	 * * Max: 255
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(UIMax=255, ClampMax = 255, UIMin = 0, ClampMin=0, NoSpinbox =true, DisplayName="Row Type ID"))
	int32 UIRowID = 0;
	/**
	 * Optional Row Icon.
	 * 
	 * ❗Optional value.
	 * ❔Could be used to mark special dialogue options, like "Open Store" or "Leave conversation" with special icon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	UTexture* RowOptionalIcon = nullptr;
	/**
	 * Name of the Dialogue Participant.
	 * 
	 * ❗Optional value.
	 * If left empty, Dialogue will ignore it and use its default Participant name.
	 * ❔Each row might have different participant tied to it, so there can be three-way dialogues, where one player talks to 2 NPCs
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FText DialogueParticipant;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FText RowTitle;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(TitleProperty="RowText"))
	TSet<FDialogueRowData> DialogueRowData;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FSubtitlesSettings TitleSettingsOverride;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	UDataAsset* DialogueRowAdditionalData = nullptr;
	/**
	 * 
	 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay, meta=(NoExport, IgnoreForMemberInitializationTest, NoElementDuplicate))
	FGuid RowGUID;

public:

	FDialogueRow()
		:  RowOptionalIcon(nullptr), DialogueParticipant(LOCTEXT("FDialogueRow_Participant", "Dialogue Participant")), RowTitle(LOCTEXT("FDialogueRow_Title", "Selectable Option"))
	{
		RowGUID = FGuid::NewGuid();
	};

	FDialogueRow(const int32 NewUIRowID, UTexture* InRowIcon, const FText& InText, const FText& InParticipant, const TSet<FDialogueRowData>& InData, UDataAsset* NewData)
		: UIRowID(NewUIRowID), RowOptionalIcon(InRowIcon), DialogueParticipant(InParticipant), RowTitle(InText), DialogueRowData(InData), DialogueRowAdditionalData(NewData)
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
		UIRowID = Other.UIRowID;
		DialogueRowAdditionalData = Other.DialogueRowAdditionalData;
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
USTRUCT(BlueprintType)
struct FUIRowID
{
	GENERATED_BODY()

	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(UIMax=255, ClampMax = 255, UIMin = 0, ClampMin=0, NoSpinbox =true))
	int32 UIRowID = 0;
	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	TSubclassOf<UUserWidget> RowWidgetClass;

public:

	bool operator==(const FUIRowID& Other) const
	{
		return
		UIRowID == Other.UIRowID &&
		RowWidgetClass == Other.RowWidgetClass;
	}

	bool operator!=(const FUIRowID& Other) const
	{
		return !(*this == Other);
	}
	
	friend uint32 GetTypeHash(const FUIRowID& RowID)
	{
		return FCrc::MemCrc32(&RowID.RowWidgetClass, sizeof(FUIRowID)) + RowID.UIRowID;
	}
};
