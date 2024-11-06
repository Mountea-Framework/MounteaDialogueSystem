// All rights reserved Dominik Pavlicek 2023

#include "Components/MounteaDialogueManager.h"

#include "TimerManager.h"
#include "Blueprint/GameViewportSubsystem.h"

#include "Graph/MounteaDialogueGraph.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/PlayerState.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/MounteaDialogueWBPInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"


UMounteaDialogueManager::UMounteaDialogueManager()
	: DialogueWidgetZOrder(12)
	, DefaultManagerState(EDialogueManagerState::EDMS_Enabled)
	, DialogueContext(nullptr)
{
	bAutoActivate = true;
	
	SetIsReplicatedByDefault(true);
	SetActiveFlag(true);

	PrimaryComponentTick.bStartWithTickEnabled = false;

	ComponentTags.Add(FName("Mountea"));
	ComponentTags.Add(FName("Dialogue"));
	ComponentTags.Add(FName("Manager"));
}

void UMounteaDialogueManager::BeginPlay()
{
	Super::BeginPlay();
	
	ManagerState = Execute_GetDefaultManagerState(this);

	// Force replicate Owner to avoid setup issues with less experienced users
	if (GetOwner() && !GetOwner()->GetIsReplicated() && GetIsReplicated())
	{
		GetOwner()->SetReplicates(true);
	}

	if (IsAuthority())
	{
		OnDialogueStartRequested.AddUniqueDynamic(this, &UMounteaDialogueManager::DialogueStartRequestReceived);
		OnDialogueContextUpdated.AddUniqueDynamic(this, &UMounteaDialogueManager::RequestBroadcastContext);
	}

	OnDialogueFailed.AddUniqueDynamic(this, &UMounteaDialogueManager::DialogueFailed);

	// Binding Broadcasting Events
	{
		OnDialogueStarted.AddUniqueDynamic(this, &UMounteaDialogueManager::RequestBroadcastContext);
		OnDialogueClosed.AddUniqueDynamic(this, &UMounteaDialogueManager::RequestBroadcastContext);
	
		OnDialogueNodeSelected.AddUniqueDynamic(this, &UMounteaDialogueManager::RequestBroadcastContext);
		OnDialogueNodeFinished.AddUniqueDynamic(this, &UMounteaDialogueManager::RequestBroadcastContext);

		OnDialogueRowStarted.AddUniqueDynamic(this, &UMounteaDialogueManager::RequestBroadcastContext);
		OnDialogueRowFinished.AddUniqueDynamic(this, &UMounteaDialogueManager::RequestBroadcastContext);
	}
}

void UMounteaDialogueManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMounteaDialogueManager, ManagerState);
}

bool UMounteaDialogueManager::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	return Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
}

MounteaDialogueManagerHelpers::FDialogueRowDataInfo MounteaDialogueManagerHelpers::GetDialogueRowDataInfo(const UMounteaDialogueContext* DialogueContext)
{
	FDialogueRowDataInfo Info;
	
	const int32 currentIndex = DialogueContext->GetActiveDialogueRowDataIndex();
	Info.IncreasedIndex = currentIndex + 1;

	const FDialogueRow dialogueRow = DialogueContext->GetActiveDialogueRow();
	Info.bIsActiveRowValid = UMounteaDialogueSystemBFC::IsDialogueRowValid(dialogueRow);

	const TArray<FDialogueRowData> rowDataArray = DialogueContext->GetActiveDialogueRow().DialogueRowData.Array();
	
	Info.bDialogueRowDataValid = rowDataArray.IsValidIndex(Info.IncreasedIndex);

	Info.NextRowExecutionMode = Info.bDialogueRowDataValid ? rowDataArray[Info.IncreasedIndex].RowExecutionBehaviour : ERowExecutionMode::EREM_Automatic;
	Info.ActiveRowExecutionMode = rowDataArray.IsValidIndex(currentIndex) ? rowDataArray[currentIndex].RowExecutionBehaviour : ERowExecutionMode::EREM_Automatic;

	return Info;
}

AActor* UMounteaDialogueManager::GetOwningActor_Implementation() const
{
	return GetOwner();
}

EDialogueManagerState UMounteaDialogueManager::GetManagerState_Implementation() const
{
	return ManagerState;
}

void UMounteaDialogueManager::SetManagerState(const EDialogueManagerState NewState)
{
	if (NewState == ManagerState)
	{
		LOG_INFO(TEXT("[Set Manager State] New State `%s` is same as current State. Update aborted."), *(UMounteaDialogueSystemBFC::GetEnumFriendlyName(NewState)))
		return;
	}
	
	if (!IsAuthority())
		SetManagerState_Server(NewState);
	else
	{
		ManagerState = NewState; // State can only be changed on server side!
		ProcessStateUpdated(); // Simulate OnRep for Listen server and cleanup on all servers and standalone	
	}

	OnDialogueManagerStateChanged.Broadcast(NewState);
}

void UMounteaDialogueManager::OnRep_ManagerState()
{
	OnDialogueManagerStateChanged.Broadcast(ManagerState);
	ProcessStateUpdated();
}

bool UMounteaDialogueManager::IsAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

void UMounteaDialogueManager::ProcessStateUpdated()
{
	if (IsAuthority() && !UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		return;
	}
	
	switch (ManagerState)
	{
		case EDialogueManagerState::EDMS_Disabled:
			Execute_CloseDialogue(this);
			break;
		case EDialogueManagerState::EDMS_Enabled:
			Execute_CloseDialogue(this);
			break;
		case EDialogueManagerState::EDMS_Active:
			Execute_StartDialogue(this);
			break;
	}
}

void UMounteaDialogueManager::RequestBroadcastContext(UMounteaDialogueContext* Context)
{
	if (IsAuthority())
	{
		RequestBroadcastContext_Multicast(FMounteaDialogueContextReplicatedStruct(Context));
	}
	else
		RequestBroadcastContext_Server(FMounteaDialogueContextReplicatedStruct(Context));
}

void UMounteaDialogueManager::RequestBroadcastContext_Server_Implementation(const FMounteaDialogueContextReplicatedStruct& Context)
{
	if (DialogueContext)
	{
		*DialogueContext += Context;
	}
	RequestBroadcastContext_Multicast(FMounteaDialogueContextReplicatedStruct(DialogueContext));
}

void UMounteaDialogueManager::RequestBroadcastContext_Multicast_Implementation(const FMounteaDialogueContextReplicatedStruct& Context)
{
	if(IsAuthority())
		return;

	auto localOwner = UMounteaDialogueSystemBFC::GetDialogueManagerLocalOwner(this) ;
	auto localRole = UMounteaDialogueSystemBFC::GetOwnerLocalRole(localOwner);
	if (localRole == ROLE_AutonomousProxy)
		return;

	*DialogueContext += Context;

	OnDialogueContextUpdated.Broadcast(DialogueContext);
}

void UMounteaDialogueManager::DialogueFailed(const FString& ErrorMessage)
{
	LOG_ERROR(TEXT("[Dialogue Failed] %s"), *ErrorMessage)
	SetManagerState(DefaultManagerState);
}

void UMounteaDialogueManager::SetManagerState_Server_Implementation(const EDialogueManagerState NewState)
{
	SetManagerState(NewState);
}

EDialogueManagerState UMounteaDialogueManager::GetDefaultManagerState_Implementation() const
{
	return DefaultManagerState;
}

void UMounteaDialogueManager::SetDefaultManagerState(const EDialogueManagerState NewState)
{
	if (NewState == DefaultManagerState)
	{
		LOG_WARNING(TEXT("[Set Default Manager State] New State `%s` is same as current State. Update aborted."), *(UMounteaDialogueSystemBFC::GetEnumFriendlyName(NewState)))
		return;
	}
	
	if (!IsAuthority())
		SetManagerState_Server(NewState);
	
	DefaultManagerState = NewState;
}

EDialogueManagerType UMounteaDialogueManager::GetDialogueManagerType() const
{
	auto ownerActor = GetOwner();
	if (!IsValid(ownerActor))
		return EDialogueManagerType::Default;

	auto ownerClass = ownerActor->GetClass();
	
	if (ownerClass->IsChildOf(APawn::StaticClass()) || 
		ownerClass->IsChildOf(APlayerState::StaticClass()) || 
		ownerClass->IsChildOf(APlayerController::StaticClass()))
	{
		return EDialogueManagerType::EDMT_PlayerDialogue;
	}

	return EDialogueManagerType::EDMT_EnvironmentDialogue;
}

void UMounteaDialogueManager::SetDefaultManagerState_Server_Implementation(const EDialogueManagerState NewState)
{
	SetDefaultManagerState(NewState);
}

bool UMounteaDialogueManager::CanStartDialogue_Implementation() const
{
	return ManagerState == EDialogueManagerState::EDMS_Enabled;
}

UMounteaDialogueContext* UMounteaDialogueManager::GetDialogueContext_Implementation() const
{
	return DialogueContext;
}

// TODO: Same logic as for ManagerState, replicate to all
void UMounteaDialogueManager::SetDialogueContext(UMounteaDialogueContext* NewContext)
{
	if (NewContext == DialogueContext) return;

	// Is the Context propagated from Client to Server?
	if (!IsAuthority())
		SetDialogueContext_Server(NewContext);

	DialogueContext = NewContext;

	OnDialogueContextUpdated.Broadcast(NewContext);
}

void UMounteaDialogueManager::SetDialogueContext_Server_Implementation(UMounteaDialogueContext* NewContext)
{
	SetDialogueContext(NewContext);

	// TODO: multicast context with information who is the current Manager, so it won't be updated locally?
}

void UMounteaDialogueManager::UpdateDialogueContext_Implementation(UMounteaDialogueContext* NewContext)
{
	if (NewContext == DialogueContext) return;
	
	if (!IsAuthority())
		UpdateDialogueContext_Server(NewContext);

	(*DialogueContext) += NewContext;

	OnDialogueContextUpdated.Broadcast(NewContext);
}

void UMounteaDialogueManager::UpdateDialogueContext_Server_Implementation(UMounteaDialogueContext* NewContext)
{
	Execute_UpdateDialogueContext(this, NewContext);

	// TODO: multicast context with information who is the current Manager, so it won't be updated locally?
}

// TODO: let's find a middle-point between Server authority and reducing double-runs at some point (what steps should be done on Server only?)
void UMounteaDialogueManager::RequestStartDialogue_Implementation(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	bool bSatisfied = true;
	TArray<FText> errorMessages;
	errorMessages.Add(FText::FromString("[Request Start Dialogue]"));

	if (!DialogueInitiator)
	{
		errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "MissingInitiator", "`DialogueInitiator` is not valid!"));
		bSatisfied = false;
	}

	if (!InitialParticipants.MainParticipant)
	{
		errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "MissingParticipant", "`MainParticipant` is not valid!"));
		bSatisfied = false;
	}

	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> dialogueParticipants;
	bool bMainParticipantFound = true;
	const TScriptInterface<IMounteaDialogueParticipantInterface> mainParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(InitialParticipants.MainParticipant, bMainParticipantFound);
	if (!bMainParticipantFound || !mainParticipant.GetObject())
	{
		errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "InvalidParticipant", "Main Participant doesn't have `Dialogue Participant` component or doesn't implement the `IMounteaDialogueParticipantInterface`!"));
		bSatisfied = false;
	}

	if (bMainParticipantFound)
	{
		dialogueParticipants.Add(mainParticipant);

 		if (!mainParticipant->Execute_CanStartDialogue(mainParticipant.GetObject())) // TODO: move Graph->CanStartDialogue to participant
		{
			errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "ParticipantCannotStart", "Main Participant Cannot Start Dialogue!"));
			bSatisfied = false;
		}
	}
	
	if (!Execute_CanStartDialogue(this))
	{
		errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "CannotStart", "Cannot Start Dialogue!"));
		bSatisfied = false;
	}

	LOG_INFO(TEXT("[Request Start Dialogue] Dialogue Type is %s"), *UMounteaDialogueSystemBFC::GetEnumFriendlyName(GetDialogueManagerType()))
	switch (GetDialogueManagerType()) {
		case EDialogueManagerType::EDMT_PlayerDialogue:
		{
			int searchDepth = 0;
			APawn* playerPawn = UMounteaDialogueSystemBFC::FindPlayerPawn(GetOwner(), searchDepth);
			if (!playerPawn)
			{
				errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "NoPawn", "Unable to find Player Pawn!"));
				bSatisfied = false;
			}
			else
			{
				bool bPlayerParticipantFound = true;
				const TScriptInterface<IMounteaDialogueParticipantInterface> playerParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(playerPawn, bPlayerParticipantFound);
				if (!bPlayerParticipantFound || !playerParticipant.GetObject())
				{
					errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "InvalidPawn", "Player Pawn doesn't have `Dialogue Participant` component or doesn't implement the `IMounteaDialogueParticipantInterface`!"));
					bSatisfied = false;
				}
				else
				{
					dialogueParticipants.Add(playerParticipant);
				}
			}
			break;
		}
		case EDialogueManagerType::EDMT_EnvironmentDialogue:
			{
				// TODO: We need player NetConnection to make replication work even for non-player dialogues (two NPCs etc.)
				// Those dialogues are triggered by PLAYER (so Player Pawn/Controller/State should be the Initiator)
			}
			break;
		case EDialogueManagerType::Default:
			{
				errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "WrongManager", "This Manager Type is not valid!"));
				bSatisfied = false;
			}
			break;
	}

	for (const auto& dialogueParticipant : dialogueParticipants)
	{
		const UObject* dialogueParticipantObject = dialogueParticipant.GetObject();
		if (!dialogueParticipantObject)
		{
			errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "EmptyParticipant", "Dialogue Participant is not Valid!"));
			bSatisfied = false;
		}
		else if (!dialogueParticipant->Execute_CanParticipateInDialogue(dialogueParticipantObject))
		{
			const FText message = FText::Format(NSLOCTEXT("RequestStartDialogue", "ParticipantCannotStart", "Dialogue Participant {0} cannot Participate in Dialogue!"), FText::FromString(dialogueParticipantObject->GetName()));
			errorMessages.Add(message);
			bSatisfied = false;
		}
	}

	if (bSatisfied)
	{
		errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "OK", "OK"));

		// If dialogue request returns false we simply CloseDialogue, which will destroy this Context
		DialogueContext = UMounteaDialogueSystemBFC::CreateDialogueContext(this, mainParticipant, dialogueParticipants);
	}
	
	const FText finalErrorMessage = FText::Join(FText::FromString("\n"), errorMessages);
	OnDialogueStartRequested.Broadcast(bSatisfied, finalErrorMessage.ToString());

	if (bSatisfied)
	{
		// Request Start on Server
		if (!IsAuthority())
		{
			RequestStartDialogue_Server(DialogueInitiator, InitialParticipants);
		}
	}
}

void UMounteaDialogueManager::RequestStartDialogue_Server_Implementation(AActor* DialogueInitiator,const FDialogueParticipants& InitialParticipants)
{
	Execute_RequestStartDialogue(this, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueManager::RequestCloseDialogue_Implementation()
{
	if (!IsAuthority())
	{
		SetManagerState(DefaultManagerState); // Let's close Dialogue by changing state
	}

	// Don't wait for Server, close dialogue anyways?
	Execute_CloseDialogue(this);
}

void UMounteaDialogueManager::StartParticipants() const
{
	for (const auto& dialogueParticipant : DialogueContext->DialogueParticipants)
	{
		if (!dialogueParticipant.GetObject() || !dialogueParticipant.GetInterface()) continue;

		TScriptInterface<IMounteaDialogueTickableObject> tickableObject = dialogueParticipant.GetObject();
		if (tickableObject.GetInterface() && tickableObject.GetObject())
		{
			// Register ticks for participants, no need to define Parent as Participants are the most paren ones
			tickableObject->Execute_RegisterTick(tickableObject.GetObject(), nullptr);
		}

		dialogueParticipant->Execute_SetParticipantState(dialogueParticipant.GetObject(), EDialogueParticipantState::EDPS_Active);
	}
}

void UMounteaDialogueManager::DialogueStartRequestReceived(const bool bResult, const FString& ResultMessage)
{
	if (bResult)
	{
		SetManagerState(EDialogueManagerState::EDMS_Active);
		StartParticipants();
	}
	else
	{
		SetManagerState(DefaultManagerState);
		OnDialogueFailed.Broadcast(ResultMessage);
	}
}

void UMounteaDialogueManager::StartDialogue_Implementation()
{
	StartParticipants();
	
	LOG_INFO(TEXT("[Start Dialogue] Starting Dialogue"))
	if (!IsAuthority())
	{
		RequestBroadcastContext(DialogueContext); // let Server know about our Context
	}
	
	FString resultMessage;
	if (!Execute_CreateDialogueUI(this, resultMessage))
	{
		LOG_WARNING(TEXT("[Create Dialogue UI] %s"), *(resultMessage))
	}
	
	Execute_PrepareNode(this);
}

void UMounteaDialogueManager::CloseDialogue_Implementation()
{
	// Unbind all UI Objects
	Execute_CloseDialogueUI(this);

	// Close Node Loop
	Execute_CleanupDialogue(this);

	SetDialogueContext(nullptr);
}

void UMounteaDialogueManager::CleanupDialogue_Implementation()
{
	if (!UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
		return;
	
	if (!IsAuthority())
		CleanupDialogue_Server();

	auto dialogueGraph = DialogueContext->ActiveNode ? DialogueContext->ActiveNode->Graph : nullptr;
	if (IsValid(dialogueGraph))
	{
		dialogueGraph->CleanupGraph();
	}
}

void UMounteaDialogueManager::CleanupDialogue_Server_Implementation()
{
	Execute_CleanupDialogue(this);
}

void UMounteaDialogueManager::PrepareNode_Implementation()
{
	if (!UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Prepare Node] Invalid Dialogue Context!"));
		return;
	}

	if (!IsValid(DialogueContext->ActiveNode))
	{
		OnDialogueFailed.Broadcast(TEXT("[Prepare Node] No Active Node!"));
		return;
	}
	
	DialogueContext->ActiveNode->PreProcessNode(this);
}

void UMounteaDialogueManager::NodePrepared_Implementation()
{
	if (!UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Prepared] Invalid Dialogue Context!"));
		return;
	}
	
	DialogueContext->AddTraversedNode(DialogueContext->ActiveNode);
	
	Execute_ProcessNode(this);
}

void UMounteaDialogueManager::ProcessNode_Implementation()
{
	if (DialogueContext && DialogueContext->ActiveNode)
	{
		DialogueContext->ActiveNode->ProcessNode(this);

		OnDialogueNodeStarted.Broadcast(DialogueContext);

		Execute_ProcessDialogueRow(this);
	}
	else
		OnDialogueFailed.Broadcast(TEXT("[Process Node] Invalid Context or Active Node!"));
}

void UMounteaDialogueManager::NodeProcessed_Implementation()
{
	if (!UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Processed] Invalid Dialogue Context!"));
		return;
	}

	if (!IsValid(DialogueContext->ActiveNode) )
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Processed] Invalid Active Node!"));
		return;
	}

	DialogueContext->ActiveNode->Execute_UnregisterTick(DialogueContext->ActiveNode, DialogueContext->ActiveNode->Graph);
	
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);

	TArray<UMounteaDialogueGraphNode*> allowedChildrenNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(DialogueContext->ActiveNode);
	UMounteaDialogueSystemBFC::SortNodes(allowedChildrenNodes);
	
	// If there are only Complete Nodes left or no DialogueNodes left, just shut it down
	if (allowedChildrenNodes.Num() == 0)
	{
		Execute_RequestCloseDialogue(this);
		return;
	}
	
	UMounteaDialogueGraphNode** foundNodePtr = allowedChildrenNodes.FindByPredicate([](const UMounteaDialogueGraphNode* node) {
		return node->DoesAutoStart();
	});

	UMounteaDialogueGraphNode* newActiveNode = foundNodePtr ? *foundNodePtr : nullptr;

	if (newActiveNode != nullptr)
	{
		OnDialogueClosed.Broadcast(DialogueContext);

		auto newActiveDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(newActiveNode);
		auto allowedChildNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(newActiveNode);
		UMounteaDialogueSystemBFC::SortNodes(allowedChildNodes);

		FDataTableRowHandle newDialogueTableHandle = FDataTableRowHandle();
		newDialogueTableHandle.DataTable = newActiveDialogueNode->GetDataTable();
		newDialogueTableHandle.RowName = newActiveDialogueNode->GetRowName();
		
		DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, newActiveNode, allowedChildNodes);
		DialogueContext->UpdateActiveDialogueTable(newActiveDialogueNode ? newDialogueTableHandle : FDataTableRowHandle());
		
		OnDialogueNodeSelected.Broadcast(DialogueContext);

		Execute_PrepareNode(this);
	}
	else
	{
		FString resultMessage;
		if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::AddDialogueOptions))
		{
			LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
		}
	}
}

void UMounteaDialogueManager::SelectNode_Implementation(const FGuid& NodeGuid)
{
	if (!UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Selected] Invalid Context!"));
		return;
	}
	
	const TArray<UMounteaDialogueGraphNode*>& childrenNodes = DialogueContext->GetChildrenNodes();
	UMounteaDialogueGraphNode* selectedNode = *childrenNodes.FindByPredicate(
		[NodeGuid](const UMounteaDialogueGraphNode* Node)
		{
			return Node && Node->GetNodeGUID() == NodeGuid;
		});
	
	if (!IsValid(selectedNode))
	{
		const FString errorMessage = FText::Format(FText::FromString("[Node Selected] Node with GUID {0} not found"), FText::FromString(NodeGuid.ToString())).ToString();
		LOG_ERROR(TEXT("%s"), *errorMessage);
		OnDialogueFailed.Broadcast(errorMessage);
		return;
	}

	// Straight up set dialogue row from Node and index to 0
	auto allowedChildNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(selectedNode);
	UMounteaDialogueSystemBFC::SortNodes(allowedChildNodes);

	if (const auto selectedDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(selectedNode))
	{
		FDataTableRowHandle newDialogueTableHandle = FDataTableRowHandle();
		newDialogueTableHandle.DataTable = selectedDialogueNode->GetDataTable();
		newDialogueTableHandle.RowName = selectedDialogueNode->GetRowName();
		
		DialogueContext->UpdateActiveDialogueTable(selectedNode ? newDialogueTableHandle : FDataTableRowHandle());
	}

	// TODO: replace with UPDATE, make UPDATE take struct input, too
	DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, selectedNode, allowedChildNodes);
	DialogueContext->UpdateActiveDialogueRow(UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode));
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);

	FString resultMessage;
	if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::RemoveDialogueOptions))
	{
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
	}
	
	OnDialogueNodeSelected.Broadcast(DialogueContext);
	
	Execute_PrepareNode(this);
}

void UMounteaDialogueManager::ProcessDialogueRow_Implementation()
{
	if (!IsValid(GetWorld()))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] World is not Valid!"));
		return;
	}
	
	if (!UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Invalid Dialogue Context!"));
		return;
	}
	
	// non-dialogue nodes are handled in their own ways
	if (!DialogueContext->ActiveNode->IsA(UMounteaDialogueGraphNode_DialogueNodeBase::StaticClass()))
		return;
	
	FString resultMessage;
	if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::ShowDialogueRow))
	{
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
	}

	if (DialogueContext->GetActiveDialogueRow().DialogueRowData.Array().IsValidIndex(DialogueContext->GetActiveDialogueRowDataIndex()) == false)
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Trying to Access Invalid Dialogue Row data!"));
		return;
	}

	const int32 activeIndex = DialogueContext->GetActiveDialogueRowDataIndex();
	const auto Row = DialogueContext->GetActiveDialogueRow();
	bool bValidRowData = Row.DialogueRowData.Array().IsValidIndex(activeIndex);

	if (!bValidRowData)
	{
		LOG_WARNING(TEXT("[Process Dialogue Row] Invalid Dialogue Row Data at index %d! Skipping Row. Next Row will be processed instead."), activeIndex)
		Execute_DialogueRowProcessed(this);
		return;
	}
	
	const FDialogueRowData RowData = Row.DialogueRowData.Array()[activeIndex];
	bValidRowData = UMounteaDialogueSystemBFC::IsDialogueRowDataValid(RowData);

	if (!bValidRowData)
	{
		LOG_WARNING(TEXT("[Process Dialogue Row] Invalid Dialogue Row Data! Skipping Row. Next Row will be processed instead."))
		Execute_DialogueRowProcessed(this);
		return;
	}
	
	DialogueContext->ActiveDialogueParticipant->Execute_PlayParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), RowData.RowSound);

	if (bValidRowData)
	{
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UMounteaDialogueManager::DialogueRowProcessed_Implementation);
		
		GetWorld()->GetTimerManager().SetTimer
		(
			TimerHandle_RowTimer,
			Delegate,
			UMounteaDialogueSystemBFC::GetRowDuration(RowData),
			false
		);
	}
}

void UMounteaDialogueManager::DialogueRowProcessed_Implementation()
{
	FString resultMessage;
	if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::HideDialogueRow))
	{
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
	}
	
	if (!IsValid(GetWorld()))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] World is not Valid!"));
		return;
	}
	
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const auto processInfo = MounteaDialogueManagerHelpers::GetDialogueRowDataInfo(DialogueContext);

	OnDialogueRowFinished.Broadcast(DialogueContext);

	// This row has finished and next will execute only using 'TriggerNextDialogueRow'.
	if (processInfo.ActiveRowExecutionMode == ERowExecutionMode::EREM_AwaitInput)
	{
		return;
	}
	
	if (processInfo.bIsActiveRowValid && processInfo.bDialogueRowDataValid)
	{
		switch (processInfo.NextRowExecutionMode)
		{
		case ERowExecutionMode::EREM_Automatic:
			{
				DialogueContext->UpdateActiveDialogueRowDataIndex(processInfo.IncreasedIndex);
				OnDialogueContextUpdated.Broadcast(DialogueContext);
				Execute_ProcessDialogueRow(this); // Continue in the loop, just with another row
			}
			break;
		case ERowExecutionMode::EREM_AwaitInput:
			break;
		case ERowExecutionMode::EREM_Stopping:
			OnDialogueNodeFinished.Broadcast(DialogueContext);
			break;
		case ERowExecutionMode::Default:
			break;
		}
	}
	else
	{
		Execute_NodeProcessed(this); // Exit Row loop, this is the last one, let's finish whole Node
	}
}

void UMounteaDialogueManager::SkipDialogueRow_Implementation()
{
	if (!IsValid(DialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceSkipRequestEvent] Invalid Dialogue Context!"));
		return;
	}

	DialogueContext->ActiveDialogueParticipant->Execute_SkipParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), nullptr);

	Execute_DialogueRowProcessed(this);
}

void UMounteaDialogueManager::UpdateWorldDialogueUI_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command)
{
	if (!IsAuthority())
	{
		for (const auto& dialogueObject : DialogueObjects)
		{
			if (dialogueObject)
			{
				IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(dialogueObject, DialogueManager, Command);
			}
		}
		
		UpdateWorldDialogueUI_Server(DialogueManager, Command);
	}
	else
	{
		UpdateWorldDialogueUI_Multicast(DialogueManager, Command);
	}
}

void UMounteaDialogueManager::UpdateWorldDialogueUI_Server_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command)
{
	UpdateWorldDialogueUI_Multicast(DialogueManager, Command);
}

void UMounteaDialogueManager::UpdateWorldDialogueUI_Multicast_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command)
{
	if (!UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
		return;

	if(IsAuthority())
		return;

	auto localOwner = UMounteaDialogueSystemBFC::GetDialogueManagerLocalOwner(this) ;
	auto localRole = UMounteaDialogueSystemBFC::GetOwnerLocalRole(localOwner);
	if (localRole == ROLE_AutonomousProxy)
		return;
	
	for (const auto& dialogueObject : DialogueObjects)
	{
		if (dialogueObject)
		{
			IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(dialogueObject, DialogueManager, Command);
		}
	}
}

bool UMounteaDialogueManager::AddDialogueUIObject_Implementation(UObject* NewDialogueObject)
{
	if (NewDialogueObject == nullptr)
	{
		LOG_WARNING(TEXT("[AddDialogueUIObject] Input parameter is null!"));
		return false;
	}

	if (!NewDialogueObject->Implements<UMounteaDialogueWBPInterface>())
	{
		LOG_WARNING(TEXT("[AddDialogueUIObject] Input parameter does not implement 'IMounteaDialogueWBPInterface'!"));
		return false;
	}

	if (DialogueObjects.Contains(NewDialogueObject))
	{
		LOG_WARNING(TEXT("[AddDialogueUIObject] Input parameter already stored!"));
		return false;
	}

	DialogueObjects.Add(NewDialogueObject);
	
	return true;
}

bool UMounteaDialogueManager::AddDialogueUIObjects_Implementation(const TArray<UObject*>& NewDialogueObjects)
{
	if (NewDialogueObjects.Num() == 0)
	{
		LOG_WARNING(TEXT("[AddDialogueUIObjects] Input array is empty!"));
		return false;
	}

	bool bAllAdded = true;
	for (UObject* Object : NewDialogueObjects)
	{
		if (!AddDialogueUIObject_Implementation(Object))
		{
			bAllAdded = false;
		}
	}

	return bAllAdded;
}

bool UMounteaDialogueManager::RemoveDialogueUIObject_Implementation(UObject* DialogueObjectToRemove)
{
	if (DialogueObjectToRemove == nullptr)
	{
		LOG_WARNING(TEXT("[RemoveDialogueUIObject] Input parameter is null!"));
		return false;
	}

	if (!DialogueObjects.Contains(DialogueObjectToRemove))
	{
		LOG_WARNING(TEXT("[RemoveDialogueUIObject] Input parameter not found in stored objects!"));
		return false;
	}

	DialogueObjects.Remove(DialogueObjectToRemove);
	return true;
}

bool UMounteaDialogueManager::RemoveDialogueUIObjects_Implementation(const TArray<UObject*>& DialogueObjectsToRemove)
{
	if (DialogueObjectsToRemove.Num() == 0)
	{
		LOG_WARNING(TEXT("[RemoveDialogueUIObjects] Input array is empty!"));
		return false;
	}

	bool bAllRemoved = true;
	for (UObject* Object : DialogueObjectsToRemove)
	{
		if (!RemoveDialogueUIObject_Implementation(Object))
		{
			bAllRemoved = false;
		}
	}

	return bAllRemoved;
}

void UMounteaDialogueManager::SetDialogueUIObjects_Implementation(const TArray<UObject*>& NewDialogueObjects)
{
	DialogueObjects.Empty();

	for (UObject* Object : NewDialogueObjects)
	{
		AddDialogueUIObject_Implementation(Object);
	}
}

void UMounteaDialogueManager::ResetDialogueUIObjects_Implementation()
{
	DialogueObjects.Empty();
}

bool UMounteaDialogueManager::CreateDialogueUI_Implementation(FString& Message)
{
	if (GetDialogueWidgetClass() == nullptr)
	{
		Message = TEXT("Invalid Widget Class! Setup Widget class at least in Project settings!");
		return false;
	}
	if (!GetWorld())
	{
		Message = TEXT("Invalid World!");
		return false;
	}
	int seachDepth = 0;
	APlayerController* playerController = UMounteaDialogueSystemBFC::FindPlayerController(GetOwner(), seachDepth);
	if (playerController == nullptr)
	{
		Message = TEXT("Invalid Player Controller!");
		return false;
	}
	if (!playerController->IsLocalController())
	{
		Message = TEXT("UI can be shown only to Local Players!");
		return false;
	}

	DialogueWidget = CreateWidget<UUserWidget>(playerController,  GetDialogueWidgetClass());

	if (DialogueWidget == nullptr)
	{
		Message = TEXT("Cannot spawn Dialogue Widget!");
		return false;
	}

	if (DialogueWidget->Implements<UMounteaDialogueWBPInterface>() == false)
	{
		Message = TEXT("Does not implement Diaogue Widget Interface!");
		return false;
	}
	
	OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidget);
	
	return Execute_UpdateDialogueUI(this, Message, MounteaDialogueWidgetCommands::CreateDialogueWidget);
}

bool UMounteaDialogueManager::UpdateDialogueUI_Implementation(FString& Message, const FString& Command)
{
	LOG_INFO(TEXT("[Update Dialogue UI] Command: %s"), *Command)
	Execute_UpdateWorldDialogueUI(this, this, Command);

	if (DialogueWidget)
	{
		IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidget, this, Command);
		return true;
	}
	return false;
}

bool UMounteaDialogueManager::CloseDialogueUI_Implementation()
{
	FString dialogueMessage;
	const bool bSatisfied = Execute_UpdateDialogueUI(this, dialogueMessage, MounteaDialogueWidgetCommands::CloseDialogueWidget);

	if (IsValid((DialogueWidget)))
	{
		DialogueWidget->MarkAsGarbage();
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}
	
	return bSatisfied;
}

void UMounteaDialogueManager::ExecuteWidgetCommand_Implementation(const FString& Command)
{
	FString resultMessage;
	if (!Execute_UpdateDialogueUI(this, resultMessage, Command))
	{
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
	}
}

TSubclassOf<UUserWidget> UMounteaDialogueManager::GetDialogueWidgetClass() const
{
	return DialogueWidgetClass != nullptr ? DialogueWidgetClass : UMounteaDialogueSystemBFC::GetDefaultDialogueWidget();
}

void UMounteaDialogueManager::SetDialogueWidgetClass(const TSubclassOf<UUserWidget> NewWidgetClass)
{
	if (DialogueWidgetClass != NewWidgetClass)
	{
		DialogueWidgetClass = NewWidgetClass;
		OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidget);
	}
}

void UMounteaDialogueManager::SetDialogueWidget_Implementation(UUserWidget* NewDialogueWidget)
{
	DialogueWidget = NewDialogueWidget;

	OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidget);
}

UUserWidget* UMounteaDialogueManager::GetDialogueWidget_Implementation() const
{
	return DialogueWidget;
}

int32 UMounteaDialogueManager::GetDialogueWidgetZOrder_Implementation() const
{
	return DialogueWidgetZOrder;
}

void UMounteaDialogueManager::SetDialogueWidgetZOrder_Implementation(const int32 NewZOrder)
{
	if (NewZOrder != DialogueWidgetZOrder)
	{
		DialogueWidgetZOrder = NewZOrder;
		auto dialogueWidget = Execute_GetDialogueWidget(this);
		if (dialogueWidget)
		{
			ULocalPlayer* localPlayer = dialogueWidget->GetOwningLocalPlayer();
			if (localPlayer)
			{
				if (UGameViewportSubsystem* viewportSubsystem = UGameViewportSubsystem::Get(GetWorld()))
				{
					FGameViewportWidgetSlot widgetSlot = viewportSubsystem->GetWidgetSlot(dialogueWidget);
					widgetSlot.ZOrder = NewZOrder;

					viewportSubsystem->AddWidgetForPlayer(dialogueWidget, localPlayer, widgetSlot);
				}
			}
		}
	}
}








