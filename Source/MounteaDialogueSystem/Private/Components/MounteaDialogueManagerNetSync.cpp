// All rights reserved Dominik Morse (Pavlicek) 2024

#include "Components/MounteaDialogueManagerNetSync.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"

UMounteaDialogueManagerNetSync::UMounteaDialogueManagerNetSync()
{
	bAutoActivate = true;
	SetIsReplicatedByDefault(true);
	SetActiveFlag(true);
	PrimaryComponentTick.bCanEverTick = false;

	ComponentTags.Add(FName("Mountea"));
	ComponentTags.Add(FName("Dialogue"));
	ComponentTags.Add(FName("Manager"));
	ComponentTags.Add(FName("Sync"));
}

void UMounteaDialogueManagerNetSync::BeginPlay()
{
	Super::BeginPlay();
    
	if (!GetOwner() || !GetOwner()->IsA(APlayerController::StaticClass()))
		SetActive(false);
}

void UMounteaDialogueManagerNetSync::RouteRPC_Server_Implementation(APlayerController* Instigator, const FGenericRPCPayload& Payload)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_MounteaDialogue_RPCServer); // For profiling

	if (!Instigator)
	{
		LOG_ERROR(TEXT("Server received invalid Instigator"));
		return;
	}

	FServerFunctionCall FuncCall;
	AActor* DialogueInitiator = nullptr;
	FDialogueParticipants InitialParticipants;
    
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_MounteaDialogue_UnpackPayload);
		Payload.Unpack(FuncCall, DialogueInitiator, InitialParticipants);
	}
    
	if (!FuncCall.IsValid())
	{
		LOG_ERROR(TEXT("Invalid function call data"));
		return;
	}

	UFunction* Function = nullptr;
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_MounteaDialogue_FindFunction);
		Function = FuncCall.Caller->FindFunction(FuncCall.FunctionName);
	}

	if (Function)
	{
		struct FParams
		{
			AActor* InInitiator;
			FDialogueParticipants InParticipants;
		} Params{DialogueInitiator, InitialParticipants};
        
		QUICK_SCOPE_CYCLE_COUNTER(STAT_MounteaDialogue_ProcessEvent);
		FuncCall.Caller->ProcessEvent(Function, &Params);
	}
}