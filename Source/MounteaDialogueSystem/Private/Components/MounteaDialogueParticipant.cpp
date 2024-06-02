// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueParticipant.h"

#include "Components/AudioComponent.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Nodes/MounteaDialogueGraphNode.h"

UMounteaDialogueParticipant::UMounteaDialogueParticipant()
{
	DefaultParticipantState = EDialogueParticipantState::EDPS_Enabled;

	SetIsReplicatedByDefault(true);
	SetActiveFlag(true);

	bAutoActivate = true;

	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMounteaDialogueParticipant::BeginPlay()
{
	Super::BeginPlay();

	OnDialogueGraphChanged.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueGraphChangedEvent);

	SetParticipantState(GetDefaultParticipantState());

	SetAudioComponent(FindAudioComponent());

	Execute_InitializeParticipant(this);
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

	if (AudioComponentIdentification.IsNone()) return nullptr;

	if (AudioComponentIdentification.IsValid() == false) return nullptr;
	
	if (const auto Return = FindAudioComponentByName(AudioComponentIdentification))
	{
		return Return;
	}

	if (const auto Return = FindAudioComponentByTag(AudioComponentIdentification))
	{
		return Return;
	}

	LOG_WARNING(TEXT("[FindAudioComponent] No Audio Component found with by %s"), *AudioComponentIdentification.ToString())
	
	return nullptr;
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

void UMounteaDialogueParticipant::PlayParticipantVoice(USoundBase* ParticipantVoice)
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
}

void UMounteaDialogueParticipant::SkipParticipantVoice(USoundBase* ParticipantVoice)
{
	if(!UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		LOG_INFO(TEXT("[PlayParticipantVoice] Voice cannot be played at Dedicated Server!"))
		return;
	}
	
	if (AudioComponent)
	{
		AudioComponent->SetSound(nullptr);
		AudioComponent->StopDelayed(UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal()->GetSkipFadeDuration());
	}
}

bool UMounteaDialogueParticipant::CanStartDialogue() const
{
	switch (GetParticipantState())
	{
		case EDialogueParticipantState::EDPS_Active:
		case EDialogueParticipantState::EDPS_Disabled:
			return false;
		case EDialogueParticipantState::EDPS_Enabled:
			return true;
	}

	return true;
}

void UMounteaDialogueParticipant::SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode)
{
	if (!DialogueGraph) return;
	if (!DialogueGraph->GetAllNodes().Contains(NewStartingNode)) return;

	StartingNode = NewStartingNode;

	OnStartingNodeSaved.Broadcast(StartingNode);
}

void UMounteaDialogueParticipant::SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph)
{
	if (ParticipantState == EDialogueParticipantState::EDPS_Active) return;

	if (NewDialogueGraph == DialogueGraph) return;;

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

void UMounteaDialogueParticipant::SetParticipantState(const EDialogueParticipantState NewState)
{
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetParticipantState] Component has no Owner!"))
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		ParticipantState = NewState;

		UpdateParticipantTick();

		OnDialogueParticipantStateChanged.Broadcast(NewState);
	}
	else
	{
		SetParticipantState_Server(NewState);
	}
}

void UMounteaDialogueParticipant::SetDefaultParticipantState(const EDialogueParticipantState NewState)
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

void UMounteaDialogueParticipant::SetAudioComponent(UAudioComponent* NewAudioComponent)
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
		SetAudioComponent_Server(NewAudioComponent);
	}
}

AActor* UMounteaDialogueParticipant::GetOwningActor_Implementation() const
{
	return GetOwner();
}

//TODO: instead of the Actor handling this logic realtime, make FRunnable queue and let data be calculated async way
void UMounteaDialogueParticipant::SaveTraversedPath_Implementation(TArray<FDialogueTraversePath>& InPath)
{
	TMap<FGuid, FDialogueTraversePath> PathMap;

	// Insert or update existing entries from CurrentPath
	for (auto& Path : TraversedPath)
	{
		PathMap.Add(Path.NodeGuid, Path);
	}

	// Update counts or add new entries from InPath
	for (auto& Path : InPath)
	{
		if (FDialogueTraversePath* FoundPath = PathMap.Find(Path.NodeGuid))
		{
			FoundPath->TraverseCount += Path.TraverseCount; // Increment existing count
		}
		else
		{
			PathMap.Add(Path.NodeGuid, Path); // Add new entry
		}
	}

	// Convert map back to array
	TraversedPath.Empty();
	for (const auto& Pair : PathMap)
	{
		TraversedPath.Add(Pair.Value);
	}

	// Traverse path has been updated! Great job.
}

FGameplayTag UMounteaDialogueParticipant::GetParticipantTag() const
{
	return ParticipantTag;
}

void UMounteaDialogueParticipant::RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	SetComponentTickEnabled(true);

	if (GetDialogueGraph())
	{
		GetDialogueGraph()->Execute_RegisterTick(GetDialogueGraph(), this);
	}
}

void UMounteaDialogueParticipant::UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	SetComponentTickEnabled(false);

	if (GetDialogueGraph())
	{
		GetDialogueGraph()->Execute_UnregisterTick(GetDialogueGraph(), this);
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
	SetDialogueGraph(NewGraph);
}

void UMounteaDialogueParticipant::SetAudioComponent_Server_Implementation(UAudioComponent* NewAudioComponent)
{
	SetAudioComponent(NewAudioComponent	);
}

void UMounteaDialogueParticipant::SetDefaultParticipantState_Server_Implementation(const EDialogueParticipantState NewState)
{
	SetDefaultParticipantState(NewState);
}

void UMounteaDialogueParticipant::SetParticipantState_Server_Implementation(const EDialogueParticipantState NewState)
{
	SetParticipantState(NewState);
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
