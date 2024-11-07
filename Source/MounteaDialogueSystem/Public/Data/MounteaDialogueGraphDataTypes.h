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

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EMounteaDialogueLoggingVerbosity : uint8
{
	None		= 0			UMETA(hidden),
	// Toggle Info On/Off. Info level provides most basic information. Color is green.
	Info		= 1 << 0,
	// Toggle Warning On/Off. Warning level provides information about issues that might affect dialogue, but are not blockers.
	Warning		= 1 << 1,
	// Toggle Error On/Off. Error level provides information about issues that will block dialogue.
	Error		= 1 << 2
};
ENUM_CLASS_FLAGS(EMounteaDialogueLoggingVerbosity)


UENUM(BlueprintType)
enum class EDialogueManagerType : uint8
{
	EDMT_PlayerDialogue UMETA(DisplayName="Player (Active) Dialogue"),
	EDMT_EnvironmentDialogue UMETA(DisplayName="Environment (Passive) Dialogue"),

	Default UMETA(hidden)
};

/**
 * Dialogue Manager State
 * 
 * Define state of Dialogue Manager.
 */
UENUM(BlueprintType)
enum class EDialogueManagerState : uint8
{
	EDMS_Disabled UMETA(DisplayName="Disabled", Tooltip="Disabled. Dialogue cannot start."),
	EDMS_Enabled UMETA(DisplayName="Enabled", Tooltip="Enabled. Dialogue can start."),
	EDMS_Active UMETA(DisplayName="Active", Tooltip="Active. Is in Diaologue."),

	Default UMETA(hidden)
};

/**
 * Dialogue Participant State
 * 
 * Define state of Dialogue Participant. 
 */
UENUM(BlueprintType)
enum class EDialogueParticipantState : uint8
{
	EDPS_Disabled UMETA(DisplayName="Disabled", Tooltip="Disabled. Dialogue cannot start."),
	EDPS_Enabled UMETA(DisplayName="Enabled", Tooltip="Enabled. Dialogue can start."),
	EDPS_Active UMETA(DisplayName="Active", Tooltip="Active. Is in Diaologue."),

	Default UMETA(hidden)
};

/**
 * Row Duration Mode
 * 
 * Used for calculation of the Row Duration.
 */
UENUM(BlueprintType)
enum class ERowDurationMode : uint8
{
	ERDM_Manual UMETA(DisplayName="Manual", Tooltip="Row won't start automatically and will wait for `NextDialogueRow` request.", hidden),
	ERDM_Duration UMETA(DisplayName="Duration", Tooltip="Uses either duration of 'Row Sound' or value from 'Duration'."),
	EDRM_Override UMETA(DisplayName="Override", Tooltip="Uses 'Duration Override' value."),
	EDRM_Add UMETA(DisplayName="Add Time", Tooltip="Adds 'Duration Override' value to 'Duration'."),
	ERDM_AutoCalculate UMETA(DisplayName="Calculate", Tooltip="Calculates Duration automatically. Base value is: 100 characters per 8 seconds."),

	Default UMETA(hidden)
};

/**
 * Row execution mode. Defines
 */
UENUM(BlueprintType)
enum class ERowExecutionMode : uint8
{
	EREM_Automatic UMETA(DisplayName="Automatic", Tooltip="Next row will be executed if any is present."),
	EREM_AwaitInput UMETA(DisplayName="Await Input", Tooltip="Next row will be executed once request is triggered."),
	EREM_Stopping UMETA(DisplayName="Stopping", Tooltip="Row will stop execution of whole Node and will finish both."),

	Default UMETA(hidden)
};

/**
 * Input Mode type.
 * When Dialogue starts, this value defines what effect it has on Player.
 * Player can be stopped and forced to dialogue 'cinematic' or not affected at all.
 */
UENUM(BlueprintType)
enum class EInputMode : uint8
{
	EIM_UIOnly UMETA(DisplayName="UI Only"),
	EIM_UIAndGame UMETA(DisplayName="UI & Game")
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

#define LOCTEXT_NAMESPACE "DialogueParticipant"

USTRUCT(BlueprintType)
struct FDialogueParticipant : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Participant")
	FName ParticipantName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	FGameplayTag ParticipantCategoryTag;
};

#undef LOCTEXT_NAMESPACE

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
	TObjectPtr<USoundBase> RowSound = nullptr;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(EditCondition="RowSound==nullptr", UIMin=0.f, ClampMin=0.f, ExposeOnSpawn = true))
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
	 * If set to true this Row will stop the whole Node execution and next row won't start.
	 * Default is false.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ExposeOnSpawn=true))
	ERowExecutionMode RowExecutionBehaviour;
	/**
	 * Row GUID.
	 * 
	 * Unique Key when searching and binding this Row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay)
	FGuid RowGUID = FGuid::NewGuid();

public:
	FDialogueRowData()
		: RowText(LOCTEXT("FDialogueRowData_RowText", "Dialogue Example"))
		, RowSound(nullptr)
		, RowDurationMode(ERowDurationMode::ERDM_Duration)
		, RowDuration(0)
		, RowDurationOverride(0)
		, RowExecutionBehaviour(ERowExecutionMode::EREM_Automatic)
		, RowGUID(FGuid::NewGuid())
	{
	};

	FDialogueRowData
	(
		const FText& InText, USoundBase* InSound, const ERowDurationMode InRowDurationMode, const float InDuration,
		const float InDurationOverride, const ERowExecutionMode InRowBehaviour = ERowExecutionMode::EREM_Automatic
	)
		: RowText(InText)
		, RowSound(InSound)
		, RowDurationMode(InRowDurationMode)
		, RowDuration(InDuration)
		, RowDurationOverride(InDurationOverride)
		, RowExecutionBehaviour(InRowBehaviour)
		, RowGUID(FGuid::NewGuid())
	{
	};

public:
	inline FDialogueRowData& operator =(const FDialogueRowData& Other)
	{
		RowText = Other.RowText;
		RowSound = Other.RowSound;
		RowDurationMode = Other.RowDurationMode;
		RowDuration = Other.RowDuration;
		RowDurationOverride = Other.RowDurationOverride;
		RowExecutionBehaviour = Other.RowExecutionBehaviour;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(UIMax=255, ClampMax = 255, UIMin = 0, ClampMin=0, NoSpinbox =true, DisplayName="Row Type ID"))
	int32 UIRowID = 0;
	
	/**
	 * Optional Row Icon.
	 * 
	 * ❗ Optional value.
	 * ❔ Could be used to mark special dialogue options, like "Open Store" or "Leave conversation" with special icon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
	TObjectPtr<UTexture> RowOptionalIcon = nullptr;
	
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
	TObjectPtr<UDialogueAdditionalData> DialogueRowAdditionalData = nullptr;
	
	/**
	 * Row GUID.
	 * 
	 * Unique Key when searching and binding this Row.
	 */
	UPROPERTY(/*Transient, */VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay, meta=(NoExport, IgnoreForMemberInitializationTest, NoElementDuplicate))
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
		: CompatibleTags(), RowOptionalIcon(nullptr),
		DialogueParticipant(LOCTEXT("FDialogueRow_Participant", "Dialogue Participant")), RowTitle(LOCTEXT("FDialogueRow_Title", "Selectable Option"))
	{
		RowGUID = FGuid::NewGuid();
	};

	FDialogueRow(const int32 NewUIRowID, UTexture* InRowIcon, const FText& InText, const FText& InParticipant,
				const TSet<FDialogueRowData>& InData, UDialogueAdditionalData* NewData, const FGameplayTagContainer& InCompatibleTags)
		: CompatibleTags(InCompatibleTags), UIRowID(NewUIRowID), RowOptionalIcon(InRowIcon), DialogueParticipant(InParticipant),
		RowTitle(InText), DialogueRowData(InData), DialogueRowAdditionalData(NewData)
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
		CompatibleTags = Other.CompatibleTags;

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

	bool IsValid() const
	{
		return
		RowGUID.IsValid() &&
		DialogueRowData.Num() > 0;
	}

	bool IsNearlyEqual(const FDialogueRow& Other) const
	{
		if (RowTitle.EqualTo(Other.RowTitle) &&
		CompatibleTags.HasAllExact(Other.CompatibleTags) &&
		DialogueParticipant.EqualTo(Other.DialogueParticipant))
		{
			return true;
		}

		if (RowTitle.EqualTo(Other.RowTitle) && DialogueRowData.Num() > 0 && Other.DialogueRowData.Num() > 0)
		{
			const FDialogueRowData ThisFirstRowData = DialogueRowData.Array()[0];
			const FDialogueRowData OtherFirstRowData = Other.DialogueRowData.Array()[0];

			if (ThisFirstRowData == OtherFirstRowData)
			{
				return true;
			}
		}

		return false;
	}
	
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override
	{
		FTableRowBase::OnDataTableChanged(InDataTable, InRowName);

		if (!InDataTable)
		{
			return;
		}

		const FName* FoundRowName = nullptr;
	
		for (const auto& Pair : InDataTable->GetRowMap())
		{
			const FDialogueRow* RowPtr = reinterpret_cast<const FDialogueRow*>(Pair.Value);
			if (RowPtr == this)
			{
				FoundRowName = &Pair.Key;
				break;
			}
		}

		if (FoundRowName)
		{
			if (*FoundRowName != InRowName)
			{
				//RowGUID = FGuid::NewGuid();
			}
		}
	}

	FString ToString() const;
	
	static FDialogueRow Invalid()
	{
		FDialogueRow Row;
		Row.RowGUID.Invalidate();
		Row.DialogueRowData.Empty();
		return Row;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(UIMax=255, ClampMax = 255, UIMin = 0, ClampMin=0, NoSpinbox =true))
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
	AActor* ParticipantOwner = nullptr;
	
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
	FGuid PreviousActiveNodeGuid;
	UPROPERTY()
	TArray<FGuid> AllowedChildNodes;
	UPROPERTY()
	FDataTableRowHandle ActiveDialogueTableHandle;
	UPROPERTY()
	int32 ActiveDialogueRowDataIndex = 0;

	FMounteaDialogueContextReplicatedStruct();
	explicit FMounteaDialogueContextReplicatedStruct(UMounteaDialogueContext* Source);

	FMounteaDialogueContextReplicatedStruct operator+=(UMounteaDialogueContext* Source);
	bool operator==(const FMounteaDialogueContextReplicatedStruct& Other) const
	{
		return ActiveDialogueParticipant == Other.ActiveDialogueParticipant
		&& DialogueParticipants == Other.DialogueParticipants
		&& ActiveNodeGuid == Other.ActiveNodeGuid
		&& AllowedChildNodes == Other.AllowedChildNodes
		&& ActiveDialogueTableHandle == Other.ActiveDialogueTableHandle
		&& ActiveDialogueRowDataIndex == Other.ActiveDialogueRowDataIndex;
	}
	bool operator!=(const FMounteaDialogueContextReplicatedStruct& Other) const
	{
		return !(*this == Other);
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	FString ToString() const;
	bool IsValid() const;
	void Reset();
};

template<>
struct TStructOpsTypeTraits<FMounteaDialogueContextReplicatedStruct> : public TStructOpsTypeTraitsBase2<FMounteaDialogueContextReplicatedStruct>
{
	enum 
	{
		WithNetSerializer = true
	};
};