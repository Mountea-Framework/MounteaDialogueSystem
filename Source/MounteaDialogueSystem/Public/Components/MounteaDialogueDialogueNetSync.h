// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueDialogueNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

/**
 * @deprecated Use server-authoritative RPCs on UMounteaDialogueManager instead.
 * RPC routing through PlayerController is replaced by direct Server Reliable RPCs on the manager component.
 * This component will be removed in a future version.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"),
	meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Net Sync (Deprecated)",
		DeprecatedNode, DeprecationMessage="NetSync is replaced by direct Server RPCs on UMounteaDialogueManager. Remove this component."))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDialogueNetSync : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueDialogueNetSync();

protected:
	
	virtual void BeginPlay() override;

public:

	// --- Manager functions ------------------------------
	
	void ReceiveStartRequest(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	void ReceiveCloseRequest(UObject* CallingManager);
	void ReceiveSetState(UObject* CallingManager, const EDialogueManagerState NewState);
	void ReceiveBroadcastContextRequest(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& Context);
	void ReceiveCloseDialogue(UObject* CallingManager);

protected:

	// --- Manager functions ------------------------------
	
	UFUNCTION(Server, Reliable)
	void ReceiveStartRequest_Server(UObject* CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	UFUNCTION(Server, Reliable)
	void ReceiveCloseRequest_Server(UObject* CallingManager);
	UFUNCTION(Server, Reliable)
	void ReceiveSetState_Server(UObject* CallingManager, const EDialogueManagerState NewState);
	UFUNCTION(Server, Reliable)
	void ReceiveBroadcastContextRequest_Server(UObject* CallingManager, const FMounteaDialogueContextReplicatedStruct& Context);
	UFUNCTION(Server, Reliable)
	void ReceiveCloseDialogue_Server(UObject* CallingManager);
};
