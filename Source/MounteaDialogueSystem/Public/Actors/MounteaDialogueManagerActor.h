// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MounteaDialogueManagerActor.generated.h"

class UMounteaDialogueManager;

UCLASS(DisplayName="Dialogue Manager Actor")
class MOUNTEADIALOGUESYSTEM_API AMounteaDialogueManagerActor : public AActor
{
	GENERATED_BODY()

public:
	
	AMounteaDialogueManagerActor();

	UMounteaDialogueManager* GetDialogueManagerComponent() const;

protected:
	
	virtual void BeginPlay() override;

private:

	UPROPERTY(Category="Dialogue", VisibleAnywhere)
	TObjectPtr<UMounteaDialogueManager> DialogueManager;

};
