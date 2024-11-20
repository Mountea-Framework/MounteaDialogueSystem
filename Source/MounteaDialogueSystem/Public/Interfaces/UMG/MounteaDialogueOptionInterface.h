// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueOptionInterface.generated.h"

#define LOCTEXT_NAMESPACE "DialogueOption"

struct FDialogueRow;

/**
 * A helper struct to move Dialogue Option data around.
 */
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/MounteaDialogueSystem.UMounteaDialogueHUDStatics.NewDialogueOptionData"))
struct FDialogueOptionData
{
	GENERATED_BODY()

public:
	
	/**
	 * The unique identifier for this dialogue option parent Node.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FGuid OptionGuid;

	/**
	 * The title of the dialogue option, displayed to the user.
	 * Default value: "This is dialogue option title text."
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FText OptionTitle = LOCTEXT("DialogueOption_Title", "This is dialogue option title text.");
	
	/**
	 * The body text of the dialogue option, can be used as tooltip or additional information.
	 * Default value: "This is dialogue option body text.\nCan be used as tooltip text etc."
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FText OptionBody = LOCTEXT("DialogueOption_Body", "This is dialogue option body text.\nCan be used as tooltip text etc.");

	/**
	 * An optional icon for the dialogue option.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TObjectPtr<UTexture> OptionIcon = nullptr;

	/**
	 * The UI row ID for this dialogue option, used to order or identify the option in the UI.
	 * Default value: 0.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	int32 UIRowID = 0;

public:

	FDialogueOptionData();
	explicit FDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow);
	bool operator==(const FDialogueOptionData& Other) const
	{
		return
		Other.OptionGuid == OptionGuid				&&
		Other.OptionTitle.EqualTo(OptionTitle)	&&
		Other.UIRowID == UIRowID						&&
		Other.OptionIcon == OptionIcon;
	}
	
	bool operator!=(const FDialogueOptionData& Other) const
	{
		return !(*this == Other);
	}

	void ResetOption()
	{
		OptionGuid.Invalidate();
		OptionTitle = FText();
		OptionBody = FText();
		OptionIcon = nullptr;
		UIRowID = 0;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueOptionSelected, const FGuid&, SelectedOption, UUserWidget*, CallingWidget);

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueOptionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueOptionInterface
{
	GENERATED_BODY()

public:
	
	/**
	 * Gets the current dialogue option data.
	 * 
	 * @return The current dialogue option data.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Option")
	FDialogueOptionData GetDialogueOptionData() const;
	virtual FDialogueOptionData GetDialogueOptionData_Implementation() const = 0;

	/**
	 * Sets new dialogue option data.
	 * 
	 * @param NewData The new data to set for the dialogue option.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Option")
	void SetNewDialogueOptionData(const FDialogueOptionData& NewData);
	virtual void SetNewDialogueOptionData_Implementation(const FDialogueOptionData& NewData) = 0;

	/**
	 * Resets the dialogue option data to its default state.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Option")
	void ResetDialogueOptionData();
	virtual void ResetDialogueOptionData_Implementation() = 0;

	/**
	 * Responsible for setting visual data from Dialogue Option data.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Option")
	void InitializeDialogueOption();
	virtual void InitializeDialogueOption_Implementation() = 0;

	/**
	 * Processes the event of a dialogue option being selected.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Option")
	void ProcessOptionSelected();
	virtual void ProcessOptionSelected_Implementation() = 0;

	virtual FOnDialogueOptionSelected& GetDialogueOptionSelectedHandle() = 0;
};

#undef LOCTEXT_NAMESPACE