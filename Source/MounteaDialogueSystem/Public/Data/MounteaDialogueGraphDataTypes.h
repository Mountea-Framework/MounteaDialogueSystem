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
class UMounteaDialogueGraph;
class USoundBase;
class UTexture;
class UDataAsset;
class AActor;

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
	EDMT_PlayerDialogue 		UMETA(DisplayName="Player (Active) Dialogue"),
	EDMT_EnvironmentDialogue 	UMETA(DisplayName="Environment (Passive) Dialogue"),

	Default 					UMETA(hidden)
};

/**
 * Dialogue Manager State
 * 
 * Define state of Dialogue Manager.
 */
UENUM(BlueprintType)
enum class EDialogueManagerState : uint8
{
	EDMS_Disabled 	UMETA(DisplayName="Disabled", Tooltip="Disabled. Dialogue cannot start."),
	EDMS_Enabled 	UMETA(DisplayName="Enabled", Tooltip="Enabled. Dialogue can start."),
	EDMS_Active 	UMETA(DisplayName="Active", Tooltip="Active. Is in Diaologue."),

	Default 		UMETA(hidden)
};

/**
 * Dialogue Participant State
 * 
 * Define state of Dialogue Participant. 
 */
UENUM(BlueprintType)
enum class EDialogueParticipantState : uint8
{
	EDPS_Disabled 	UMETA(DisplayName="Disabled", Tooltip="Disabled. Dialogue cannot start."),
	EDPS_Enabled 	UMETA(DisplayName="Enabled", Tooltip="Enabled. Dialogue can start."),
	EDPS_Active 	UMETA(DisplayName="Active", Tooltip="Active. Is in Diaologue."),

	Default 		UMETA(hidden)
};

/**
 * Row Duration Mode
 * 
 * Used for calculation of the Row Duration.
 */
UENUM(BlueprintType)
enum class ERowDurationMode : uint8
{
	ERDM_Manual 		UMETA(DisplayName="Manual", Tooltip="Row won't start automatically and will wait for `NextDialogueRow` request.", hidden),
	ERDM_Duration 		UMETA(DisplayName="Duration", Tooltip="Uses either duration of 'Row Sound' or value from 'Duration'."),
	EDRM_Override 		UMETA(DisplayName="Override", Tooltip="Uses 'Duration Override' value."),
	EDRM_Add 			UMETA(DisplayName="Add Time", Tooltip="Adds 'Duration Override' value to 'Duration'."),
	ERDM_AutoCalculate 	UMETA(DisplayName="Calculate", Tooltip="Calculates Duration automatically. Base value is: 100 characters per 8 seconds."),

	Default 			UMETA(hidden)
};

/**
 * Row execution mode. Defines
 */
UENUM(BlueprintType)
enum class ERowExecutionMode : uint8
{
	EREM_Automatic 		UMETA(DisplayName="Automatic", Tooltip="Next row will be executed if any is present."),
	EREM_AwaitInput 	UMETA(DisplayName="Await Input", Tooltip="Next row will be executed once request is triggered."),
	EREM_Stopping 		UMETA(DisplayName="Stopping", Tooltip="Row will stop execution of whole Node and will finish both."),

	Default 			UMETA(hidden)
};

/**
 * Input Mode type.
 * When Dialogue starts, this value defines what effect it has on Player.
 * Player can be stopped and forced to dialogue 'cinematic' or not affected at all.
 */
UENUM(BlueprintType)
enum class EMounteaInputMode : uint8
{
	EIM_UIOnly 			UMETA(DisplayName="UI Only"),
	EIM_UIAndGame 		UMETA(DisplayName="UI & Game")
};

/**
 * Helper structure exposing Widget text settings to set them globally in Dialogue Settings.
 */
USTRUCT(BlueprintType)
struct FMounteaSubtitlesSettings
{
	GENERATED_BODY()

	/**
	 * Slate Color Settings.
	 * Defines color of the Font for UI.
	 * Default:
	 * * Color: White
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles", 
		meta=(DisplayName="Color and Oppacity"),
		meta=(NoResetToDefault))
	FSlateColor FontColor;
	/**
	 * Slate Font Info settings.
	 * Should be filled by default with:
	 * * Font: Roboot
	 * * Size 16
	 * * Typeface: 'Regular'
	 * * Outline: 1
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles", 
		meta=(ForceShowEngineContent),
		meta=(NoResetToDefault))
	FSlateFontInfo SubtitlesFont;
	
	/**
	 * Shadow Offset Settings.
	 * Defines shadow offset on X and Y axis.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles",
		meta=(NoResetToDefault))
	FVector2D ShadowOffset;
	
	/**
	 * Shadow Color Settings.
	 * Default:
	 * * Color: Black
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles",
		meta=(NoResetToDefault))
	FLinearColor ShadowColor;

	/**
	 * Optional icon associated with this subtitle style.
	 * Replaces the per-row RowOptionalIcon that was previously stored on FDialogueRow.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtitles",
		meta=(NoResetToDefault))
	TSoftObjectPtr<UTexture2D> RowOptionalIcon = nullptr;

	/**
	 * Settings GUID.
	 * Defines whether settings are valid or not.
	 * Invalid settings are:
	 * * With no Font
	 * 
	 * Invalid settings are ignored!
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subtitles", 
		meta=(IgnoreForMemberInitializationTest),
		meta=(NoResetToDefault))
	FGuid SettingsGUID;

public:
	FMounteaSubtitlesSettings() : FontColor(FLinearColor::White), ShadowOffset(1.5f, 1.25f), ShadowColor(FLinearColor::Black)
	{
		SubtitlesFont = FCoreStyle::GetDefaultFontStyle("Regular", 16, FFontOutlineSettings(1));
	};
};

/**
 * Defines the role of a participant in a dialogue session.
 * Used as a bitmask — a single participant may hold multiple roles simultaneously.
 *
 * Example: An NPC who is also the session host could be both NPC and Environment.
 */
UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class EDialogueParticipantType : uint8
{
	None		= 0			UMETA(Hidden),
	Player		= 1 << 0	UMETA(DisplayName="Player"),
	NPC			= 1 << 1	UMETA(DisplayName="NPC"),
	Environment	= 1 << 2	UMETA(DisplayName="Environment"),
};
ENUM_CLASS_FLAGS(EDialogueParticipantType)

#define LOCTEXT_NAMESPACE "DialogueParticipant"

USTRUCT(BlueprintType)
struct FDialogueParticipant : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Participant",
		meta=(NoResetToDefault))
	FName ParticipantName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(NoResetToDefault),
		meta=(Categories="Mountea_Dialogue,Dialogue"))
	FGameplayTag ParticipantCategoryTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(NoResetToDefault))
	TSoftObjectPtr<UTexture2D> ParticipantIcon;

	/**
	 * Role bitmask for this participant.
	 * Used at runtime to determine how the participant interacts with dialogue flow
	 * (e.g. whether it drives player input, auto-advances, owns the graph, etc.).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Participant",
		meta=(Bitmask, BitmaskEnum="/Script/MounteaDialogueSystem.EDialogueParticipantType"),
		meta=(NoResetToDefault))
	int32 ParticipantType = 0;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		meta=(ExposeOnSpawn=true, MultiLine=true),
		meta=(NoResetToDefault))
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		meta=(ExposeOnSpawn=true),
		meta=(NoResetToDefault))
	TObjectPtr<USoundBase> RowSound = nullptr;
	
	/**
	 * Row Duration Mode
	 * 
	 * ❗ Recommended value
	 * ❔ Determines how the 'Row Duration' is calculated.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		meta=(ExposeOnSpawn=true),
		meta=(NoResetToDefault))
	ERowDurationMode RowDurationMode;
	
	/**
	 * Row Duration
	 * 
	 * ❗ Recommended value
	 * ❔ Determines for how long the UI will display this Row Data.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(EditCondition="RowSound==nullptr", UIMin=0.f, ClampMin=0.f, ExposeOnSpawn = true),
		meta=(NoResetToDefault))
	float RowDuration;
	
	/**
	 * Row Duration Override
	 * 
	 * ❗ Optional value
	 * ❔ Determines how much time is added to the Row Duration if any.
	 * 
	 * ❗ No validation applied here, so using value of -4684 will result in weird behaviour.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		meta=(ExposeOnSpawn=true),
		meta=(NoResetToDefault))
	float RowDurationOverride;
	
	/**
	 * If set to true this Row will stop the whole Node execution and next row won't start.
	 * Default is false.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		meta=(ExposeOnSpawn=true),
		meta=(NoResetToDefault))
	ERowExecutionMode RowExecutionBehaviour;
	
	/**
	 * Row GUID.
	 * 
	 * Unique Key when searching and binding this Row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay, 
		meta=(IgnoreForMemberInitializationTest),
		meta=(NoResetToDefault))
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
struct MOUNTEADIALOGUESYSTEM_API FDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	
	/**
	 * List of GameplayTags which distinguish participants. 
	 * 
	 * ❗ New feature in version 1.0.5.X.
	 * ❔ Each unique dialogue Participant should be using different Tag, if generic, then use something like `Dialogue.NPC`
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		DisplayName="Compatible Participants Tags",
		meta=(Categories="Mountea_Dialogue,Dialogue"),
		meta=(NoResetToDefault))
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
		meta=(UIMax=255, ClampMax = 255, UIMin = 0, ClampMin=0, NoSpinbox =true),
		meta=(DisplayName="Row Type ID"),
		meta=(NoResetToDefault))
	int32 UIRowID = 0;
	
	UPROPERTY(NotReplicated, 
		meta=(DeprecatedProperty, DeprecationMessage="Use DialogueParticipantName instead"))
	FText DialogueParticipant;
	
	/**
	 * Name of the Dialogue Participant.
	 * 
	 * ❗ Optional value.
	 * If left empty, Dialogue will ignore it and use its default Participant name.
	 * ❔ Each row might have different participant tied to it, so there can be three-way dialogues, where one player talks to 2 NPCs
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(GetOptions="MounteaDialogueSystem.MounteaDialogueParticipantStatics.GetDialogueParticipantRowNames"),
		meta=(NoResetToDefault))
	FName DialogueParticipantName;
	
	/**
	 * Title of the Dialogue Row.
	 * 
	 * ❔ This should summarize what is this row about, let's say "Accept offering" is a title for "Thank you very much, kind sir, it would be pleasure to join you on your adventure!".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(NoResetToDefault))
	FText RowTitle;
	
	/**
	 * List of Dialogue Row Data.
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(TitleProperty="RowText"),
		meta=(ShowOnlyInnerProperties),
		meta=(NoResetToDefault))
	TArray<FDialogueRowData> RowData;

	/**
	 * Legacy serialized field kept for one migration pass.
	 * Older assets stored this as a Set under this exact name.
	 */
	UPROPERTY(NotReplicated, 
		meta=(DeprecatedProperty, DeprecationMessage="Use RowData array instead"))
	TSet<FDialogueRowData> DialogueRowData;
	
	/**
	 * Additional Row Data
	 * 
	 * Generic Data Asset reference which could hold some more data.
	 * This data could be used for Decorators or UI in general.
	 * Any Data Asset can be used here and no logic is tied to this attribute.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		meta=(AllowAbstract=false),
		meta=(NoResetToDefault))
	TObjectPtr<UDialogueAdditionalData> DialogueRowAdditionalData = nullptr;
	
	/**
	 * Row GUID.
	 * 
	 * Unique Key when searching and binding this Row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue", AdvancedDisplay, 
		meta=(NoExport, IgnoreForMemberInitializationTest, NoElementDuplicate),
		meta=(NoResetToDefault))
	FGuid RowGUID;
	
public:
	FDialogueRow()
		: CompatibleTags(),
		DialogueParticipantName(NAME_None), RowTitle(LOCTEXT("FDialogueRow_Title", "Selectable Option"))
	{
		RowGUID = FGuid::NewGuid();
	};

	FDialogueRow(const int32 NewUIRowID, const FText& InText, const FText& InParticipant,
				const TArray<FDialogueRowData>& InData, UDialogueAdditionalData* NewData, const FGameplayTagContainer& InCompatibleTags)
		: CompatibleTags(InCompatibleTags), UIRowID(NewUIRowID), DialogueParticipant(FText::GetEmpty()),
		DialogueParticipantName(InParticipant.IsEmpty() ? NAME_None : FName(*InParticipant.ToString())),
		RowTitle(InText), RowData(InData), DialogueRowAdditionalData(NewData)
	{
		RowGUID = FGuid::NewGuid();
	}

public:
	
	inline FDialogueRow& operator=(const FDialogueRow& Other)
	{
		DialogueParticipant = Other.DialogueParticipant;
		DialogueParticipantName = Other.DialogueParticipantName;
		RowTitle = Other.RowTitle;
		RowData = Other.RowData;
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
		RowData.Num() > 0;
	}

	bool IsNearlyEqual(const FDialogueRow& Other) const
	{
		const FName thisParticipantName = !DialogueParticipantName.IsNone() ? DialogueParticipantName : FName(*DialogueParticipant.ToString());
		const FName otherParticipantName = !Other.DialogueParticipantName.IsNone() ? Other.DialogueParticipantName : FName(*Other.DialogueParticipant.ToString());

		if (RowTitle.EqualTo(Other.RowTitle) &&
		CompatibleTags.HasAllExact(Other.CompatibleTags) &&
		thisParticipantName == otherParticipantName)
			return true;

		if (RowTitle.EqualTo(Other.RowTitle) && RowData.Num() > 0 && Other.RowData.Num() > 0)
		{
			if (RowData[0] == Other.RowData[0])
				return true;
		}

		return false;
	}

	FString ToString() const;

	virtual void OnPostDataImport(const UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems) override;
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override;
	
	static FDialogueRow Invalid()
	{
		FDialogueRow Row;
		Row.RowGUID.Invalidate();
		Row.RowData.Empty();
		Row.DialogueRowData.Empty();
		return Row;
	}

	void PostSerialize(const FArchive& Ar)
	{
		if (Ar.IsLoading() && RowData.Num() == 0 && DialogueRowData.Num() > 0)
		{
			RowData = DialogueRowData.Array();
			DialogueRowData.Empty();
		}
		
		if (Ar.IsLoading() && !DialogueParticipant.IsEmpty() && DialogueParticipantName.IsNone())
		{
			DialogueParticipantName = FName(*DialogueParticipant.ToString());
			DialogueParticipant = FText::GetEmpty();
		}

		if (Ar.IsLoading() && DialogueParticipant.IsEmpty() && !DialogueParticipantName.IsNone())
		{
			DialogueParticipant = FText::FromName(DialogueParticipantName);
		}
	}

	void UpdateFromDialogueParticipantName();
};

template<>
struct TStructOpsTypeTraits<FDialogueRow> : public TStructOpsTypeTraitsBase2<FDialogueRow>
{
	enum
	{
		WithPostSerialize = true
	};
};

#undef LOCTEXT_NAMESPACE

/**
 * UI Row Identifier
 *
 * Represents a unique identifier for a UI row element.
 */
USTRUCT(BlueprintType)
struct FUIRowID
{
	GENERATED_BODY()

	/**
	 * UI Row Identifier
	 *
	 * Represents a unique identifier for a user interface row in the dialogue graph.
	 * This identifier is clamped between 0 and 255..
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", 
		meta=(UIMax=255, ClampMax = 255, UIMin = 0, ClampMin=0, NoSpinbox =true),
		meta=(NoResetToDefault))
	int32 UIRowID = 0;

	/**
	 * Row Widget Class
	 *
	 * Specifies the widget class to be used for representing rows within the dialogue system.
	 * This allows customization of the user interface rows by linking to a specific `UUserWidget` subclass.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue",
		meta=(NoResetToDefault))
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

/**
 * Dialogue Traverse Path
 *
 * Represents a path taken during the traversal of a dialogue tree.
 */
USTRUCT(BlueprintType)
struct FDialogueTraversePath
{
	GENERATED_BODY()

public:
	
	FDialogueTraversePath()
		: NodeGuid(FGuid::NewGuid())
		, GraphGuid(FGuid::NewGuid())
		, TraverseCount(0)
	{}

	FDialogueTraversePath(const FGuid& InNodeGuid, const FGuid& InGraphGuid, int32 InTraverseCount = 1)
		: NodeGuid(InNodeGuid)
		, GraphGuid(InGraphGuid)
		, TraverseCount(FMath::Max(0, InTraverseCount))
	{}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath",
		meta=(NoResetToDefault))
	FGuid NodeGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath",
		meta=(NoResetToDefault))
	FGuid GraphGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath",
		meta=(NoResetToDefault))
	int32 TraverseCount;

	bool operator==(const FDialogueTraversePath& Other) const
	{
		return NodeGuid == Other.NodeGuid && GraphGuid == Other.GraphGuid;
	}

	bool operator!=(const FDialogueTraversePath& Other) const
	{
		return !(*this == Other);
	}

	FDialogueTraversePath& operator+=(const FDialogueTraversePath& Other)
	{
		if (NodeGuid == Other.NodeGuid && GraphGuid == Other.GraphGuid)
			TraverseCount += Other.TraverseCount;
		return *this;
	}

	friend uint32 GetTypeHash(const FDialogueTraversePath& Path)
	{
		return HashCombine(GetTypeHash(Path.NodeGuid), GetTypeHash(Path.GraphGuid));
	}

	void IncrementCount(int32 IncrementBy = 1)
	{
		TraverseCount += FMath::Max(0, IncrementBy);
	}

	TPair<FGuid, FGuid> GetGuidPair() const
	{
		return TPair<FGuid, FGuid>(NodeGuid, GraphGuid);
	}
};

/**
 * A lightweight request struct passed to the server when initiating a dialogue.
 * Uses soft object references to avoid passing raw actor pointers across RPCs.
 */
USTRUCT(BlueprintType)
struct FDialogueStartRequest
{
	GENERATED_BODY()

public:

	/**
	 * Preferred runtime actor reference for the primary participant.
	 * This is network-safe for RPC usage and should be populated by callers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Request")
	TObjectPtr<AActor> MainParticipantActorRef = nullptr;

	/**
	 * The primary participant actor — typically the NPC initiating or owning the dialogue graph.
	 * Legacy soft reference kept for compatibility fallback.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Request")
	TSoftObjectPtr<AActor> MainParticipantActor;

	/**
	 * Preferred runtime actor references for additional participants.
	 * This is network-safe for RPC usage and should be populated by callers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Request")
	TArray<TObjectPtr<AActor>> OtherParticipantActorRefs;

	/**
	 * Additional participants in the conversation.
	 * Legacy soft references kept for compatibility fallback.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Request")
	TArray<TSoftObjectPtr<AActor>> OtherParticipantActors;

	/**
	 * Optional dialogue graph override to run for this session.
	 * If empty, the main participant's graph is used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Request")
	TSoftObjectPtr<UMounteaDialogueGraph> DialogueGraph;

	bool IsValid() const
	{
		return ::IsValid(MainParticipantActorRef) || !MainParticipantActor.IsNull();
	}
};

