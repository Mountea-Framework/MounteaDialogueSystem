// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManagerNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

/**
 * 
 */
UCLASS(ClassGroup=(Mountea), Blueprintable,  AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"), meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager Sync"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManagerNetSync : public UActorComponent
{
	GENERATED_BODY()

public:

	UMounteaDialogueManagerNetSync();

protected:
	UFUNCTION()
	void OnManagerSyncActivated(UActorComponent* Component, bool bReset);
	UFUNCTION()
	void OnManagerSyncDeactivated(UActorComponent* Component);
	
	virtual void BeginPlay() override;

public:
	void AddManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager);
	void RemoveManager(const TScriptInterface<IMounteaDialogueManagerInterface>& OldManager);
	

protected:

	UFUNCTION()
	void SyncStartRequested(const TScriptInterface<IMounteaDialogueManagerInterface>& CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	UFUNCTION(Server, Reliable)
	void SyncStartRequested_Server(const TScriptInterface<IMounteaDialogueManagerInterface>& CallingManager, AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);

	UFUNCTION()
	void SyncBroadcastContext(const FMounteaDialogueContextReplicatedStruct& Context);
	UFUNCTION(Server, Reliable)
	void SyncBroadcastContext_Server(const FMounteaDialogueContextReplicatedStruct& Context);
	
protected:

	UPROPERTY(VisibleAnywhere, Category="Dialogue")
	TArray<TScriptInterface<IMounteaDialogueManagerInterface>> Managers;
};
