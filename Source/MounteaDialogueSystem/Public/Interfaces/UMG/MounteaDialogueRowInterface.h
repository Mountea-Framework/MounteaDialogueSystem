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
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/MounteaDialogueSystem.MounteaDialogueUIBFL.NewDialogueWidgetRowData"))
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
	UTexture* RowOptionalIcon				= nullptr;

	/**
	 * The unique identifier for the dialogue row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
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
	 * 
	 * @return 
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Getter"))
	FWidgetDialogueRow GetDialogueWidgetRowData() const;
	virtual FWidgetDialogueRow GetDialogueWidgetRowData_Implementation() const = 0;

	/**
	 * 
	 * @param NewData 
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	void SetNewWidgetDialogueRowData(const FWidgetDialogueRow& NewData);
	virtual void SetNewWidgetDialogueRowData_Implementation(const FWidgetDialogueRow& NewData) = 0;

	/**
	 * 
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	void ResetWidgetDialogueRow();
	virtual void ResetWidgetDialogueRow_Implementation() = 0;

	/**
	 * 
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	void InitializeWidgetDialogueRow();
	virtual void InitializeWidgetDialogueRow_Implementation() = 0;

	/**
	 * Stop the effect from play and finishes the text.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	void StopTypeWriterEffect ();
	virtual void StopTypeWriterEffect_Implementation () = 0;

	/**
	 * Starts the typewriter effect on the given text for the specified duration.
	 * @param SourceText		The full text to display.
	 * @param Duration				The total duration for the typewriter effect.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	void StartTypeWriterEffect (const FText& SourceText, float Duration);
	virtual void StartTypeWriterEffect_Implementation (const FText& SourceText, float Duration) = 0;

	/**
	 * Enables Type-Writer effect.
	 * Based on implementation the effect can start or stop.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	void EnableTypeWriterEffect(bool bEnable);
	virtual void EnableTypeWriterEffect_Implementation(bool bEnable) = 0;
};

#undef LOCTEXT_NAMESPACE