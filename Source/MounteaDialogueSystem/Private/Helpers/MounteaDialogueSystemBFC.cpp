// All rights reserved Dominik Pavlicek 2023


#include "Helpers/MounteaDialogueSystemBFC.h"

#include "Kismet/KismetSystemLibrary.h"

#include "Graph/MounteaDialogueGraph.h"

#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#include "Components/AudioComponent.h"
#include "Data/MounteaDialogueContext.h"
#include "GameFramework/PlayerState.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Sound/SoundBase.h"

bool UMounteaDialogueSystemBFC::IsEditor()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
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
	if (!Node || !Participant || !Participant.GetObject() || !Node->Graph)
		return false;

	const TArray<FDialogueTraversePath>& TraversedPaths = Participant->Execute_GetTraversedPath(Participant.GetObject());
	const FDialogueTraversePath* FoundPath = TraversedPaths.FindByPredicate([&](const FDialogueTraversePath& Path)
	{
		return Path.NodeGuid == Node->GetNodeGUID() && Path.GraphGuid == Node->Graph->GetGraphGUID();
	});

	return FoundPath != nullptr;
}

bool UMounteaDialogueSystemBFC::HasNodeBeenTraversedV2(const UMounteaDialogueGraphNode* Node,  const UMounteaDialogueContext* Context)
{
	if (!Node || !Context || !Node->Graph)
		return false;
	
	const TArray<FDialogueTraversePath>& TraversedPaths = Context->TraversedPath;

	const FDialogueTraversePath* FoundPath = TraversedPaths.FindByPredicate([&](const FDialogueTraversePath& Path)
	{
		return Path.NodeGuid == Node->GetNodeGUID() && Path.GraphGuid == Node->Graph->GetGraphGUID();
	});

	return FoundPath != nullptr;
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
			return Itr;
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
			return Itr;
	}
	
	return nullptr;
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::GetPlayerDialogueParticipant(AActor* WorldContextObject)
{
	if (WorldContextObject == nullptr) return nullptr;
	
	if (WorldContextObject->Implements<UMounteaDialogueParticipantInterface>())
		return WorldContextObject;

	if (UActorComponent* ParticipantComp = WorldContextObject->FindComponentByInterface(UMounteaDialogueParticipantInterface::StaticClass()))
		return ParticipantComp;

	APawn* playerPawn = Cast<APawn>(WorldContextObject);
	if (playerPawn)
		return GetPlayerDialogueParticipant(playerPawn);

	const APlayerState* playerState = Cast<APlayerState>(WorldContextObject);
	if (playerState)
		return GetPlayerDialogueParticipant(playerState->GetPawn());
	
	const APlayerController* playerController = Cast<APlayerController>(WorldContextObject);
	if (playerController)
		return GetPlayerDialogueParticipant(playerController->GetPawn());
	
	return nullptr;
}

bool UMounteaDialogueSystemBFC::IsContextValid(const UMounteaDialogueContext* Context)
{
	if (Context == nullptr) return false;

	return Context->IsValid();
}

bool UMounteaDialogueSystemBFC::ExecuteDecorators(const UObject* WorldContextObject, const UMounteaDialogueContext* DialogueContext)
{
	if (DialogueContext == nullptr)
		return false;

	if (DialogueContext->DialogueParticipant.GetInterface() == nullptr || DialogueContext->DialogueParticipant.GetObject() == nullptr)
		return false;

	UObject* participantObject = DialogueContext->DialogueParticipant.GetObject();
	if (DialogueContext->DialogueParticipant->Execute_GetDialogueGraph(participantObject) == nullptr)
		return false;

	const auto ActiveNode = DialogueContext->GetActiveNode();
	if (ActiveNode == nullptr)
		return false;

	// First process Node Decorators, then Graph Decorators
	// TODO: add weight to them so we can sort them by Weight and execute in correct order
	TArray<FMounteaDialogueDecorator> AllDecorators;

	AllDecorators.Append(DialogueContext->GetActiveNode()->GetNodeDecorators());
	if (ActiveNode->DoesInheritDecorators())
		AllDecorators.Append(DialogueContext->DialogueParticipant->Execute_GetDialogueGraph(participantObject)->GetGraphDecorators());
		
	for (auto Itr : AllDecorators)
		Itr.ExecuteDecorator();

	return true;
}

bool UMounteaDialogueSystemBFC::AddParticipants(AActor* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	const TScriptInterface<IMounteaDialogueManagerInterface> Manager = GetDialogueManager(WorldContextObject);
	if (!Manager)
		return false;

	UMounteaDialogueContext* Context = Manager->Execute_GetDialogueContext(Manager.GetObject());

	if (!Context)
		return false;

	return Context->AddDialogueParticipants(NewParticipants);
}

bool UMounteaDialogueSystemBFC::RemoveParticipants(AActor* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
{
	const TScriptInterface<IMounteaDialogueManagerInterface> Manager = GetDialogueManager(WorldContextObject);
	if (!Manager)
		return false;

	UMounteaDialogueContext* Context = Manager->Execute_GetDialogueContext(Manager.GetObject());

	if (!Context)
		return false;

	return Context->RemoveDialogueParticipants(NewParticipants);
}

TScriptInterface<IMounteaDialogueManagerInterface> UMounteaDialogueSystemBFC::GetDialogueManager(UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	if (WorldContextObject->GetWorld() == nullptr) return nullptr;

	if (WorldContextObject->Implements<UMounteaDialogueManagerInterface>())
		return WorldContextObject;

	
	if (AActor* ActorContext = Cast<AActor>(WorldContextObject))
	{
		UActorComponent* ManagerComponent = ActorContext->FindComponentByInterface(UMounteaDialogueManagerInterface::StaticClass());
		if (ManagerComponent)
			return ManagerComponent;
	}
	

	LOG_WARNING(TEXT("[Get Dialogue Manager] Cannot find Dialogue Manager the easy way."))
	
	const APlayerController* PlayerController = WorldContextObject->GetWorld()->GetFirstPlayerController();

	if (!PlayerController) return nullptr;

	if (UActorComponent* ManagerComponent = PlayerController->FindComponentByInterface(UMounteaDialogueManagerInterface::StaticClass()))
		return ManagerComponent;
		
	LOG_ERROR(TEXT("[Get Dialogue Manager] Unable to find Dialogue Manager."));
	return nullptr;
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::FindBestMatchingParticipant(const UMounteaDialogueContext* Context)
{
	if (!IsValid(Context))
		return nullptr;

	if (IsValid(Context->ActiveNode))
	{
		if (const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Context->ActiveNode))
		{
			const FDialogueRow Row = GetDialogueRow(DialogueNode);
			const auto Predicate = [&Row](const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant)
			{
				return Row.CompatibleTags.HasTagExact(Participant->Execute_GetParticipantTag(Participant.GetObject()));
			};
	
			if (const TScriptInterface<IMounteaDialogueParticipantInterface>* MatchingParticipant = Context->GetDialogueParticipants().FindByPredicate(Predicate))
				return *MatchingParticipant;
		}
	}

	LOG_WARNING(TEXT("[FindBestMatchingParticipant] Unable to find Dialogue Participant based on Gameplay Tags, returning first (index 0) Participant from Dialogue Context!"))
	return Context->DialogueParticipants.Num() > 0 ? Context->DialogueParticipants[0] : nullptr;
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

TArray<UMounteaDialogueGraphNode*> UMounteaDialogueSystemBFC::FindNodesByGUID(const UMounteaDialogueGraph* FromGraph, const TArray<FGuid> Guids)
{
	TArray<UMounteaDialogueGraphNode*> resultArray;
	for (const auto& Itr : Guids)
	{
		if (auto foundNode = FindNodeByGUID(FromGraph, Itr))
			resultArray.Add(foundNode);
	}

	return resultArray;
}

TArray<FGuid> UMounteaDialogueSystemBFC::NodesToGuids(TArray<UMounteaDialogueGraphNode*> Nodes)
{
	TArray<FGuid> resultArray;
	for (const auto& Itr : Nodes)
	{
		if (Itr) resultArray.Add(Itr->GetNodeGUID());
	}

	return resultArray;
}

UMounteaDialogueGraphNode* UMounteaDialogueSystemBFC::GetChildrenNodeFromIndex(const int32 Index, const UMounteaDialogueGraphNode* ParentNode)
{
	if (ParentNode->GetChildrenNodes().IsValidIndex(Index))
		return ParentNode->GetChildrenNodes()[Index];

	return nullptr;
}

UMounteaDialogueGraphNode* UMounteaDialogueSystemBFC::GetFirstChildNode(const UMounteaDialogueGraphNode* ParentNode)
{
	if (ParentNode == nullptr) return nullptr;

	if (ParentNode->GetChildrenNodes().IsValidIndex(0))
		return ParentNode->GetChildrenNodes()[0]->CanStartNode() ? ParentNode->GetChildrenNodes()[0] : nullptr;

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
			ReturnNodes.Add(Itr);
	}

	return ReturnNodes;
}

void UMounteaDialogueSystemBFC::SortNodes(TArray<UMounteaDialogueGraphNode*>& SortedNodes)
{
	SortNodes<UMounteaDialogueGraphNode>(SortedNodes);
}

UMounteaDialogueGraphNode* UMounteaDialogueSystemBFC::GetStartingNode(const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant, const UMounteaDialogueGraph* Graph)
{
	if (!IsValid(Participant.GetObject())  || !IsValid(Graph))
	{
		LOG_WARNING(TEXT("[Get Starting Node] Dialogue Participant or Graph are invalid."))
		return nullptr;
	}
	
	UMounteaDialogueGraphNode* NodeToStart = Participant->Execute_GetSavedStartingNode(Participant.GetObject());
	if (!IsValid(NodeToStart) || NodeToStart->CanStartNode() == false)
		NodeToStart = Graph->GetStartNode();
	
	if (!IsValid(NodeToStart))
	{
		LOG_ERROR(TEXT("[Get Starting Node] Dialogue Graph has no Nodes to start. Cannot Initialize dialogue."));
		return nullptr;
	}
	
	if (NodeToStart->GetClass()->IsChildOf(UMounteaDialogueGraphNode_StartNode::StaticClass()))
	{
		auto firstChildNode = GetFirstChildNode(NodeToStart);
		if (!IsValid(firstChildNode))
		{
			LOG_ERROR(TEXT("[Get Starting Node] Dialogue Graph has only Start Node and no Nodes to start. Cannot Initialize dialogue."));
			return nullptr;
		}

		NodeToStart = firstChildNode;
	}

	return NodeToStart;
}

UMounteaDialogueContext* UMounteaDialogueSystemBFC::CreateDialogueContext(UObject* NewOwner, const TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants)
{
	if (!IsValid(NewOwner))
	{
		LOG_WARNING(TEXT("[Create Dialogue Context] Invalid Owner for Dialogue Context!"))
		return nullptr;
	}
	
	if (!IsValid(MainParticipant.GetObject()))
	{
		LOG_WARNING(TEXT("[Create Dialogue Context] Invalid Main Participant for Dialogue Context!"))
		return nullptr;
	}

	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> AllDialogueParticipants = DialogueParticipants;
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
			LOG_INFO(TEXT("[Create Dialogue Context] Dialogue Participant %d is invalid! Participant will be ignored."), DialogueParticipants.Find(Itr));
			continue;
		}
		
		if (!Itr->Execute_GetOwningActor(Itr.GetObject()))
		{
			UnavailableParticipants.Add(Itr);
			LOG_INFO(TEXT("[Create Dialogue Context] Dialogue Participant %d has no Owning Actor. Check whether function `GetOwningActor` is implemented. Participant will be ignored."), DialogueParticipants.Find(Itr));
			continue;
		}

		if (!Itr->Execute_CanParticipateInDialogue(Itr.GetObject()))
		{
			UnavailableParticipants.Add(Itr);
			LOG_INFO(TEXT("[Create Dialogue Context] Dialogue Participant %d cannot Participate in Dialogue, so it will be ignored"), DialogueParticipants.Find(Itr))
		}

		TScriptInterface<IMounteaDialogueManagerInterface> dialogueManager = NewOwner;
		Itr->Execute_InitializeParticipant(Itr.GetObject(), dialogueManager);
	}

	if (UnavailableParticipants.Num() == AllDialogueParticipants.Num())
	{
		LOG_ERROR(TEXT("[Create Dialogue Context] None of %d Dialogue Participants can Start Dialogue!"), DialogueParticipants.Num())
		return nullptr;
	}
	
	UMounteaDialogueContext* newDialogueContext = NewObject<UMounteaDialogueContext>(NewOwner);

	const UMounteaDialogueGraph* dialogueGraph = MainParticipant->Execute_GetDialogueGraph(MainParticipant.GetObject());
		
	auto newActiveNode = GetStartingNode(MainParticipant, dialogueGraph);
	auto allowedChildNodes = GetAllowedChildNodes(newActiveNode);

	auto newActiveDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(newActiveNode);
	FDataTableRowHandle newDialogueTableHandle = FDataTableRowHandle();
	newDialogueTableHandle.DataTable = newActiveDialogueNode ? newActiveDialogueNode->GetDataTable() : nullptr;
	newDialogueTableHandle.RowName = newActiveDialogueNode ? newActiveDialogueNode->GetRowName() : NAME_None;

	newDialogueContext->SetDialogueContext(MainParticipant, newActiveNode, allowedChildNodes);
	newDialogueContext->UpdateActiveDialogueTable(newActiveDialogueNode ? newDialogueTableHandle : FDataTableRowHandle());
	newDialogueContext->AddDialogueParticipants(DialogueParticipants);
	UpdateMatchingDialogueParticipant(newDialogueContext, SwitchActiveParticipant(newDialogueContext));

	return newDialogueContext;
}

UMounteaDialogueContext* UMounteaDialogueSystemBFC::CreateDialogueContext(UObject* NewOwner, const FMounteaDialogueContextReplicatedStruct& NewData)
{
	if (!IsValid(NewOwner))
	{
		LOG_WARNING(TEXT("[Create Dialogue Context] Invalid Owner for Dialogue Context!"))
		return nullptr;
	}

	UMounteaDialogueContext* newDialogueContext = NewObject<UMounteaDialogueContext>(NewOwner);
	(*newDialogueContext) += NewData;
	
	return newDialogueContext;
}

AActor* UMounteaDialogueSystemBFC::GetDialogueManagerLocalOwner(const UObject* Manager)
{
	if (!IsValid(Manager))
		return nullptr;

	const APlayerState* playerState = Cast<APlayerState>(Manager);
	if (IsValid(playerState))
	{
		if (IsValid(playerState->GetPlayerController()))
			return playerState->GetPlayerController();
		return  playerState->GetPawn();
	}

	auto worldContext = Manager->GetWorld();
	if (!IsValid(worldContext))
		return nullptr;

	return worldContext->GetFirstPlayerController();
}

AActor* UMounteaDialogueSystemBFC::GetDialogueManagerLocalOwner(const TScriptInterface<const IMounteaDialogueManagerInterface>& Manager)
{
	if (!IsValid(Manager.GetObject()))
		return nullptr;

	const APlayerState* playerState = Cast<APlayerState>(Manager->Execute_GetOwningActor(Manager.GetObject()));
	if (IsValid(playerState))
	{
		if (IsValid(playerState->GetPlayerController()))
			return playerState->GetPlayerController();
		return  playerState->GetPawn();
	}

	// TODO: Make sure this is local run only, so we can get the first Player
	auto worldContext = Manager.GetObject()->GetWorld();
	if (!IsValid(worldContext))
		return nullptr;

	return worldContext->GetFirstPlayerController();
}

ENetRole UMounteaDialogueSystemBFC::GetOwnerLocalRole(const AActor* ForActor)
{
	if (!IsValid(ForActor))
		return ROLE_None;
	return ForActor->GetLocalRole();
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::SwitchActiveParticipant(const UMounteaDialogueContext* DialogueContext)
{
	if (!IsValid(DialogueContext) || !IsValid(DialogueContext->ActiveNode))
		return nullptr;

	const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& dialogueParticipants = DialogueContext->DialogueParticipants;
	if (dialogueParticipants.Num() == 0)
		return DialogueContext->ActiveDialogueParticipant;

	FGameplayTagContainer activeTags;
	if (DialogueContext->ActiveNode->IsA<UMounteaDialogueGraphNode_DialogueNodeBase>())
		activeTags.AppendTags(DialogueContext->ActiveDialogueRow.CompatibleTags);
	activeTags.AppendTags(DialogueContext->ActiveNode->NodeGameplayTags);
	
	if (activeTags.Num() == 0)
		return IsValid(DialogueContext->ActiveDialogueParticipant.GetObject()) ? DialogueContext->ActiveDialogueParticipant : dialogueParticipants[0];
	
	auto* foundParticipant = dialogueParticipants.FindByPredicate([&](const TScriptInterface<IMounteaDialogueParticipantInterface>& dialogueParticipant)
	{
		return dialogueParticipant.GetObject() && activeTags.HasTagExact(dialogueParticipant->Execute_GetParticipantTag(dialogueParticipant.GetObject()));
	});

	return (foundParticipant && *foundParticipant != DialogueContext->ActiveDialogueParticipant) ? *foundParticipant : DialogueContext->ActiveDialogueParticipant;
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::FindParticipantByTag(const UMounteaDialogueContext* DialogueContext, const FGameplayTag& SearchTag)
{
	if (!IsValid(DialogueContext))
		return nullptr;

	const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& dialogueParticipants = DialogueContext->DialogueParticipants;
	if (dialogueParticipants.Num() == 0)
		return DialogueContext->ActiveDialogueParticipant;

	for (const auto& dialogueParticipant : dialogueParticipants)
	{
		if (!IsValid(dialogueParticipant.GetObject()))
			continue;

		if (dialogueParticipant->Execute_GetParticipantTag(dialogueParticipant.GetObject()).MatchesTag(SearchTag))
			return dialogueParticipant;
	}

	return DialogueContext->ActiveDialogueParticipant;
}

bool UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(UMounteaDialogueContext* Context, const TScriptInterface<IMounteaDialogueParticipantInterface>& NewActiveParticipant)
{
	if (!IsValid(Context))
		return false;

	if (Context->ActiveDialogueParticipant == NewActiveParticipant)
		return false;

	Context->ActiveDialogueParticipant = NewActiveParticipant;
	Context->OnDialogueContextUpdated.Broadcast();
	return true;
}

FDialogueRowData UMounteaDialogueSystemBFC::GetActiveDialogueData(const UMounteaDialogueContext* Context, bool& bResult)
{
	if (!IsValid(Context))
	{ bResult = false; return FDialogueRowData(); }

	const int32 activeIndex = Context->GetActiveDialogueRowDataIndex();
	const auto Row = Context->GetActiveDialogueRow();
	bResult = Row.DialogueRowData.Array().IsValidIndex(activeIndex);

	if (!bResult)
		return FDialogueRowData();
	
	const FDialogueRowData rowData = Row.DialogueRowData.Array()[activeIndex];
	bResult = IsDialogueRowDataValid(rowData);

	return bResult ? rowData : FDialogueRowData();
}

bool UMounteaDialogueSystemBFC::DoesRowMatchParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& ParticipantInterface, const FDialogueRow& Row)
{
	if (!Row.IsValid())
		return false;

	if (!IsValid(ParticipantInterface.GetObject()))
		return false;

	if (Row.CompatibleTags.IsEmpty())
	{
		LOG_WARNING(TEXT("[Does Row Match Participant] Row has now Tags to compare against!"))
		return false;
	}

	const auto participantTag = ParticipantInterface->Execute_GetParticipantTag(ParticipantInterface.GetObject());
	if (!participantTag.IsValid())
	{
		LOG_WARNING(TEXT("[Does Row Match Participant] Participant Tag is not valid!"))
		return false;
	}

	return Row.CompatibleTags.HasTagExact(participantTag);
}

FDialogueRow UMounteaDialogueSystemBFC::GetDialogueRow(const UMounteaDialogueGraphNode* Node)
{
	if (!Node)
	{
		LOG_ERROR(TEXT("[GetDialogueRow] Invalid Node input!"))
		return FDialogueRow::Invalid();
	}
	const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		
	if (!DialogueNodeBase)
	{
		LOG_WARNING(TEXT("[GetDialogueRow] Invalid Dialogue Node input!"))
		return FDialogueRow::Invalid();
	}
	if (DialogueNodeBase->GetDataTable() == nullptr)
	{
		LOG_ERROR(TEXT("[GetDialogueRow] Node %s has empty Data Table!"), *DialogueNodeBase->GetNodeTitle().ToString())
		return FDialogueRow::Invalid();
	}
	if (DialogueNodeBase->GetDataTable()->RowStruct->IsChildOf(FDialogueRow::StaticStruct()) == false)
	{
		LOG_ERROR(TEXT("[GetDialogueRow] Node %s has invalid Data Table data!"), *DialogueNodeBase->GetNodeTitle().ToString())
		return FDialogueRow::Invalid();
	}

	const FDialogueRow* Row = DialogueNodeBase->GetDataTable()->FindRow<FDialogueRow>(DialogueNodeBase->GetRowName(), FString("") );
	if (!Row)
	{
		LOG_WARNING(TEXT("[GetDialogueRow] Node %s has no Row Data by ID: %s!"), *DialogueNodeBase->GetNodeTitle().ToString(), *DialogueNodeBase->GetRowName().ToString())
		return FDialogueRow::Invalid();
	}
	if (IsDialogueRowValid(*Row) == false)
	{
		LOG_ERROR(TEXT("[GetDialogueRow] Node %s has invalid Dialogue Row %s"), *DialogueNodeBase->GetNodeTitle().ToString(), *DialogueNodeBase->GetRowName().ToString())
		return FDialogueRow::Invalid();
	}

	return *Row;
}

FDialogueRow UMounteaDialogueSystemBFC::GetDialogueRow(const UDataTable* SourceTable, const FName& SourceName)
{
	if (!SourceTable)
		return FDialogueRow::Invalid();

	const FDialogueRow* FoundRow = SourceTable->FindRow<FDialogueRow>(SourceName, TEXT(""));
	return FoundRow ? *FoundRow : FDialogueRow::Invalid();
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
			break;
		}
		case ERowDurationMode::EDRM_Override:
		{
			ReturnValue = Row.RowDurationOverride;
			break;
		}
		case ERowDurationMode::EDRM_Add:
		{
			if (Row.RowSound)
			{
				ReturnValue = Row.RowSound->Duration;
				ReturnValue = ReturnValue + Row.RowDurationOverride;
				break;
			}
			ReturnValue = Row.RowDurationOverride;
			break;
		}
		case ERowDurationMode::ERDM_AutoCalculate:
		{
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

	// Timer running in background need some time and replication takes a few ms, too
	ReturnValue = FMath::Max(UE_KINDA_SMALL_NUMBER, ReturnValue);
		
	return ReturnValue;
}

TArray<FMounteaDialogueDecorator> UMounteaDialogueSystemBFC::GetAllDialogueDecorators(const UMounteaDialogueGraph* FromGraph)
{
	TArray<FMounteaDialogueDecorator> Decorators;

	if (FromGraph == nullptr) return Decorators;
		
	Decorators.Append(FromGraph->GetAllDecorators());

	for (const auto& Itr : FromGraph->GetAllNodes())
	{
		if (Itr)
			Decorators.Append(Itr->GetNodeDecorators());
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

APawn* UMounteaDialogueSystemBFC::FindPlayerPawn(AActor* ForActor, int& SearchDepth)
{
	SearchDepth++;
	if (SearchDepth >= 8)
		return nullptr;
	
	if (APawn* playerPawn = Cast<APawn>(ForActor))
		return playerPawn;

	if (APlayerState* playerState = Cast<APlayerState>(ForActor))
		return playerState->GetPawn();

	if (APlayerController* playerController = Cast<APlayerController>(ForActor))
	{
		if (playerController->IsLocalPlayerController())
			return playerController->GetPawn();
		return FindPlayerPawn(playerController->PlayerState, SearchDepth);
	}

	if (AActor* ownerActor = ForActor->GetOwner())
		return FindPlayerPawn(ownerActor, SearchDepth);

	return nullptr;
}

APlayerController* UMounteaDialogueSystemBFC::FindPlayerController(AActor* ForActor, int& SearchDepth)
{
	SearchDepth++;
	if (SearchDepth >= 8)
		return nullptr;
	
	if (APlayerController* playerController = Cast<APlayerController>(ForActor))
		return playerController;

	if (APlayerState* playerState = Cast<APlayerState>(ForActor))
		return playerState->GetPlayerController();

	if (APawn* actorPawn = Cast<APawn>(ForActor))
		return Cast<APlayerController>(actorPawn->GetController());
	
	if (AActor* ownerActor = ForActor->GetOwner())
		return FindPlayerController(ownerActor, SearchDepth);
	
	return nullptr;
}

bool UMounteaDialogueSystemBFC::DoesNodeInvertSkipSettings(UMounteaDialogueGraphNode* Node)
{
	return Node ? Node->bInvertSkipRowSetting : false;
}

bool UMounteaDialogueSystemBFC::DoesPreviousNodeSkipActiveNode(const UMounteaDialogueGraph* ParentGraph, const FGuid PreviousNode)
{
	if (!ParentGraph) return false;

	UMounteaDialogueGraphNode* tempNode = FindNodeByGUID(ParentGraph, PreviousNode);

	if (tempNode == nullptr) return false;

	if (const UMounteaDialogueGraphNode_ReturnToNode* returnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(tempNode))
		return returnNode->bAutoCompleteSelectedNode;
	
	return false;
}

ERowExecutionMode UMounteaDialogueSystemBFC::GetActiveRowExecutionMode(UMounteaDialogueContext* DialogueContext, const int32 RowIndex)
{
	constexpr ERowExecutionMode result = ERowExecutionMode::EREM_Automatic;

	if (!DialogueContext)
		return result;

	if (RowIndex < 0)
		return result;

	auto activeRow = DialogueContext->GetActiveDialogueRow();
	if (!activeRow.IsValid())
		return result;

	const TArray<FDialogueRowData> rowDataArray = activeRow.DialogueRowData.Array();
	if (!rowDataArray.IsValidIndex(RowIndex))
		return result;

	auto activeRowData = rowDataArray[RowIndex];
	if (!IsDialogueRowDataValid(activeRowData))
		return result;

	return activeRowData.RowExecutionBehaviour;
}

UObject* UMounteaDialogueSystemBFC::GetObjectByClass(UObject* Object, const TSubclassOf<UObject> ClassFilter, bool& bResult)
{
	if (ClassFilter == nullptr)
	{
		bResult = false;
		return nullptr;
	}

	if (Object == nullptr)
	{
		bResult = false;
		return nullptr;
	}

	if (Object->IsA(ClassFilter))
	{
		bResult = true;
		return Object;
	}

	bResult = false;
	return nullptr;
}

UActorComponent* UMounteaDialogueSystemBFC::GetSingleComponentByInterface(const AActor* Target, TSubclassOf<UInterface> InterfaceFilter, bool& bResult)
{
	bResult = false;
	if (Target == nullptr) return nullptr;

	TArray<UActorComponent*> TempComps = Target->GetComponentsByInterface(InterfaceFilter);

	if (TempComps.IsEmpty()) return nullptr;

	bResult = true;
	return TempComps[0];
}
