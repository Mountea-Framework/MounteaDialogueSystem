// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaFocusableWidgetInterface.generated.h"

UENUM(BlueprintType)
enum class EDialogueOptionState : uint8
{
	EDOS_Focused UMETA(DisplayName="Focused"),
	EDOS_Unfocused UMETA(DisplayName="Unfocused"),

	Default UMETA(hidden)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMounteaFocusChanged, UUserWidget*, Widget, const bool, IsFocused);

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaFocusableWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaFocusableWidgetInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Focus")
	EDialogueOptionState GetFocusState() const;
	virtual EDialogueOptionState GetFocusState_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Focus")
	void SetFocusState(const bool IsSelected);
	virtual void SetFocusState_Implementation(const bool IsSelected) = 0;

	virtual FOnMounteaFocusChanged& GetOnMounteaFocusChangedEventHandle() = 0;
};
