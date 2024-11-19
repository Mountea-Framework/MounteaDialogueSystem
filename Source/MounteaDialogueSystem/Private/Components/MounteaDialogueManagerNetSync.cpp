// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Components/MounteaDialogueManagerNetSync.h"

UMounteaDialogueManagerNetSync::UMounteaDialogueManagerNetSync()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void UMounteaDialogueManagerNetSync::BeginPlay()
{
	Super::BeginPlay();
}

void UMounteaDialogueManagerNetSync::AddManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager)
{
	Managers.Add(NewManager);
}

void UMounteaDialogueManagerNetSync::SyncBroadcastContext(const FMounteaDialogueContextReplicatedStruct& Context)
{
	
}

void UMounteaDialogueManagerNetSync::SyncBroadcastContext_Server_Implementation(const FMounteaDialogueContextReplicatedStruct& Context)
{
	
}

