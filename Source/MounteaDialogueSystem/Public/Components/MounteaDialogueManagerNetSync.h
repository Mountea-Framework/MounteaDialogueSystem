// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MounteaDialogueManagerNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

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

public:

	void AddManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager);

	void SyncBroadcastContext(const FMounteaDialogueContextReplicatedStruct& Context);
	UFUNCTION(Server, Reliable)
	void SyncBroadcastContext_Server(const FMounteaDialogueContextReplicatedStruct& Context);

protected:

	UPROPERTY(VisibleAnywhere, Category="Dialogue")
	TSet<TScriptInterface<IMounteaDialogueManagerInterface>> Managers;
};
