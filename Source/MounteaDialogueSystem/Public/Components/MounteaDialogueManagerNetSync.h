// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MounteaDialogueManagerNetSync.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(Mountea), meta=(BlueprintSpawnableComponent))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManagerNetSync : public UActorComponent
{
	GENERATED_BODY()

public:

	UMounteaDialogueManagerNetSync();

protected:

	virtual void BeginPlay() override;

};
