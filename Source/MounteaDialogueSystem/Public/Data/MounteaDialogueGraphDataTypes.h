// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "DialogueAdditionalData.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"

#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"

#include "MounteaDialogueGraphDataTypes.generated.h"

class UMounteaDialogueContext;
class IMounteaDialogueParticipantInterface;
class UMounteaDialogueGraphNode;
class USoundBase;
class UTexture;
class UDataAsset;

/**
 * Dialogue Manager State
 * 
 * Define state of Dialogue Manager.
 */
UENUM(BlueprintType)
enum class EDialogueManagerState : uint8
{
	EDMS_Disabled			UMETA(DisplayName="Disabled",			Tooltip="Disabled. Dialogue cannot start."),
	EDMS_Enabled			UMETA(DisplayName="Enabled",			Tooltip="Enabled. Dialogue can start."),
	EDMS_Active				UMETA(DisplayName="Active",				Tooltip="Active. Is in Diaologue."),
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
	EDPS_Active			UMETA(DisplayName="Active",				Tooltip="Active. Is in Diaologue."),
};

/**
 * Row Duration Mode
 * 
 * Used for calculation of the Row Duration.
 */
UENUM(BlueprintType)
enum class ERowDurationMode : uint8
{
	ERDM_Manual					UMETA(DisplayName="Manual",				Tooltip="Row won't start automatically and will wait for `NextDialogueRow` request."),
	ERDM_Duration				UMETA(DisplayName="Duration",			Tooltip="Uses either duration of 'Row Sound' or value from 'Duration'."),
	EDRM_Override				UMETA(DisplayName="Override",			Tooltip="Uses 'Duration Override' value."),
	EDRM_Add						UMETA(DisplayName="Add Time",			Tooltip="Adds 'Duration Override' value to 'Duration'."),
	ERDM_AutoCalculate		UMETA(DisplayName="Calculate",			Tooltip="Calculates Duration automatically. Base value is: 100 characters per 8 seconds.")
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
	 * Row Text.
	 * 
	 * ❗ Required value
	 * ❔ Localizable 
	 * Row Text contains data Players will see in the UI.
	 * 
	 * ❗ This Text should not be displayed as option to be selected, for that use 'DialogueRow.RowTitle' value
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true, MultiLine=true))
	FText RowText;
	/**
	 * Row Sound.
	 * 
	 * ❗ Recommended value
	 * ❔ Sound to be triggered once this Row Data has been displayed in UI.
	 * 
	 * ❗ Is not directly used in any C++ code
	 * ❔ Could be used with 'DP_PlayDialogueSound' or as Sound Value for any better way of handling synced animations and sounds (to get more info how to do that, join the Support Discord)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	USoundBase* RowSound = nullptr;
	/**
	 * Row Duration Mode
	 * 
	 * ❗ Recommended value
	 * ❔ Determines how the 'Row Duration' is calculated.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	ERowDurationMode RowDurationMode;
	/**
	 * Row Duration
	 * 
	 * ❗ Recommended value
	 * ❔ Determines for how long the UI will display this Row Data.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(EditCondition="RowSound==nullptr", UIMin=0.f, ClampMin=0.f, ExposeOnSpawn = true))
	float RowDuration;
	/**
	 * Row Duration Override
	 * 
	 * ❗ Optional value
	 * ❔ Determines how much time is added to the Row Duration if any.
	 * 
	 * ❗ No validation applied here, so using value of -4684 will result in weird behaviour.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	float RowDurationOverride;
	/**
	 * Row GUID.
	 * 
	 * Unique Key when searching and binding this Row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay)
	FGuid RowGUID;

public:

	FDialogueRowData()
		: RowText(LOCTEXT("FDialogueRowData_RowText", "Dialogue Example")), RowSound(nullptr), RowDurationMode(ERowDurationMode::ERDM_Duration), RowDuration(0), RowDurationOverride(0), RowGUID(FGuid::NewGuid())
	{};

	FDialogueRowData
	(
		const FText& InText, USoundBase* InSound, const ERowDurationMode InRowDurationMode, const float InDuration, const float InDurationOverride
	)
	: RowText(InText), RowSound(InSound), RowDurationMode(InRowDurationMode), RowDuration(InDuration), RowDurationOverride(InDurationOverride), RowGUID(FGuid::NewGuid())
	{};

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
	
	friend uint32 GetTypeHash(const FDialogueRowData& ActionKeyData)
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
	 * List of GameplayTags which distinguish participants. 
	 * 
	 * ❗ New feature in version 1.0.5.X.
	 * ❔ Each unique dialogue Participant should be using different Tag, if generic, then use something like `Dialogue.NPC`
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", DisplayName="Compatible Participants Tags")
	FGameplayTagContainer CompatibleTags;
	
	/**
	 * Optional Row type ID.
	 * 
	 * ❗ Optional value.
	 * ❔ Could be used to visually differentiate rows which are using same UI Class.
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
	 * ❗ Optional value.
	 * ❔ Could be used to mark special dialogue options, like "Open Store" or "Leave conversation" with special icon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	UTexture* RowOptionalIcon = nullptr;
	/**
	 * Name of the Dialogue Participant.
	 * 
	 * ❗ Optional value.
	 * If left empty, Dialogue will ignore it and use its default Participant name.
	 * ❔ Each row might have different participant tied to it, so there can be three-way dialogues, where one player talks to 2 NPCs
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FText DialogueParticipant;
	/**
	 * Title of the Dialogue Row.
	 * 
	 * ❔ This should summarize what is this row about, let's say "Accept offering" is a title for "Thank you very much, kind sir, it would be pleasure to join you on your adventure!".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FText RowTitle;
	/**
	 * List of Dialogue Row Data. Not replicated, must be found locally for each Client from replicated Active Node!
	 * 
	 * ❔ Each Dialogue Row can contain multiple of those, where each Data Row represents:
	 * * What Sound should be played
	 * * What text should be displayed
	 * 
	 * This provides easy way to have multiple dialogue lines per single Node.
	 * As example, Player asks NPC what happened to its family. And each sentence could be its own Dialogue Row Data input.
	 * This makes UI easier to read and sounds more managable.
	 * 
	 * Each Data Row has its Duration, which could be based on the Sound, directly set, calculated on generic formula or added atop of the sound duration.
	 */
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(TitleProperty="RowText"))
	TSet<FDialogueRowData> DialogueRowData;
	/**
	 * Additional Row Data
	 * 
	 * Generic Data Asset reference which could hold some more data.
	 * This data could be used for Decorators or UI in general.
	 * Any Data Asset can be used here and no logic is tied to this attribute.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(AllowAbstract=false))
	UDialogueAdditionalData* DialogueRowAdditionalData = nullptr;
	/**
	 * Row GUID.
	 * 
	 * Unique Key when searching and binding this Row.
	 */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay, meta=(NoExport, IgnoreForMemberInitializationTest, NoElementDuplicate))
	FGuid RowGUID;
	/**
	 * ❗ WIP
	 * Title Settings Override.
	 * 
	 * ❔ Provides ability to override this Row Title using direct settings rather than 'UIRowID'.
	 * ❗ No logic is implemented yet
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay)
	FSubtitlesSettings TitleSettingsOverride;

public:

	FDialogueRow()
		:  RowOptionalIcon(nullptr), DialogueParticipant(LOCTEXT("FDialogueRow_Participant", "Dialogue Participant")), RowTitle(LOCTEXT("FDialogueRow_Title", "Selectable Option"))
	{
		RowGUID = FGuid::NewGuid();
	};

	FDialogueRow(const int32 NewUIRowID, UTexture* InRowIcon, const FText& InText, const FText& InParticipant, const TSet<FDialogueRowData>& InData, UDialogueAdditionalData* NewData)
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
	
	friend uint32 GetTypeHash(const FDialogueRow& Row)
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

USTRUCT()
struct FMounteaDialogueContextReplicatedStruct
{
	GENERATED_BODY()
	
	UPROPERTY()
	TScriptInterface<IMounteaDialogueParticipantInterface> ActiveDialogueParticipant;
	UPROPERTY()
	TScriptInterface<IMounteaDialogueParticipantInterface> PlayerDialogueParticipant;
	UPROPERTY()
	TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant;
	UPROPERTY()
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> DialogueParticipants;
	UPROPERTY()
	FGuid ActiveNodeGuid;
	UPROPERTY()
	TArray<TObjectPtr<UMounteaDialogueGraphNode>> AllowedChildNodes;
	UPROPERTY()
	int32 ActiveDialogueRowDataIndex = 0;

	FMounteaDialogueContextReplicatedStruct();
	explicit FMounteaDialogueContextReplicatedStruct(UMounteaDialogueContext* Source);

	void SetData(class UMounteaDialogueContext* Source);
	bool IsValid() const;
};