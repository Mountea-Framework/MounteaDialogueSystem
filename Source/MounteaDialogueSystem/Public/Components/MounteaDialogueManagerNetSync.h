// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManagerNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

/**
 * Component that enables network synchronization for Mountea Dialogue Managers.
 * Handles RPC routing through PlayerController's network connection and manages dialogue manager registration.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"),  meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager Sync"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManagerNetSync : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueManagerNetSync();

protected:
	
	virtual void BeginPlay() override;

public:

	void ReceiveCreateContext(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& NewContext);
	void ReceiveStartRequest(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);

protected:

	UFUNCTION(Server, Reliable)
	void ReceiveCreateContext_Server(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& NewContext);
	UFUNCTION(Server, Reliable)
	void ReceiveStartRequest_Server(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
};
