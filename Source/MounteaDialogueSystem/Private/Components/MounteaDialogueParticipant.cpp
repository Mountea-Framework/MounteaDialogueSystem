// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueParticipant.h"

#include "Components/AudioComponent.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode.h"

UMounteaDialogueParticipant::UMounteaDialogueParticipant()
{
	DefaultParticipantState = EDialogueParticipantState::EDPS_Enabled;
}

void UMounteaDialogueParticipant::BeginPlay()
{
	Super::BeginPlay();

	OnDialogueGraphChanged.AddUniqueDynamic(this, &UMounteaDialogueParticipant::OnDialogueGraphChangedEvent);

	SetParticipantState(GetDefaultParticipantState());

	SetAudioComponent(FindAudioComponent());

	InitializeParticipant();
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
			Itr.DecoratorType->InitializeDecorator(GetWorld(), this);
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
	if (AudioComponent)
	{
		AudioComponent->SetSound(ParticipantVoice);
		AudioComponent->Play();
	}
}

void UMounteaDialogueParticipant::SkipParticipantVoice(USoundBase* ParticipantVoice)
{
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
	
	if (NewDialogueGraph != DialogueGraph)
	{
		DialogueGraph = NewDialogueGraph;

		InitializeParticipant();
		
		OnDialogueGraphChanged.Broadcast(NewDialogueGraph);
	}
}

void UMounteaDialogueParticipant::SetParticipantState(const EDialogueParticipantState NewState)
{
	ParticipantState = NewState;

	OnDialogueParticipantStateChanged.Broadcast(NewState);
}

void UMounteaDialogueParticipant::SetDefaultParticipantState(const EDialogueParticipantState NewState)
{
	DefaultParticipantState = NewState;
}

void UMounteaDialogueParticipant::SetAudioComponent(UAudioComponent* NewAudioComponent)
{
	AudioComponent = NewAudioComponent;

	OnAudioComponentChanged.Broadcast(AudioComponent);
}

AActor* UMounteaDialogueParticipant::GetOwningActor_Implementation() const
{
	return GetOwner();
}

//TODO: instead of the Actor handling this logic realtime, make FRunnable queue and let data be calculated async way
void UMounteaDialogueParticipant::SaveTraversedPath_Implementation(TMap<FGuid, int32>& InPath)
{
	TMap<FGuid, int32> CurrentPath = TraversedPath;
	TMap<FGuid, int32> OuterJoin;
	TraversedPath.Empty();

	// Increase those Paths that have been passes in past
	for (auto& Itr : InPath)
	{
		if (CurrentPath.Contains(Itr.Key))
		{
			Itr.Value += CurrentPath[Itr.Key];
		}
	}

	// Now we have two maps - current path and new path, make outer-join
	for (auto& Itr : CurrentPath)
	{
		if (!InPath.Contains(Itr.Key))
		{
			OuterJoin.Add(Itr);
		}
	}

	// Append both, new path and paths that have been passed before but are not in current iteration
	TraversedPath.Append(InPath);
	TraversedPath.Append(OuterJoin);
}

FGameplayTag UMounteaDialogueParticipant::GetParticipantTag() const
{
	return ParticipantTag;
}
