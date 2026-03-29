// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueParticipant.h"

#include "Components/AudioComponent.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Settings/MounteaDialogueSystemSettings.h"
#include "Sound/SoundBase.h"

#if WITH_EDITOR
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#endif

namespace
{
	template <typename TComponent>
	void AddUniqueComponentPointer(TArray<TComponent*>& InOutComponents, TSet<const UObject*>& InOutSeen, TComponent* Candidate)
	{
		if (!IsValid(Candidate))
			return;

		if (InOutSeen.Contains(Candidate))
			return;

		InOutSeen.Add(Candidate);
		InOutComponents.Add(Candidate);
	}

	UClass* ResolveTargetActorClass(const UActorComponent* Component)
	{
		if (!IsValid(Component))
			return nullptr;

		if (const AActor* ownerActor = Component->GetOwner())
			return ownerActor->GetClass();

		if (const AActor* outerClass = Cast<AActor>(Component->GetOuter()))
			return outerClass->GetClass();

		if (const UClass* outerClass = Cast<UClass>(Component->GetOuter()))
			return outerClass->IsChildOf(AActor::StaticClass()) ? const_cast<UClass*>(outerClass) : nullptr;

		return nullptr;
	}
}

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

	auto audioComponent = FindAudioComponent();
	if (IsValid(audioComponent))
		Execute_SetAudioComponent(this, audioComponent);
	else
		LOG_WARNING(TEXT("[Begin Play] Participant %s has invalid audio component. Voice will be player unbound and skipping might lead to issues."), *GetName())

	Execute_InitializeParticipant(this, DialogueManager);

	// Force replicate Owner to avoid setup issues with less experienced users
	auto participantOwner = GetOwner();
	if (IsValid(participantOwner) && !participantOwner->GetIsReplicated() && GetIsReplicated())
		participantOwner->SetReplicates(GetIsReplicated());
}

void UMounteaDialogueParticipant::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ParticipantState == EDialogueParticipantState::EDPS_Active)
		Execute_TickMounteaEvent(this, this, nullptr, DeltaTime);
}

void UMounteaDialogueParticipant::InitializeParticipant_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (DialogueGraph == nullptr) return;

	if (DialogueManager != Manager)
		DialogueManager = Manager;
	
	for (const auto& Itr : DialogueGraph->GetAllNodes())
	{
		if (Itr)
			Itr->InitializeNode(GetWorld());
	}
	
	TArray<FMounteaDialogueDecorator> Decorators = UMounteaDialogueSystemBFC::GetAllDialogueDecorators(DialogueGraph);
	for (const auto& Itr : Decorators)
	{
		if (Itr.DecoratorType)
			Itr.DecoratorType->InitializeDecorator(GetWorld(), this, Manager);
	}
}

UAudioComponent* UMounteaDialogueParticipant::FindAudioComponent() const
{
	if (IsValid(AudioComponent)) 
		return AudioComponent;
	
	if (const auto audioComponent = FindAudioComponentByName(AudioComponentIdentification))
		return audioComponent;

	if (const auto audioComponent = FindAudioComponentByTag(AudioComponentIdentification))
		return audioComponent;

	LOG_INFO(TEXT("[FindAudioComponent] No Audio Component found with by Identification (%s)\nFirst Audio Component will be used instead. To override this behaviour implement 'FindAudioComponent' event."), *AudioComponentIdentification.ToString())

	if (!GetOwner()) 
		return nullptr;

	UAudioComponent* firstFoundAudioComp = GetOwner()->FindComponentByClass<UAudioComponent>();
	return firstFoundAudioComp;
}

UAudioComponent* UMounteaDialogueParticipant::FindAudioComponentByName(const FName& Arg) const
{
	if (GetOwner() == nullptr) 
		return nullptr;

	return UMounteaDialogueSystemBFC::FindAudioComponentByName(GetOwner(), Arg);
}

UAudioComponent* UMounteaDialogueParticipant::FindAudioComponentByTag(const FName& Arg) const
{
	if (GetOwner() == nullptr) 
		return nullptr;

	return UMounteaDialogueSystemBFC::FindAudioComponentByTag(GetOwner(), Arg);
}

void UMounteaDialogueParticipant::PlayParticipantVoice_Implementation(USoundBase* ParticipantVoice)
{
	if(!UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		LOG_INFO(TEXT("[PlayParticipantVoice] Voice cannot be played at Dedicated Server!"))
		return;
	}
	
	if (IsValid(AudioComponent) && IsValid(ParticipantVoice))
	{
		AudioComponent->SetSound(ParticipantVoice);
		AudioComponent->Play();
	}
}

void UMounteaDialogueParticipant::SkipParticipantVoice_Implementation(USoundBase* ParticipantVoice)
{
	if(!UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		LOG_INFO(TEXT("[PlayParticipantVoice] Voice cannot be played at Dedicated Server!"))
		return;
	}
	
	if (IsValid(AudioComponent))
	{
		AudioComponent->StopDelayed(UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal()->GetSkipFadeDuration());
		AudioComponent->SetSound(nullptr);
	}
}

TArray<FName> UMounteaDialogueParticipant::GetAvailableParticipants() const
{
	const auto dialogueSettings = GetDefault<UMounteaDialogueSystemSettings>();
	if (!IsValid(dialogueSettings))
		return {};
	const auto dialogueConfig = dialogueSettings->GetDialogueConfiguration().LoadSynchronous();
	if (!IsValid(dialogueConfig))
		return {};
	if (dialogueConfig->DialogueParticipantsTables.Num() == 0)
		return {};
	TSet<FName> returnValue;
	returnValue.Reserve(dialogueConfig->DialogueParticipantsTables.Num() * 4);
	for (const auto& participantsTable : dialogueConfig->DialogueParticipantsTables)
	{
		if (!IsValid(participantsTable.LoadSynchronous()))
			continue;
		returnValue.Append(participantsTable.LoadSynchronous()->GetRowNames());
	}	
	
	return returnValue.Array();
}

TArray<FName> UMounteaDialogueParticipant::GetAvailableAudioComponentsNames() const
{
	const TArray<UAudioComponent*> audioComponents = GetAvailableAudioComponents();

	TArray<FName> returnValue;
	returnValue.Reserve(audioComponents.Num());

	TSet<FName> uniqueNames;
	uniqueNames.Reserve(audioComponents.Num());

	for (const UAudioComponent* audioComponent : audioComponents)
	{
		if (!IsValid(audioComponent))
		continue;

		FString audioCompName = audioComponent->GetFName().ToString();
		audioCompName.RemoveFromEnd(TEXT("_GEN_VARIABLE"));

		const FName finalName(*audioCompName);
		if (!uniqueNames.Contains(finalName))
		{
			uniqueNames.Add(finalName);
			returnValue.Add(finalName);
		}
	}

	return returnValue;
}

TArray<UAudioComponent*> UMounteaDialogueParticipant::GetAvailableAudioComponents() const
{
	TArray<UAudioComponent*> returnValue;
	TSet<const UObject*> ceenComponents;

	if (const AActor* ownerActor = GetOwner())
	{
		TArray<UAudioComponent*> instanceComponents;
		ownerActor->GetComponents<UAudioComponent>(instanceComponents);
		for (UAudioComponent* audioComponent : instanceComponents)
		{
			AddUniqueComponentPointer(returnValue, ceenComponents, audioComponent);
		}
	}

	UClass* rargetClass = ResolveTargetActorClass(this);
	if (!IsValid(rargetClass))
		return returnValue;

	if (AActor* actorCDO = rargetClass->GetDefaultObject<AActor>())
	{
		TArray<UAudioComponent*> defaultComponents;
		actorCDO->GetComponents<UAudioComponent>(defaultComponents);
		for (UAudioComponent* audioComponent : defaultComponents)
		{
			AddUniqueComponentPointer(returnValue, ceenComponents, audioComponent);
		}
	}

#if WITH_EDITOR
	for (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(rargetClass); BPClass; BPClass = Cast<UBlueprintGeneratedClass>(BPClass->GetSuperClass()))
	{
		if (!BPClass->SimpleConstructionScript)
			continue;

		const TArray<USCS_Node*>& scriptNodes = BPClass->SimpleConstructionScript->GetAllNodes();
		for (const USCS_Node* scriptNode : scriptNodes)
		{
			if (!scriptNode)
				continue;

			UActorComponent* componentTemplate = scriptNode->GetActualComponentTemplate(BPClass);
			UAudioComponent* audioTemplate = Cast<UAudioComponent>(componentTemplate);
			AddUniqueComponentPointer(returnValue, ceenComponents, audioTemplate);
		}
	}
#endif

	return returnValue;
}

bool UMounteaDialogueParticipant::CanStartDialogue_Implementation() const
{
	return Execute_CanParticipateInDialogue(this) &&
		(IsValid(DialogueGraph) && DialogueGraph->CanStartDialogueGraph());
}

bool UMounteaDialogueParticipant::CanParticipateInDialogue_Implementation() const
{
	return ParticipantState == EDialogueParticipantState::EDPS_Enabled;
}

void UMounteaDialogueParticipant::SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode)
{
	if (!DialogueGraph) 
		return;
	if (!DialogueGraph->GetAllNodes().Contains(NewStartingNode)) 
		return;

	StartingNode = NewStartingNode;

	OnStartingNodeSaved.Broadcast(StartingNode);
}

void UMounteaDialogueParticipant::SetDialogueGraph_Implementation(UMounteaDialogueGraph* NewDialogueGraph)
{
	if (ParticipantState == EDialogueParticipantState::EDPS_Active) 
		return;

	if (NewDialogueGraph == DialogueGraph) 
		return;

	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetParticipantState] Component has no Owner!"))
		return;
	}

	if (IsValid(DialogueGraph))
	{
#if WITH_EDITORONLY_DATA
		UnregisterFromPIEInstance();
#endif
		DialogueGraph->ShutdownGraph();
	}
		
	DialogueGraph = NewDialogueGraph;
	
	if (IsValid(DialogueGraph))
	{
		DialogueGraph->InitializeGraph();
#if WITH_EDITORONLY_DATA
		RegisterWithPIEInstance();
#endif
	}

	Execute_InitializeParticipant(this, DialogueManager);
		
	OnDialogueGraphChanged.Broadcast(NewDialogueGraph);
	
	if (!GetOwner()->HasAuthority())
		SetDialogueGraph_Server(NewDialogueGraph);
}

EDialogueParticipantState UMounteaDialogueParticipant::GetParticipantState_Implementation() const
{
	return  ParticipantState;
};

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
		SetParticipantState_Server(NewState);
	else
	{
		ParticipantState = NewState;
		UpdateParticipantTick();
		OnDialogueParticipantStateChanged.Broadcast(NewState);

		switch (ParticipantState)
		{
			case EDialogueParticipantState::EDPS_Disabled:
			case EDialogueParticipantState::Default:
			case EDialogueParticipantState::EDPS_Enabled:
			{
#if WITH_EDITORONLY_DATA
				UnregisterFromPIEInstance();
#endif
				DialogueManager = nullptr;
				break;
			}
			case EDialogueParticipantState::EDPS_Active:
			{
				if (DialogueGraph)
				{
					DialogueGraph->InitializeGraph();
#if WITH_EDITORONLY_DATA
					RegisterWithPIEInstance();
#endif
				}
				break;
			}
		}
	}
}

void UMounteaDialogueParticipant::SetDefaultParticipantState_Implementation(const EDialogueParticipantState NewState)
{
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetDefaultParticipantState] Component has no Owner!"))
		return;
	}
	if (GetOwner()->HasAuthority())
		DefaultParticipantState = NewState;
	else
		SetDefaultParticipantState_Server(NewState);
}

void UMounteaDialogueParticipant::SetAudioComponent_Implementation(UAudioComponent* NewAudioComponent)
{
	if (AudioComponent == NewAudioComponent) 
		return;
	
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
	TMap<TPair<FGuid, FGuid>, int32> pathMap;
	pathMap.Reserve(TraversedPath.Num() + InPath.Num());
	
	for (const auto& Path : TraversedPath)
	{
		pathMap.Add(Path.GetGuidPair(), Path.TraverseCount);
	}

	for (const auto& Path : InPath)
	{
		int32& traversedCount = pathMap.FindOrAdd(Path.GetGuidPair());
		traversedCount += Path.TraverseCount;
	}
	
	TraversedPath.Empty(pathMap.Num());
	for (const auto& Pair : pathMap)
	{
		TraversedPath.Add(FDialogueTraversePath(Pair.Key.Key, Pair.Key.Value, Pair.Value));
	}
}

void UMounteaDialogueParticipant::RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	SetComponentTickEnabled(true);

	if (auto dialogueGraph = Execute_GetDialogueGraph(this))
		dialogueGraph->Execute_RegisterTick(dialogueGraph, this);
}

void UMounteaDialogueParticipant::UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	SetComponentTickEnabled(false);
	
	if (auto parentGraph = Execute_GetDialogueGraph(this))
		parentGraph->Execute_UnregisterTick(parentGraph, this);
}

void UMounteaDialogueParticipant::TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick,float DeltaTime)
{
	ParticipantTickEvent.Broadcast(SelfRef, ParentTick, DeltaTime);
}

void UMounteaDialogueParticipant::OnRep_DialogueGraph()
{
	Execute_InitializeParticipant(this, DialogueManager);
}

void UMounteaDialogueParticipant::UpdateParticipantTick()
{
	switch (ParticipantState)
	{
		case EDialogueParticipantState::EDPS_Disabled:
		case EDialogueParticipantState::EDPS_Enabled:
			Execute_UnregisterTick(this, nullptr);
			break;
		case EDialogueParticipantState::EDPS_Active:
			Execute_RegisterTick(this, nullptr);
			break;
	}
}

void UMounteaDialogueParticipant::OnRep_ParticipantState()
{
	OnDialogueParticipantStateChanged.Broadcast(ParticipantState);

	switch (ParticipantState)
	{
		case EDialogueParticipantState::EDPS_Disabled:
		case EDialogueParticipantState::Default:
		case EDialogueParticipantState::EDPS_Enabled:
		{
#if WITH_EDITORONLY_DATA
			UnregisterFromPIEInstance();
#endif
			break;
		}
		case EDialogueParticipantState::EDPS_Active:
		{
			if (DialogueGraph)
			{
				DialogueGraph->InitializeGraph();
#if WITH_EDITORONLY_DATA
				RegisterWithPIEInstance();
#endif
			}
			break;
		}
	}
	
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

	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, DialogueGraph,				COND_AutonomousOnly);
	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, DefaultParticipantState,	COND_AutonomousOnly);
	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, TraversedPath,				COND_AutonomousOnly);
	DOREPLIFETIME_CONDITION(UMounteaDialogueParticipant, StartingNode,				COND_AutonomousOnly);
	
	DOREPLIFETIME(UMounteaDialogueParticipant, ParticipantState);
	DOREPLIFETIME(UMounteaDialogueParticipant, ParticipantTag);
}

#if WITH_EDITORONLY_DATA

void UMounteaDialogueParticipant::RegisterWithPIEInstance()
{
	if (IsValid(DialogueGraph))
	{
		const int32 PIEInstanceID = GetCurrentPIEInstanceID();
		DialogueGraph->InitializePIEInstance(this, PIEInstanceID, true);
	}
}

void UMounteaDialogueParticipant::UnregisterFromPIEInstance()
{
	if (IsValid(DialogueGraph))
	{
		const int32 PIEInstanceID = GetCurrentPIEInstanceID();
		DialogueGraph->InitializePIEInstance(this, PIEInstanceID, false);
	}
}

int32 UMounteaDialogueParticipant::GetCurrentPIEInstanceID() const
{
	if (const UWorld* World = GetWorld())
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() == World && Context.WorldType == EWorldType::PIE)
				return Context.PIEInstance;
		}
	}
	return -1;
}

#endif

#if WITH_EDITOR

void UMounteaDialogueParticipant::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueParticipant, ParticipantRow))
	{
		const FDialogueParticipant* foundParticipantData = UMounteaDialogueParticipantStatics::FindParticipantDataRow(ParticipantRow, &ParticipantTag);
		ParticipantData = const_cast<FDialogueParticipant*>(foundParticipantData);
	}
	
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueParticipant, AudioComponentIdentification))
	{
		AudioComponent = FindAudioComponent();
	}
}

#endif
