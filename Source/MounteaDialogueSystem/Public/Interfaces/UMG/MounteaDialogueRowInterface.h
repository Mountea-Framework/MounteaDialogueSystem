// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueRowInterface.generated.h"

struct FDialogueRowData;
struct FDialogueRow;

class UTexture;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTypeWriterEffectChanged, bool, bEnabled);

#define LOCTEXT_NAMESPACE "DialogueRow"

/**
 * A helper struct to move Dialogue Row data around.
 */
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/MounteaDialogueSystem.MounteaDialogueHUDStatics.NewDialogueWidgetRowData"))
struct FWidgetDialogueRow
{
	GENERATED_BODY()

public:

	/**
	 * The name of the dialogue participant.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FText DialogueRowParticipantName		= LOCTEXT("DialogueRow_Particpant", "Participant Name");

	/**
	 * The title of the dialogue row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FText DialogueRowTitle					= LOCTEXT("DialogueRow_Title", "This is dialogue row title text.");

	/**
	 * The text of the dialogue row body.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FText DialogueRowBody					= LOCTEXT("DialogueRow_Body", "This is dialogue option title text.");

	/**
	 * The duration of the dialogue row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	float RowDuration						= 0.f;

	/**
	 * The UI row ID.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	int32 UIRowID							= 0;

	/**
	 * An optional icon for the dialogue row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TObjectPtr<UTexture> RowOptionalIcon				= nullptr;

	/**
	 * The unique identifier for the dialogue row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue", meta=(IgnoreForMemberInitializationTest))
	FGuid RowGuid							= FGuid::NewGuid();

public:

	FWidgetDialogueRow();
	explicit FWidgetDialogueRow(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData);
	bool operator==(const FWidgetDialogueRow& Other) const
	{
		return
		Other.RowGuid == RowGuid												&&
		Other.DialogueRowParticipantName.EqualTo(DialogueRowParticipantName)	&&
		Other.DialogueRowTitle.EqualTo(DialogueRowTitle)						&&
		Other.DialogueRowBody.EqualTo(DialogueRowBody)							&&
		Other.UIRowID == UIRowID												&&
		FMath::IsNearlyEqual(Other.RowDuration, RowDuration)				&&
		Other.RowOptionalIcon == RowOptionalIcon;
	}
	
	bool operator!=(const FWidgetDialogueRow& Other) const
	{
		return !(*this == Other);
	}

	void ResetRow()
	{
		RowGuid.Invalidate();
		DialogueRowParticipantName = FText();
		DialogueRowTitle = FText();
		DialogueRowBody = FText();
		RowDuration = 0.f;
		RowOptionalIcon = nullptr;
		UIRowID = 0;
	}
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueRowInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueRowInterface
{
	GENERATED_BODY()

public:

	
	/**
	 * Retrieves the dialogue row data associated with the widget.
	 *
	 * @return FWidgetDialogueRow containing the dialogue row data.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|DialogueRow")
	FWidgetDialogueRow GetDialogueWidgetRowData() const;
	virtual FWidgetDialogueRow GetDialogueWidgetRowData_Implementation() const = 0;

	/**
	 * Sets new dialogue row data for the widget.
	 *
	 * @param NewData The new dialogue row data to set.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|DialogueRow")
	void SetNewWidgetDialogueRowData(const FWidgetDialogueRow& NewData);
	virtual void SetNewWidgetDialogueRowData_Implementation(const FWidgetDialogueRow& NewData) = 0;

	/**
	 * Resets the widget's dialogue row data to the default state.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|DialogueRow")
	void ResetWidgetDialogueRow();
	virtual void ResetWidgetDialogueRow_Implementation() = 0;

	/**
	 * Initializes the widget's dialogue row data.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|DialogueRow")
	void InitializeWidgetDialogueRow();
	virtual void InitializeWidgetDialogueRow_Implementation() = 0;

	/**
	 * Stops the typewriter effect and finishes displaying the text.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|DialogueRow")
	void StopTypeWriterEffect();
	virtual void StopTypeWriterEffect_Implementation() = 0;

	/**
	 * Starts the typewriter effect on the specified text for a given duration.
	 *
	 * @param SourceText The full text to display.
	 * @param Duration The duration over which the typewriter effect should play.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|DialogueRow")
	void StartTypeWriterEffect(const FText& SourceText, float Duration);
	virtual void StartTypeWriterEffect_Implementation(const FText& SourceText, float Duration) = 0;

	/**
	 * Enables or disables the typewriter effect based on the specified parameter.
	 *
	 * @param bEnable Whether to enable or disable the effect.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|DialogueRow")
	void EnableTypeWriterEffect(bool bEnable);
	virtual void EnableTypeWriterEffect_Implementation(bool bEnable) = 0;

};

#undef LOCTEXT_NAMESPACE