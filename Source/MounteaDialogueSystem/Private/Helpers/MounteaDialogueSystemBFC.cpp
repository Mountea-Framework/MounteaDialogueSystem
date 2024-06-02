// All rights reserved Dominik Pavlicek 2023


#include "Helpers/MounteaDialogueSystemBFC.h"

#include "Kismet/KismetSystemLibrary.h"

#include "Graph/MounteaDialogueGraph.h"

#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#include "Components/AudioComponent.h"
#include "Data/MounteaDialogueContext.h"
#include "GameFramework/PlayerState.h"
#include "Sound/SoundBase.h"

bool UMounteaDialogueSystemBFC::IsEditor()
{
#if WITH_EDITOR
	return true;
#endif

	return false;
}

void UMounteaDialogueSystemBFC::CleanupGraph(const UObject* WorldContextObject, const UMounteaDialogueGraph* GraphToClean)
{
	if (!GraphToClean) return;
		
	// Cleanup Decorators
	for (auto Itr : GraphToClean->GetAllDecorators())
	{
		Itr.CleanupDecorator();
	}
}

bool UMounteaDialogueSystemBFC::HasNodeBeenTraversed(const UMounteaDialogueGraphNode* Node, const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant)
{
	bool bTraversed = false;

	if (!Node) return bTraversed;
	if (!Participant || !Participant.GetObject()) return bTraversed;

	bTraversed = Participant->GetTraversedPath().Contains(Node->GetNodeGUID());
		
	return bTraversed;
}

UAudioComponent* UMounteaDialogueSystemBFC::FindAudioComponentByName(const AActor* ActorContext, const FName& Arg)
{
	if (ActorContext == nullptr) return nullptr;

	TArray<UAudioComponent*> OwnerComponents;
	ActorContext->GetComponents<UAudioComponent>(OwnerComponents);
	
	if (OwnerComponents.Num() == 0) return nullptr;

	for (const auto& Itr : OwnerComponents)
	{
		if (Itr && Itr->GetName().Equals(Arg.ToString()))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

UAudioComponent* UMounteaDialogueSystemBFC::FindAudioComponentByTag(const AActor* ActorContext, const FName& Arg)
{
	if (ActorContext == nullptr) return nullptr;

	TArray<UAudioComponent*> OwnerComponents;
	ActorContext->GetComponents<UAudioComponent>(OwnerComponents);
	
	if (OwnerComponents.Num() == 0) return nullptr;
		
	for (const auto& Itr : OwnerComponents)
	{
		if (Itr && Itr->ComponentHasTag(Arg))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::GetPlayerDialogueParticipant(AActor* WorldContextObject)
{
	if (WorldContextObject == nullptr) return nullptr;
	
	if (WorldContextObject->Implements<UMounteaDialogueParticipantInterface>())
	{
		return WorldContextObject;
	}

	if (UActorComponent* ParticipantComp = WorldContextObject->FindComponentByInterface(UMounteaDialogueParticipantInterface::StaticClass()))
	{
		return ParticipantComp;
	}
	
	const APlayerController* PlayerController = WorldContextObject->GetWorld()->GetFirstPlayerController();

	if (!PlayerController) return nullptr;

	const APawn* PlayerPawn = PlayerController->GetPawn();

	if (PlayerPawn == nullptr) return nullptr;

	return PlayerPawn->FindComponentByInterface(UMounteaDialogueParticipantInterface::StaticClass());
}

bool UMounteaDialogueSystemBFC::IsContextValid(const UMounteaDialogueContext* Context)
{
	if (Context == nullptr) return false;

	return Context->IsValid();
}

bool UMounteaDialogueSystemBFC::ExecuteDecorators(const UObject* WorldContextObject, const UMounteaDialogueContext* DialogueContext)
{
	if (DialogueContext == nullptr)
	{
		return false;
	}

	if (DialogueContext->DialogueParticipant.GetInterface() == nullptr)
	{
		return false;
	}

	if (DialogueContext->DialogueParticipant->GetDialogueGraph() == nullptr)
	{
		return false;
	}

	const auto ActiveNode = DialogueContext->GetActiveNode();
	if (ActiveNode == nullptr)
	{
		return false;
	}

	// First process Node Decorators, then Graph Decorators
	// TODO: add weight to them so we can sort them by Weight and execute in correct order
	TArray<FMounteaDialogueDecorator> AllDecorators;

	AllDecorators.Append(DialogueContext->GetActiveNode()->GetNodeDecorators());
	if (ActiveNode->DoesInheritDecorators())
	{
		AllDecorators.Append(DialogueContext->DialogueParticipant->GetDialogueGraph()->GetGraphDecorators());
	}
		
	for (auto Itr : AllDecorators)
	{
		Itr.ExecuteDecorator();
	}

	return true;
}

bool UMounteaDialogueSystemBFC::CloseDialogue(AActor* WorldContextObject, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant)
{
	if (!GetDialogueManager(WorldContextObject))
	{
		LOG_ERROR(TEXT("[CloseDialogue] Cannot find Dialogue Manager. Cannot close dialogue."));
		return false;
	}
		
	UMounteaDialogueContext* Context = NewObject<UMounteaDialogueContext>();
	Context->SetDialogueContext(DialogueParticipant, nullptr, TArray<UMounteaDialogueGraphNode*>());
		
	GetDialogueManager(WorldContextObject)->GetDialogueClosedEventHandle().Broadcast(Context);
	return true;
}

bool UMounteaDialogueSystemBFC::StartDialogue(const UObject* WorldContextObject, APlayerState* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants)
{
	if (!WorldContextObject)
	{
		LOG_ERROR(TEXT("[StartDialogue] Missing WorldContextObject. Cannot start dialogue."));
		return false;
	}

	if (!Initiator)
	{
		LOG_ERROR(TEXT("[StartDialogue] Missing Initiator Player Contorller. Cannot start dialogue."));
		return false;
	}

	if (MainParticipant.GetInterface() == nullptr || MainParticipant.GetObject() == nullptr)
	{
		LOG_ERROR(TEXT("[StartDialogue] Invalid Main Participant!. Cannot start dialogue."));
		return false;
	}

	if (MainParticipant->Execute_CanStartDialogueEvent(MainParticipant.GetObject()) == false)
	{
		LOG_ERROR(TEXT("[StartDialogue]  Main Participant cannot starti Dialogue!. Cannot start dialogue."));
		return false;
	}

	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> AllDialogueParticipants = DialogueParticipants;
	if (AllDialogueParticipants.Contains(MainParticipant) == false)
	{
		AllDialogueParticipants.Add(MainParticipant);
	}

	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> UnavailableParticipants;
	for (auto Itr : AllDialogueParticipants)
	{
		if (Itr.GetInterface() == nullptr && Itr.GetObject() != nullptr)
		{
			// When passing interfaces through array sometimes they loose the interface pointer, this might solve the issue - however its dirty af
			Itr.SetInterface(Cast<IMounteaDialogueParticipantInterface>(Itr.GetObject()));
		}
		if (Itr.GetInterface() == nullptr || Itr.GetObject() == nullptr)
		{
			UnavailableParticipants.Add(Itr);
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Participant %d is invalid! Participant will be ignored."), DialogueParticipants.Find(Itr));
			continue;
		}
		
		if (!Itr->Execute_GetOwningActor(Itr.GetObject()))
		{
			UnavailableParticipants.Add(Itr);
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Participant %d has no Owning Actor. Check whether function `GetOwningActor` is implemented. Participant will be ignored."), DialogueParticipants.Find(Itr));
			continue;
		}

		if (!Itr->Execute_CanStartDialogueEvent(Itr.GetObject()))
		{
			UnavailableParticipants.Add(Itr);
			LOG_INFO(TEXT("[StartDialogue] Dialogie Participant %d cannot Start Dialogue, so it will be ignored"), DialogueParticipants.Find(Itr))
		}

		Itr->Execute_InitializeParticipant(Itr.GetObject());
	}

	if (UnavailableParticipants.Num() == AllDialogueParticipants.Num())
	{
		LOG_ERROR(TEXT("[StartDialogue] None of %d Dialogue Participants can Start Dialogue! Dialogue will not launch!"), DialogueParticipants.Num())
		return false;
	}
	
	UWorld* TempWorld = WorldContextObject->GetWorld();
	if (!TempWorld) TempWorld = Initiator->GetWorld();

	const TScriptInterface<IMounteaDialogueManagerInterface> DialogueManager = GetDialogueManager(Initiator);
	if (DialogueManager == nullptr)
	{
		LOG_ERROR(TEXT("[StartDialogue] Unable to find Dialogue Manager. Cannot Initialize dialogue."));
		return false;
	}

	switch (DialogueManager->Execute_GetState(DialogueManager.GetObject() ))
	{
		case EDialogueManagerState::EDMS_Disabled:
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Manager is Disabled!"))
			return false;
		case EDialogueManagerState::EDMS_Enabled:
			break;
		case EDialogueManagerState::EDMS_Active:
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Manager is already Active!"))
			return false;
	}
	
	const UMounteaDialogueGraph* Graph = MainParticipant->GetDialogueGraph();

	if (!Graph)
	{
		LOG_ERROR(TEXT("[StartDialogue] WorldContextObject is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	if (Graph->CanStartDialogueGraph() == false)
	{
		LOG_ERROR(TEXT("[StartDialogue] Dialogue Graph cannot Start. Cannot Initialize dialogue."));
		return false;
	}

	UMounteaDialogueGraphNode* NodeToStart = MainParticipant->GetSavedStartingNode();
	if (!NodeToStart || NodeToStart->CanStartNode() == false)
	{
		NodeToStart = Graph->GetStartNode();
	}
	
	if (NodeToStart == nullptr)
	{
		LOG_ERROR(TEXT("[StartDialogue] Dialogue Graph has no Nodes to start. Cannot start dialogue."));
		return false;
	}
	
	if (NodeToStart->GetClass()->IsChildOf(UMounteaDialogueGraphNode_StartNode::StaticClass()))
	{
		if (GetFirstChildNode(NodeToStart) == nullptr)
		{
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Graph has only Start Node and no Nodes to start. Cannot start dialogue."));
			return false;
		}

		NodeToStart = GetFirstChildNode(NodeToStart);
	}

	const TArray<FMounteaDialogueDecorator> graphDecorators = Graph->GetAllDecorators();
	for (const auto& Itr : graphDecorators)
	{
		if (Itr.DecoratorType)
		{
			Itr.DecoratorType->SetOwningManager(DialogueManager);
		}
	}
	
	const TArray<UMounteaDialogueGraphNode*> StartNode_Children = GetAllowedChildNodes(NodeToStart);
	
	for (const auto& Itr : Graph->GetAllNodes())
	{
		if (Itr)
		{
			Itr->InitializeNode(TempWorld);
		}
	}

	for (auto Itr : Graph->GetAllDecorators())
	{
		Itr.InitializeDecorator(TempWorld, MainParticipant, DialogueManager);
	}

	UMounteaDialogueContext* Context = NewObject<UMounteaDialogueContext>();
	Context->SetDialogueContext(MainParticipant, NodeToStart, StartNode_Children);
	Context->UpdateDialoguePlayerParticipant(GetPlayerDialogueParticipant(Initiator));
	Context->AddDialogueParticipants(DialogueParticipants);
	
	return InitializeDialogueWithContext(WorldContextObject, Initiator, MainParticipant, Context);
}

bool UMounteaDialogueSystemBFC::InitializeDialogue(const UObject* WorldContextObject, AActor* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface>& DialogueParticipant)
{
	if (!DialogueParticipant)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Missing DialogueParticipant. Cannot Initialize dialogue."));
		return false;
	}

	if (!DialogueParticipant->Execute_GetOwningActor(DialogueParticipant.GetObject()))
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Participant found no Owning Actor. Check whether function `GetOwningActor` is implemented. Cannot Initialize dialogue."));
		return false;
	}
	
	UWorld* TempWorld = Initiator->GetWorld();
	if (!TempWorld) TempWorld = DialogueParticipant->Execute_GetOwningActor(DialogueParticipant.GetObject())->GetWorld();
	
	if (Initiator == nullptr && DialogueParticipant.GetInterface() == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Initiator is empty AND Participant is invalid. Cannot Initialize dialogue."));
		return false;
	}

	const TScriptInterface<IMounteaDialogueManagerInterface> DialogueManager = GetDialogueManager(Initiator);
	if (DialogueManager== nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] WorldContextObject is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	if (DialogueParticipant->CanStartDialogue() == false)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] WorldContextObject is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	const UMounteaDialogueGraph* Graph = DialogueParticipant->GetDialogueGraph();

	if (Graph == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue participant has no Graph. Cannot Initialize dialogue."));
		return false;
	}

	for (const auto& Itr : Graph->GetAllNodes())
	{
		if (Itr)
		{
			Itr->InitializeNode(TempWorld);
		}
	}

	for (auto Itr : Graph->GetAllDecorators())
	{
		Itr.InitializeDecorator(TempWorld, DialogueParticipant, DialogueManager);
	}

	if (Graph->CanStartDialogueGraph() == false)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Graph cannot Start. Cannot Initialize dialogue."));
		return false;
	}

	UMounteaDialogueGraphNode* NodeToStart = DialogueParticipant->GetSavedStartingNode();
	if (!NodeToStart || NodeToStart->CanStartNode() == false)
	{
		NodeToStart = Graph->GetStartNode();
	}
	
	if (NodeToStart == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Graph has no Nodes to start. Cannot Initialize dialogue."));
		return false;
	}
	
	if (NodeToStart->GetClass()->IsChildOf(UMounteaDialogueGraphNode_StartNode::StaticClass()))
	{
		if (GetFirstChildNode(NodeToStart) == nullptr)
		{
			LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Graph has only Start Node and no Nodes to start. Cannot Initialize dialogue."));
			return false;
		}

		NodeToStart = GetFirstChildNode(NodeToStart);
	}
	
	const TArray<UMounteaDialogueGraphNode*> StartNode_Children = GetAllowedChildNodes(NodeToStart);

	UMounteaDialogueContext* Context = NewObject<UMounteaDialogueContext>();
	Context->SetDialogueContext(DialogueParticipant, NodeToStart, StartNode_Children);
	Context->UpdateDialoguePlayerParticipant(GetPlayerDialogueParticipant(Initiator));
	
	return  InitializeDialogueWithContext(WorldContextObject, Initiator, DialogueParticipant, Context);
}

bool UMounteaDialogueSystemBFC::InitializeDialogueWithContext(const UObject* WorldContextObject, AActor* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant, UMounteaDialogueContext* Context)
{
	if (DialogueParticipant == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogueWithContext] Missing DialogueParticipant. Cannot Initialize dialogue."));
		return false;
	}
	if (Context == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogueWithContext] Missing Dialogue Context. Cannot Initialize dialogue."));
		return false;
	}
	if (IsContextValid(Context) == false)
	{
		LOG_ERROR(TEXT("[InitializeDialogueWithContext] Dialogue Context is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	const auto DialogueManager = GetDialogueManager(Initiator);
	if (DialogueManager == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogueWithContext] Dialogue Manager is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	DialogueManager->GetDialogueInitializedEventHandle().Broadcast(Context);
	return true;
}

bool UMounteaDialogueSystemBFC::AddParticipants(AActor* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	const TScriptInterface<IMounteaDialogueManagerInterface> Manager = GetDialogueManager(WorldContextObject);
	if (!Manager)
	{
		return false;
	}

	UMounteaDialogueContext* Context = Manager->GetDialogueContext();

	if (!Context)
	{
		return false;
	}

	return Context->AddDialogueParticipants(NewParticipants);
}

bool UMounteaDialogueSystemBFC::RemoveParticipants(AActor* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	const TScriptInterface<IMounteaDialogueManagerInterface> Manager = GetDialogueManager(WorldContextObject);
	if (!Manager)
	{
		return false;
	}

	UMounteaDialogueContext* Context = Manager->GetDialogueContext();

	if (!Context)
	{
		return false;
	}

	return Context->RemoveDialogueParticipants(NewParticipants);		
}

TScriptInterface<IMounteaDialogueManagerInterface> UMounteaDialogueSystemBFC::GetDialogueManager(UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	if (WorldContextObject->GetWorld() == nullptr) return nullptr;

	if (WorldContextObject->Implements<UMounteaDialogueManagerInterface>())
	{
		return WorldContextObject;
	}

	
	if (AActor* ActorContext = Cast<AActor>(WorldContextObject))
	{
		UActorComponent* ManagerComponent = ActorContext->FindComponentByInterface(UMounteaDialogueManagerInterface::StaticClass());
		if (ManagerComponent)
		{
			return ManagerComponent;
		}
	}
	

	LOG_WARNING(TEXT("[GetDialogueManager] Cannot find Dialogue Manager the easy way."))
	
	const APlayerController* PlayerController = WorldContextObject->GetWorld()->GetFirstPlayerController();

	if (!PlayerController) return nullptr;

	if (UActorComponent* ManagerComponent = PlayerController->FindComponentByInterface(UMounteaDialogueManagerInterface::StaticClass()))
	{
		return ManagerComponent;
	}
		
	LOG_ERROR(TEXT("[GetDialogueManager] Unable to find Dialogue Manager."));
	return nullptr;
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::FindBestMatchingParticipant(const UObject* WorldContextObject, const UMounteaDialogueContext* Context)
{
	if (!Context)
	{
		return nullptr;
	}

	if (!Context->ActiveNode)
	{
		return nullptr;
	}

	const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Context->ActiveNode);
	if (!DialogueNode)
	{
		return nullptr;
	}

	for (auto const& Participant : Context->GetDialogueParticipants())
	{
		const FGameplayTag Tag = Participant->Execute_GetTag(Participant.GetObject());

		const FDialogueRow Row = GetDialogueRow(DialogueNode);
		if (Row.CompatibleTags.HasTagExact(Tag))
		{
			return Participant;
		}
	}

	LOG_ERROR(TEXT("[FindBestMatchingParticipant] Unable to find Dialogue Participant based on Gameplay Tags, returning first (index 0) Participant from Dilaogue Context!"))
	return Context->DialogueParticipants[0];
}

UMounteaDialogueGraphNode* UMounteaDialogueSystemBFC::FindNodeByGUID(const UMounteaDialogueGraph* FromGraph, const FGuid ByGUID)
{
	if (!FromGraph) return nullptr;
	if (!ByGUID.IsValid()) return nullptr;

	for (const auto& Itr : FromGraph->GetAllNodes())
	{
		if (Itr && Itr->GetNodeGUID() == ByGUID) return Itr;
	}

	return nullptr;
}

UMounteaDialogueGraphNode* UMounteaDialogueSystemBFC::GetChildrenNodeFromIndex(const int32 Index, const UMounteaDialogueGraphNode* ParentNode)
{
	if (ParentNode->GetChildrenNodes().IsValidIndex(Index))
	{
		return ParentNode->GetChildrenNodes()[Index];
	}

	return nullptr;
}

UMounteaDialogueGraphNode* UMounteaDialogueSystemBFC::GetFirstChildNode(const UMounteaDialogueGraphNode* ParentNode)
{
	if (ParentNode == nullptr) return nullptr;

	if (ParentNode->GetChildrenNodes().IsValidIndex(0))
	{
		return ParentNode->GetChildrenNodes()[0]->CanStartNode() ? ParentNode->GetChildrenNodes()[0] : nullptr;
	}

	return nullptr;
}

TArray<UMounteaDialogueGraphNode*> UMounteaDialogueSystemBFC::GetAllowedChildNodes( const UMounteaDialogueGraphNode* ParentNode)
{
	TArray<UMounteaDialogueGraphNode*> ReturnNodes;

	if (!ParentNode) return ReturnNodes;

	if (ParentNode->GetChildrenNodes().Num() == 0) return ReturnNodes;

	for (UMounteaDialogueGraphNode* Itr : ParentNode->GetChildrenNodes())
	{
		if (Itr && Itr->CanStartNode())
		{
			ReturnNodes.Add(Itr);
		}
	}

	return ReturnNodes;
}

FDialogueRow UMounteaDialogueSystemBFC::GetDialogueRow(const UMounteaDialogueGraphNode* Node)
{
	const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		
	if (!DialogueNodeBase) return FDialogueRow();
	if (DialogueNodeBase->GetDataTable() == nullptr) return FDialogueRow();
	if (DialogueNodeBase->GetDataTable()->RowStruct->IsChildOf(FDialogueRow::StaticStruct()) == false) return FDialogueRow();

	const FDialogueRow* Row = DialogueNodeBase->GetDataTable()->FindRow<FDialogueRow>(DialogueNodeBase->GetRowName(), FString("") );
	if (!Row) return FDialogueRow();
	if (IsDialogueRowValid(*Row) == false) return FDialogueRow();

	return *Row;
}

float UMounteaDialogueSystemBFC::GetRowDuration(const FDialogueRowData& Row)
{
	float ReturnValue = 1.f;
	switch (Row.RowDurationMode)
	{
		case ERowDurationMode::ERDM_Duration:
			{
				if (Row.RowSound)
				{
					ReturnValue = Row.RowSound->Duration;
					break;
				}
						
				ReturnValue =  Row.RowDuration;
			}
			break;
		case ERowDurationMode::EDRM_Override:
			{
				ReturnValue = Row.RowDurationOverride;
			}
			break;
		case ERowDurationMode::EDRM_Add:
			{
				if (Row.RowSound)
				{
					ReturnValue = Row.RowSound->Duration;
					ReturnValue = ReturnValue + Row.RowDurationOverride;
					break;
				}
				ReturnValue = Row.RowDurationOverride;
			}
			break;
		case ERowDurationMode::ERDM_AutoCalculate:
			{
				//TODO: Make 8:100 ratio editable in Settings!
				if (GetDialogueSystemSettings_Internal())
				{
					ReturnValue= ((Row.RowText.ToString().Len() * GetDialogueSystemSettings_Internal()->GetDurationCoefficient()) / 100.f);
				}
				else
				{
					ReturnValue= ((Row.RowText.ToString().Len() * 8.f) / 100.f);
				}
				break;
			}
	}

	ReturnValue = FMath::Max(1.f, ReturnValue);
		
	return ReturnValue;
}

TArray<FMounteaDialogueDecorator> UMounteaDialogueSystemBFC::GetAllDialogueDecorators(const UMounteaDialogueGraph* FromGraph)
{
	TArray<FMounteaDialogueDecorator> Decorators;

	if (FromGraph == nullptr) return Decorators;
		
	Decorators.Append(FromGraph->GetGraphDecorators());

	for (const auto& Itr : FromGraph->GetAllNodes())
	{
		if (Itr)
		{
			Decorators.Append(Itr->GetNodeDecorators());
		}
	}
		
	return Decorators;
}

bool UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(const UWorld* WorldContext)
{
	return !UKismetSystemLibrary::IsDedicatedServer(WorldContext);
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(AActor* ParticipantActor, bool& bResult)
{
	bResult = false;
	
	if (ParticipantActor == nullptr)
	{
		LOG_ERROR(TEXT("[FindDialogueParticipantInterface] Participating Actor is empty! Returning null."))
		return nullptr;
	}

	TScriptInterface<IMounteaDialogueParticipantInterface> resultValue;
	if (ParticipantActor->Implements<UMounteaDialogueParticipantInterface>())
	{
		resultValue = ParticipantActor;
		bResult = true;
		return resultValue;
	}

	TArray<UActorComponent*> actorComponets = ParticipantActor->GetComponentsByInterface(UMounteaDialogueParticipantInterface::StaticClass());
	if (actorComponets.Num() == 0)
	{
		LOG_ERROR(TEXT("[FindDialogueParticipantInterface] Actor %s has no Dialogue Participant Component!"), *ParticipantActor->GetName())
		return nullptr;
	}

	resultValue = actorComponets[0];
	bResult = true;
	return resultValue;
}

APlayerController* UMounteaDialogueSystemBFC::FindPlayerController(AActor* ForActor)
{
	if (APlayerController* playerController = Cast<APlayerController>(ForActor))
	{
		return playerController;
	}

	if (APlayerState* playerState = Cast<APlayerState>(ForActor))
	{
		return playerState->GetPlayerController();
	}

	if (APawn* actorPawn = Cast<APawn>(ForActor))
	{
		return Cast<APlayerController>(actorPawn->GetController());
	}
	
	// Check the owner recursively, sorry performance :(
	// TODO: Limit depth!
	if (AActor* ownerActor = ForActor->GetOwner())
	{
		return FindPlayerController(ownerActor);
	}
	
	return nullptr;
}
