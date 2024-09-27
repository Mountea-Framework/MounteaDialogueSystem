// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/UMG/MounteaDialogueSkipInterface.h"
#include "MounteaDialogueSkip.generated.h"

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSkip : public UUserWidget, public IMounteaDialogueSkipInterface
{
	GENERATED_BODY()

public:

	virtual void RequestShowWidget_Implementation(const FVector2D& FadeProgressDuration) override;
	virtual void RequestHideWidget_Implementation() override;
};
