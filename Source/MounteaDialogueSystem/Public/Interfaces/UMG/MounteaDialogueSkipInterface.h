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

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	/**
	 * 
	 * @param FadeProgressDuration 
	 */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void RequestShow(const FVector2D& FadeProgressDuration);
	virtual void RequestShow_Implementation(const FVector2D& FadeProgressDuration) = 0;

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void RequestHide();
	virtual void RequestHide_Implementation() = 0;
};
