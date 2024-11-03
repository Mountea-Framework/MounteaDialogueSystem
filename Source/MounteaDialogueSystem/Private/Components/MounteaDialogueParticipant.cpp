// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueParticipant.h"

#include "Components/AudioComponent.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Nodes/MounteaDialogueGraphNode.h"

UMounteaDialogueParticipant::UMounteaDialogueParticipant()
	: DefaultParticipantState(EDialogueParticipantState::EDPS_Enabled)
{
	SetIsReplicatedByDefault(true);
	SetActiveFlag(true);

	bAutoActivate = true;

	PrimaryComponentTick.bStartWithTickEnabled = false;

	ComponentTags.Add(FName("Mountea"));
	ComponentTags.Add(FName("Dialogue"));
	ComponentTags.Add(FName("Participant"));
}

void UMounteaDialogueParticipant::BeginPlay()
{
	Super::BeginPlay();

	OnDialogueGraphChanged.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueGraphChangedEvent);

	Execute_SetParticipantState(this, Execute_GetDefaultParticipantState(this));

	Execute_SetAudioComponent(this,FindAudioComponent());

	Execute_InitializeParticipant(this);

	// Force replicate Owner to avoid setup issues with less experienced users
	if (GetOwner() && !GetOwner()->GetIsReplicated() && GetIsReplicated())
	{
		GetOwner()->SetReplicates(GetIsReplicated());
	}
}

void UMounteaDialogueParticipant::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ParticipantState == EDialogueParticipantState::EDPS_Active)
	{
		Execute_TickMounteaEvent(this, this, nullptr, DeltaTime);
	}
}

void UMounteaDialogueParticipant::InitializeParticipant_Implementation()
{
	if (DialogueGraph == nullptr) return;

	for (const auto& Itr : DialogueGraph->GetAllNodes())
	{
		if (Itr)
		{
			Itr->InitializeNode(GetWorld());
		}
	}
	
	TArray<FMounteaDialogueDecorator> Decorators = UMounteaDialogueSystemBFC::GetAllDialogueDecorators(DialogueGraph);
	for (const auto& Itr : Decorators)
	{
		if (Itr.DecoratorType)
		{
			Itr.DecoratorType->InitializeDecorator(GetWorld(), this, nullptr);
		}
	}
}

UAudioComponent* UMounteaDialogueParticipant::FindAudioComponent() const
{
	if (AudioComponent != nullptr) return nullptr;
	
	if (const auto Return = FindAudioComponentByName(AudioComponentIdentification))
	{
		return Return;
	}

	if (const auto Return = FindAudioComponentByTag(AudioComponentIdentification))
	{
		return Return;
	}

	LOG_WARNING(TEXT("[FindAudioComponent] No Audio Component found with by Identifiication (%s)\nFirst Audio Component will be used instead. To override this behaviour implement 'FindAudioComponent' event."), *AudioComponentIdentification.ToString())

	if (!GetOwner()) return nullptr;

	UAudioComponent* firstFoundAudioComp = GetOwner()->FindComponentByClass<UAudioComponent>();
	return firstFoundAudioComp;
}

UAudioComponent* UMounteaDialogueParticipant::FindAudioComponentByName(const FName& Arg) const
{
	if (GetOwner() == nullptr) return nullptr;

	return UMounteaDialogueSystemBFC::FindAudioComponentByName(GetOwner(), Arg);
}

UAudioComponent* UMounteaDialogueParticipant::FindAudioComponentByTag(const FName& Arg) const
{
	if (GetOwner() == nullptr) return nullptr;

	return UMounteaDialogueSystemBFC::FindAudioComponentByTag(GetOwner(), Arg);
}

void UMounteaDialogueParticipant::PlayParticipantVoice_Implementation(USoundBase* ParticipantVoice)
{
	// Audio is cosmetic -> Do not play on Dedicated server!
	if(!UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		LOG_INFO(TEXT("[PlayParticipantVoice] Voice cannot be played at Dedicated Server!"))
		return;
	}
	
	if (AudioComponent)
	{
		AudioComponent->SetSound(ParticipantVoice);
		AudioComponent->Play();
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(GetOwner(), ParticipantVoice, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation());
	}
}

void UMounteaDialogueParticipant::SkipParticipantVoice_Implementation(USoundBase* ParticipantVoice)
{
	if(!UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		LOG_INFO(TEXT("[PlayParticipantVoice] Voice cannot be played at Dedicated Server!"))
		return;
	}
	
	if (AudioComponent)
	{
		AudioComponent->StopDelayed(UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal()->GetSkipFadeDuration());
		AudioComponent->SetSound(nullptr);
	}
}

bool UMounteaDialogueParticipant::CanStartDialogue_Implementation() const
{
	return ParticipantState == EDialogueParticipantState::EDPS_Enabled && IsValid(DialogueGraph);
}

bool UMounteaDialogueParticipant::CanParticipateInDialogue_Implementation() const
{
	return ParticipantState == EDialogueParticipantState::EDPS_Enabled;
}

void UMounteaDialogueParticipant::SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode)
{
	if (!DialogueGraph) return;
	if (!DialogueGraph->GetAllNodes().Contains(NewStartingNode)) return;

	StartingNode = NewStartingNode;

	OnStartingNodeSaved.Broadcast(StartingNode);
}

void UMounteaDialogueParticipant::SetDialogueGraph_Implementation(UMounteaDialogueGraph* NewDialogueGraph)
{
	if (ParticipantState == EDialogueParticipantState::EDPS_Active) return;

	if (NewDialogueGraph == DialogueGraph) return;

	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetParticipantState] Component has no Owner!"))
		return;
	}
	
	if (GetOwner()->HasAuthority())
	{
		DialogueGraph = NewDialogueGraph;

		Execute_InitializeParticipant(this);
		
		OnDialogueGraphChanged.Broadcast(NewDialogueGraph);
	}
	else
	{
		SetDialogueGraph_Server(NewDialogueGraph);
	}
}

EDialogueParticipantState UMounteaDialogueParticipant::GetParticipantState_Implementation() const
{ return  ParticipantState; };

void UMounteaDialogueParticipant::SetParticipantState_Implementation(const EDialogueParticipantState NewState)
{
	if (NewState == ParticipantState)
	{
		LOG_INFO(TEXT("[SetParticipantState] Unable to update Participant State with same Value!"))
		return;
	}
	
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetParticipantState] Component has no Owner!"))
		return;
	}
	
	if (!GetOwner()->HasAuthority())
	{
		SetParticipantState_Server(NewState);
	}
	
	LOG_WARNING(TEXT("[SetParticipantState] Setting new state value to %s"), *UMounteaDialogueSystemBFC::GetEnumFriendlyName(NewState))

	ParticipantState = NewState;

	UpdateParticipantTick();
	
	OnDialogueParticipantStateChanged.Broadcast(NewState);
}

void UMounteaDialogueParticipant::SetDefaultParticipantState_Implementation(const EDialogueParticipantState NewState)
{
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetDefaultParticipantState] Component has no Owner!"))
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		DefaultParticipantState = NewState;
	}
	else
	{
		SetDefaultParticipantState_Server(NewState);
	}
}

void UMounteaDialogueParticipant::SetAudioComponent_Implementation(UAudioComponent* NewAudioComponent)
{
	if (AudioComponent == NewAudioComponent) return;
	
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetAudioComponent] Component has no Owner!"))
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		AudioComponent = NewAudioComponent;

		OnAudioComponentChanged.Broadcast(AudioComponent);
	}
	else
	{
		AudioComponent = NewAudioComponent;

		OnAudioComponentChanged.Broadcast(AudioComponent);
		
		SetAudioComponent_Server(NewAudioComponent);
	}
}

//TODO: instead of the Actor handling this logic realtime, make FRunnable queue and let data be calculated async way
void UMounteaDialogueParticipant::SaveTraversedPath_Implementation(TArray<FDialogueTraversePath>& InPath)
{
	TMap<TPair<FGuid, FGuid>, int32> PathMap;
	PathMap.Reserve(TraversedPath.Num() + InPath.Num());
	
	for (const auto& Path : TraversedPath)
	{
		PathMap.Add(Path.GetGuidPair(), Path.TraverseCount);
	}

	for (const auto& Path : InPath)
	{
		int32& Count = PathMap.FindOrAdd(Path.GetGuidPair());
		Count += Path.TraverseCount;
	}
	
	TraversedPath.Empty(PathMap.Num());
	for (const auto& Pair : PathMap)
	{
		TraversedPath.Add(FDialogueTraversePath(Pair.Key.Key, Pair.Key.Value, Pair.Value));
	}
}

void UMounteaDialogueParticipant::RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	SetComponentTickEnabled(true);

	if (auto dialogueGraph = Execute_GetDialogueGraph(this))
	{
		dialogueGraph->Execute_RegisterTick(dialogueGraph, this);
	}
}

void UMounteaDialogueParticipant::UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	SetComponentTickEnabled(false);
	
	if (auto parentGraph = Execute_GetDialogueGraph(this))
	{
		parentGraph->Execute_UnregisterTick(parentGraph, this);
	}
}

void UMounteaDialogueParticipant::TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick,float DeltaTime)
{
	ParticipantTickEvent.Broadcast(SelfRef, ParentTick, DeltaTime);
}

void UMounteaDialogueParticipant::OnRep_DialogueGraph()
{
	Execute_InitializeParticipant(this);
}

void UMounteaDialogueParticipant::UpdateParticipantTick()
{
	switch (ParticipantState)
	{
	case EDialogueParticipantState::EDPS_Disabled:
		Execute_UnregisterTick(this, nullptr);
		break;
	case EDialogueParticipantState::EDPS_Enabled:
		Execute_UnregisterTick(this, nullptr);
		break;
	case EDialogueParticipantState::EDPS_Active:
		Execute_RegisterTick(this, nullptr);
		break;
	}
}

void UMounteaDialogueParticipant::OnResp_ParticipantState()
{
	UpdateParticipantTick();
}

void UMounteaDialogueParticipant::SetDialogueGraph_Server_Implementation(UMounteaDialogueGraph* NewGraph)
{
	Execute_SetDialogueGraph(this, NewGraph);
}

void UMounteaDialogueParticipant::SetAudioComponent_Server_Implementation(UAudioComponent* NewAudioComponent)
{
	Execute_SetAudioComponent(this, NewAudioComponent);
}

void UMounteaDialogueParticipant::SetDefaultParticipantState_Server_Implementation(const EDialogueParticipantState NewState)
{
	Execute_SetDefaultParticipantState(this, NewState);
}

void UMounteaDialogueParticipant::SetParticipantState_Server_Implementation(const EDialogueParticipantState NewState)
{
	Execute_SetParticipantState(this, NewState);
}

void UMounteaDialogueParticipant::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, DialogueGraph, COND_AutonomousOnly);
	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, DefaultParticipantState, COND_AutonomousOnly);
	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, TraversedPath, COND_AutonomousOnly);
	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, StartingNode, COND_AutonomousOnly);
	
	DOREPLIFETIME(UMounteaDialogueParticipant, ParticipantState);
	DOREPLIFETIME(UMounteaDialogueParticipant, ParticipantTag);
}
