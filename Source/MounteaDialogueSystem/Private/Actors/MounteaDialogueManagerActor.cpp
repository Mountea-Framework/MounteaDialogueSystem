// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Actors/MounteaDialogueManagerActor.h"

#include "Components/MounteaDialogueManager.h"

AMounteaDialogueManagerActor::AMounteaDialogueManagerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	bNetLoadOnClient = true;
	bAlwaysRelevant = true;

	DialogueManager = CreateDefaultSubobject<UMounteaDialogueManager>(TEXT("Dialogue Manager"));
	ensure((DialogueManager != nullptr));
	DialogueManager->SetIsReplicated(true);
}

UMounteaDialogueManager* AMounteaDialogueManagerActor::GetDialogueManagerComponent() const
{
	return DialogueManager;
}

void AMounteaDialogueManagerActor::BeginPlay()
{
	Super::BeginPlay();
}

