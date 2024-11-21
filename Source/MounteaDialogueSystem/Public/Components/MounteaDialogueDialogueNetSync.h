// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueDialogueNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

/**
 * Component that enables network synchronization for Mountea Dialogue Managers and Participants.
 * Handles RPC routing through PlayerController's network connection and manages dialogue manager registration and updates to participants.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"),  meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Dialogue Sync"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDialogueNetSync : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueDialogueNetSync();

protected:
	
	virtual void BeginPlay() override;

public:

	void ReceiveStartRequest(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	void ReceiveSetState(UObject* CallingManager, const EDialogueManagerState NewState);

protected:
	
	UFUNCTION(Server, Reliable)
	void ReceiveStartRequest_Server(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	UFUNCTION(Server, Reliable)
	void ReceiveSetState_Server(UObject* CallingManager, const EDialogueManagerState NewState);
};
