// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueSkipInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueSkipInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueSkipInterface
{
	GENERATED_BODY()

public:
	
	/**
	 * Requests the widget to show with a fade-in effect.
	 *
	 * @param FadeProgressDuration A vector specifying the fade-in progress duration for the widget.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue|UserInterface|Skip")
	void RequestShowWidget(const FVector2D& FadeProgressDuration);
	virtual void RequestShowWidget_Implementation(const FVector2D& FadeProgressDuration) = 0;

	/**
	 * Requests the widget to hide with a fade-out effect.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue|UserInterface|Skip")
	void RequestHideWidget();
	virtual void RequestHideWidget_Implementation() = 0;

};
