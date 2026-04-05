// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "Components/MounteaDialogueManager.h"

#include "TimerManager.h"
#include "Graph/MounteaDialogueGraph.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueContextStatics.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueManagerStatics.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/MounteaDialogueSession.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Subsystem/MounteaDialogueWorldSubsystem.h"

static constexpr uint8 DialogueClientViewMode_None = 0;
static constexpr uint8 DialogueClientViewMode_Row = 1;
static constexpr uint8 DialogueClientViewMode_Options = 2;
static constexpr uint8 DialogueClientViewMode_Neutral = 3;
static constexpr uint8 DialogueClientViewMode_Closed = 4;

UMounteaDialogueManager::UMounteaDialogueManager()
	: DefaultManagerState(EDialogueManagerState::EDMS_Enabled)
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
	
	if (GetWorld())
	{
		auto* dialogueSubsystem = GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>();
		dialogueSubsystem->RegisterManager(this);
	}
	else
		LOG_ERROR(TEXT("[BeginPlay] No World Found!"))

	// Force replicate Owner to avoid setup issues with less experienced users
	const auto owningActor = GetOwner();
	if (IsValid(owningActor) && !owningActor->GetIsReplicated() && GetIsReplicated())
		GetOwner()->SetReplicates(true);

	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
		OnDialogueStartRequestedResult.AddUniqueDynamic(this, &UMounteaDialogueManager::DialogueStartRequestReceived);

	OnDialogueFailed.AddUniqueDynamic(this, &UMounteaDialogueManager::DialogueFailed);

}

void UMounteaDialogueManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	OnDialogueStartRequestedResult.Clear();
	OnDialogueFailed.Clear();
	OnDialogueStarted.Clear();
	OnDialogueClosed.Clear();
	OnDialogueNodeSelected.Clear();
	OnDialogueNodeFinished.Clear();
	OnDialogueRowStarted.Clear();
	OnDialogueRowFinished.Clear();

	if (GetWorld())
	{
		auto* dialogueSubsystem = GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>();
		dialogueSubsystem->UnregisterManager(this);
	}
	else
		LOG_ERROR(TEXT("[EndPlay] No World Found!"))

}

AActor* UMounteaDialogueManager::GetOwningActor_Implementation() const
{
	return GetOwner();
}

void UMounteaDialogueManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMounteaDialogueManager, ManagerState);
}



MounteaDialogueManagerHelpers::FDialogueRowDataInfo MounteaDialogueManagerHelpers::GetDialogueRowDataInfo(const UMounteaDialogueContext* DialogueContext)
{
	FDialogueRowDataInfo Info;
	
	const int32 currentIndex = DialogueContext->GetActiveDialogueRowDataIndex();
	Info.IncreasedIndex = currentIndex + 1;

	const FDialogueRow dialogueRow = DialogueContext->GetActiveDialogueRow();
	Info.bIsActiveRowValid = UMounteaDialogueTraversalStatics::IsDialogueRowValid(dialogueRow);

	const TArray<FDialogueRowData> rowDataArray = DialogueContext->GetActiveDialogueRow().RowData;
	
	Info.bDialogueRowDataValid = rowDataArray.IsValidIndex(Info.IncreasedIndex);

	Info.NextRowExecutionMode = Info.bDialogueRowDataValid ? rowDataArray[Info.IncreasedIndex].RowExecutionBehaviour : ERowExecutionMode::EREM_Automatic;
	Info.ActiveRowExecutionMode = rowDataArray.IsValidIndex(currentIndex) ? rowDataArray[currentIndex].RowExecutionBehaviour : ERowExecutionMode::EREM_Automatic;

	return Info;
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

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		SetManagerState_Server(NewState);
		return;
	}

	ManagerState = NewState;
	ProcessStateUpdated();
}

void UMounteaDialogueManager::OnRep_ManagerState()
{
	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		if (subsystem)
		{
			if (UMounteaDialogueSession* session = subsystem->GetGameStateSession())
				session->TryDispatchPendingClientPayload();
		}
	}

	OnDialogueManagerStateChanged.Broadcast(ManagerState);
	ProcessStateUpdated();
}

void UMounteaDialogueManager::ProcessStateUpdated()
{
	// Cosmetics are skipped on dedicated server, but state transitions (StartDialogue, CloseDialogue)
	// must always fire so server-side traversal runs regardless of rendering mode.
	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueManagerStateChanged.Broadcast(ManagerState);

	switch (ManagerState)
	{
		case EDialogueManagerState::EDMS_Disabled:
		case EDialogueManagerState::EDMS_Enabled:
			Execute_CloseDialogue(this);
			break;
		case EDialogueManagerState::EDMS_Active:
			Execute_StartDialogue(this);
			break;
	}
}


void UMounteaDialogueManager::OnContextPayloadUpdated(const FMounteaDialogueContextPayload& Payload)
{
	if (LastClientSyncSessionGUID != Payload.SessionGUID)
		ResetClientSyncCaches(Payload.SessionGUID);

	LastReceivedPayloadVersion = Payload.ContextVersion;

	if (!IsValid(DialogueContext))
		DialogueContext = NewObject<UMounteaDialogueContext>(this);

	// Rebuild participant references directly from payload
	DialogueContext->SessionGUID = Payload.SessionGUID;
	DialogueContext->ActiveDialogueParticipant = Payload.ActiveDialogueParticipant;
	DialogueContext->DialogueParticipants = Payload.DialogueParticipants;

	UMounteaDialogueGraph* activeGraph = UMounteaDialogueManagerStatics::ResolveGraphByGuid(Payload.DialogueParticipants, Payload.ActiveGraphGUID);
	if (!activeGraph)
	{
		const TScriptInterface<IMounteaDialogueParticipantInterface> graphSource =
			UMounteaDialogueParticipantStatics::GetGraphOwnerParticipant(Payload.DialogueParticipants);
		if (graphSource.GetObject() && graphSource.GetInterface())
			activeGraph = graphSource->Execute_GetDialogueGraph(graphSource.GetObject());
	}

	if (activeGraph)
	{
		DialogueContext->ActiveNode = UMounteaDialogueContextStatics::FindNodeByGUID(activeGraph, Payload.ActiveNodeGUID);
		DialogueContext->PreviousActiveNode = Payload.PreviousNodeGUID;
		DialogueContext->AllowedChildNodes = UMounteaDialogueContextStatics::FindNodesByGUID(activeGraph, Payload.AllowedChildNodeGUIDs);
	}
	else
	{
		DialogueContext->ActiveNode = nullptr;
		DialogueContext->PreviousActiveNode = FGuid();
		DialogueContext->AllowedChildNodes.Empty();
	}

	DialogueContext->ActiveDialogueRow = Payload.ActiveDialogueRow;
	DialogueContext->ActiveDialogueRowDataIndex = Payload.ActiveDialogueRowDataIndex;

	OnDialogueContextUpdated.Broadcast(DialogueContext);

	if (ManagerState == EDialogueManagerState::EDMS_Active)
		StartParticipants();

	NotifyParticipants(Payload.DialogueParticipants);

	ReconcileClientUIFromPayload(Payload);
}

void UMounteaDialogueManager::NotifyParticipants(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& Participants)
{
	for (const auto& Participant : Participants)
	{
		if (auto participantObject = Participant.GetObject())
		{
			TScriptInterface<IMounteaDialogueParticipantInterface> dialogueParticipant = TScriptInterface<IMounteaDialogueParticipantInterface>(participantObject);
			dialogueParticipant->GetDialogueUpdatedEventHandle().Broadcast(this);
		}
	}
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
	
	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
		SetDefaultManagerState_Server(NewState);

	DefaultManagerState = NewState;
}

EDialogueManagerType UMounteaDialogueManager::GetDialogueManagerType() const
{
	return DialogueManagerType;
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

void UMounteaDialogueManager::SetDialogueContext(UMounteaDialogueContext* NewContext)
{
	if (NewContext == DialogueContext) 
		return;

	DialogueContext = NewContext;
	if (!IsValid(DialogueContext))
		bParticipantsStarted = false;

	OnDialogueContextUpdated.Broadcast(NewContext);
}

void UMounteaDialogueManager::UpdateDialogueContext_Implementation(UMounteaDialogueContext* NewContext)
{
	SetDialogueContext(NewContext);
}

void UMounteaDialogueManager::RequestStartDialogue_Implementation(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	if (!Execute_CanStartDialogue(this))
	{
		OnDialogueFailed.Broadcast(TEXT("[RequestStartDialogue] Cannot start dialogue — manager is not enabled!"));
		return;
	}

	if (!InitialParticipants.MainParticipant)
	{
		OnDialogueFailed.Broadcast(TEXT("[RequestStartDialogue] MainParticipant is not valid!"));
		return;
	}

	DialogueInstigator = DialogueInitiator;

	FDialogueStartRequest request;
	request.MainParticipantActorRef = InitialParticipants.MainParticipant;
	request.MainParticipantActor = TSoftObjectPtr<AActor>(InitialParticipants.MainParticipant);
	for (const auto& other : InitialParticipants.OtherParticipants)
	{
		if (AActor* otherActor = Cast<AActor>(other))
		{
			request.OtherParticipantActorRefs.Add(otherActor);
			request.OtherParticipantActors.Add(TSoftObjectPtr<AActor>(otherActor));
		}
	}

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		RequestStartDialogue_Server(request);
		return;
	}

	// Server path: delegate to subsystem
	if (auto* subsystem = GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>())
		subsystem->HandleStartRequest(this, request);
}

void UMounteaDialogueManager::RequestStartDialogue_Server_Implementation(const FDialogueStartRequest& Request)
{
	if (auto* subsystem = GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>())
		subsystem->HandleStartRequest(this, Request);
}

void UMounteaDialogueManager::RequestSelectNode_Server_Implementation(FGuid SessionGUID, FGuid NodeGUID)
{
	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Select Node] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleSelectNode(this, SessionGUID, NodeGUID);
}

void UMounteaDialogueManager::RequestSkipRow_Server_Implementation(FGuid SessionGUID)
{
	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Skip Dialogue Row] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleSkipDialogueRow(this, SessionGUID);
}

void UMounteaDialogueManager::RequestNodeProcessed_Server_Implementation(FGuid SessionGUID)
{
	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Processed] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleNodeProcessed(this, SessionGUID);
}

void UMounteaDialogueManager::RequestDialogueRowProcessed_Server_Implementation(FGuid SessionGUID, const bool bForceFinish)
{
	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleDialogueRowProcessed(this, SessionGUID, bForceFinish);
}

void UMounteaDialogueManager::RequestCloseDialogue_Server_Implementation(FGuid SessionGUID)
{
	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Close Dialogue] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleCloseDialogue(this, SessionGUID);
}

void UMounteaDialogueManager::RequestCloseDialogue_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
		if (!sessionGuid.IsValid())
		{
			UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
			if (subsystem)
			{
				if (UMounteaDialogueSession* session = subsystem->GetGameStateSession())
					sessionGuid = session->GetContextPayload().SessionGUID;
			}
		}

		RequestCloseDialogue_Server(sessionGuid);
		return;
	}

	SetManagerState(DefaultManagerState);
}

void UMounteaDialogueManager::StartParticipants()
{
	if (bParticipantsStarted)
		return;

	if (!IsValid(DialogueContext))
		return;
	
	for (const auto& dialogueParticipant : DialogueContext->DialogueParticipants)
	{
		if (!dialogueParticipant.GetObject() || !dialogueParticipant.GetInterface()) 
			continue;

		TScriptInterface<IMounteaDialogueTickableObject> tickableObject = dialogueParticipant.GetObject();
		if (tickableObject.GetInterface() && tickableObject.GetObject())
		{
			// Register ticks for participants, no need to define Parent as Participants are the most paren ones
			tickableObject->Execute_RegisterTick(tickableObject.GetObject(), nullptr);
		}

		dialogueParticipant->Execute_SetParticipantState(dialogueParticipant.GetObject(), EDialogueParticipantState::EDPS_Active);
		dialogueParticipant->Execute_InitializeParticipant(dialogueParticipant.GetObject(), this);
		dialogueParticipant->GetOnDialogueStartedEventHandle().Broadcast();
	}

	bParticipantsStarted = true;
}

void UMounteaDialogueManager::StopParticipants()
{
	if (!bParticipantsStarted)
		return;

	if (!IsValid(DialogueContext))
	{
		bParticipantsStarted = false;
		return;
	}

	for (const auto& dialogueParticipant : DialogueContext->DialogueParticipants)
	{
		auto participantObject = dialogueParticipant.GetObject();
		if (!IsValid(participantObject) || !dialogueParticipant.GetInterface()) 
			continue;
		
		TScriptInterface<IMounteaDialogueTickableObject> tickableObject = dialogueParticipant.GetObject();
		if (tickableObject.GetInterface() && tickableObject.GetObject())
		{
			// Register ticks for participants, no need to define Parent as Participants are the most paren ones
			tickableObject->Execute_UnregisterTick(tickableObject.GetObject(), nullptr);
		}

		dialogueParticipant->Execute_SetParticipantState(participantObject, dialogueParticipant->Execute_GetDefaultParticipantState(participantObject));
		dialogueParticipant->GetOnDialogueEndedEventHandle().Broadcast();
	}

	bParticipantsStarted = false;
}

void UMounteaDialogueManager::DialogueStartRequestReceived(const bool bResult, const FString& ResultMessage)
{
	if (bResult)
	{
		SetManagerState(EDialogueManagerState::EDMS_Active);
	}
	else
	{
		if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
		{
			auto* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
			if (subsystem)
				subsystem->ReleaseDialogueLock(this, IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid());
		}

		SetManagerState(DefaultManagerState);
		OnDialogueFailed.Broadcast(ResultMessage);
	}
}

void UMounteaDialogueManager::StartDialogue_Implementation()
{
	StartParticipants();

	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueStarted.Broadcast(DialogueContext);

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		if (subsystem)
		{
			if (UMounteaDialogueSession* session = subsystem->GetGameStateSession())
				session->TryDispatchPendingClientPayload();
		}
		return;
	}

	// Server: deliver UI create signal to owning client.
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		const UMounteaDialogueSession* session = subsystem ? subsystem->GetGameStateSession() : nullptr;
		const FMounteaDialogueContextPayload& payload = session ? session->GetContextPayload() : FMounteaDialogueContextPayload();
		const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
		Client_DispatchUISignal(FMounteaDialogueUISignal{
			MounteaDialogueWidgetCommands::CreateDialogueWidget,
			sessionGuid,
			payload.ContextVersion,
			true
		});
	}

	Execute_PrepareNode(this);
}

void UMounteaDialogueManager::CloseDialogue_Implementation()
{
	const FGuid closingSessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		auto* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		if (subsystem)
		{
			if (UMounteaDialogueSession* session = subsystem->GetGameStateSession())
			{
				session->FinalizeSession();
			}

			subsystem->ReleaseDialogueLock(this, closingSessionGuid);
		}

		if (IsValid(DialogueContext))
			DialogueContext->TraversedPath.Empty();
	}

	StopParticipants();

	// Deliver UI close signal to owning client then flush the signal queue.
	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		const UMounteaDialogueSession* session = subsystem ? subsystem->GetGameStateSession() : nullptr;
		const FMounteaDialogueContextPayload& payload = session ? session->GetContextPayload() : FMounteaDialogueContextPayload();
		Client_DispatchUISignal(FMounteaDialogueUISignal{
			MounteaDialogueWidgetCommands::CloseDialogueWidget,
			closingSessionGuid,
			payload.ContextVersion,
			true
		});
		Client_ClearUISignals(closingSessionGuid);
	}

	Execute_CleanupDialogue(this);

	SetDialogueContext(nullptr);

	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueClosed.Broadcast(DialogueContext);

	DialogueInstigator = nullptr;
	ResetClientSyncCaches(FGuid());
}

void UMounteaDialogueManager::CleanupDialogue_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);
	
	if (!UMounteaDialogueContextStatics::IsContextValid(DialogueContext))
		return;
	
	auto dialogueGraph = DialogueContext->ActiveNode ? DialogueContext->ActiveNode->Graph : nullptr;
	if (IsValid(dialogueGraph))
		dialogueGraph->ShutdownGraph();
}

void UMounteaDialogueManager::CleanupDialogue_Server_Implementation()
{
	Execute_CleanupDialogue(this);
}

void UMounteaDialogueManager::Client_DispatchUISignal_Implementation(const FMounteaDialogueUISignal& Signal)
{
	OnDialogueUISignalRequested.Broadcast(Signal);
}

void UMounteaDialogueManager::Client_ClearUISignals_Implementation(const FGuid& SessionGUID)
{
	FMounteaDialogueUISignal sentinel;
	sentinel.SessionGUID = SessionGUID;
	sentinel.RequiredContextVersion = INT32_MAX;
	OnDialogueUISignalRequested.Broadcast(sentinel);
}

void UMounteaDialogueManager::PrepareNode_Implementation()
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		if (!subsystem)
			return;

		UMounteaDialogueSession* session = subsystem->GetGameStateSession();
		if (!session)
		{
			OnDialogueFailed.Broadcast(TEXT("[Prepare Node] Missing Dialogue Session on GameState."));
			return;
		}

		session->HandlePrepareNode(this, sessionGuid);
		return;
	}
}

void UMounteaDialogueManager::NodePrepared_Implementation()
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		if (!subsystem)
			return;

		UMounteaDialogueSession* session = subsystem->GetGameStateSession();
		if (!session)
		{
			OnDialogueFailed.Broadcast(TEXT("[Node Prepared] Missing Dialogue Session on GameState."));
			return;
		}

		session->HandleNodePrepared(this, sessionGuid);
		return;
	}
}

void UMounteaDialogueManager::ProcessNode_Implementation()
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		if (!subsystem)
			return;

		UMounteaDialogueSession* session = subsystem->GetGameStateSession();
		if (!session)
		{
			OnDialogueFailed.Broadcast(TEXT("[Process Node] Missing Dialogue Session on GameState."));
			return;
		}

		session->HandleProcessNode(this, sessionGuid);
		return;
	}
}

void UMounteaDialogueManager::NodeProcessed_Implementation()
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		RequestNodeProcessed_Server(sessionGuid);
		return;
	}

	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Processed] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleNodeProcessed(this, sessionGuid);
}

void UMounteaDialogueManager::SelectNode_Implementation(const FGuid& NodeGuid)
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		RequestSelectNode_Server(sessionGuid, NodeGuid);
		return;
	}

	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Select Node] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleSelectNode(this, sessionGuid, NodeGuid);
}

void UMounteaDialogueManager::ProcessDialogueRow_Implementation()
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		if (!subsystem)
			return;

		UMounteaDialogueSession* session = subsystem->GetGameStateSession();
		if (!session)
		{
			OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Missing Dialogue Session on GameState."));
			return;
		}

		session->HandleProcessDialogueRow(this, sessionGuid);
		return;
	}
}

void UMounteaDialogueManager::DialogueRowProcessed_Implementation(const bool bForceFinish)
{
	// To avoid race conditions simply return if active
	if (ManagerState != EDialogueManagerState::EDMS_Active)
		return;

	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		RequestDialogueRowProcessed_Server(sessionGuid, bForceFinish);
		return;
	}

	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleDialogueRowProcessed(this, sessionGuid, bForceFinish);
}

void UMounteaDialogueManager::SkipDialogueRow_Implementation()
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		RequestSkipRow_Server(sessionGuid);
		return;
	}

	UMounteaDialogueWorldSubsystem* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem)
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
	{
		OnDialogueFailed.Broadcast(TEXT("[Skip Dialogue Row] Missing Dialogue Session on GameState."));
		return;
	}

	session->HandleSkipDialogueRow(this, sessionGuid);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void UMounteaDialogueManager::UpdateWorldDialogueUI_Implementation(const FString& Command)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] UpdateWorldDialogueUI is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
}

void UMounteaDialogueManager::ResetClientSyncCaches(const FGuid& SessionGUID)
{
	if (bClientAudioPlaying
		&& !UMounteaDialogueManagerStatics::IsServer(GetOwner())
		&& UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner())
		&& IsValid(DialogueContext)
		&& DialogueContext->ActiveDialogueParticipant.GetObject()
		&& DialogueContext->ActiveDialogueParticipant.GetInterface())
	{
		UMounteaDialogueParticipantStatics::SkipParticipantVoice(DialogueContext->ActiveDialogueParticipant, nullptr);
	}

	LastClientSyncSessionGUID = SessionGUID;
	LastReceivedPayloadVersion = 0;
	LastPlayedAudioRowGUID.Invalidate();
	LastPlayedAudioRowIndex = INDEX_NONE;
	bClientAudioPlaying = false;
}

void UMounteaDialogueManager::ReconcileClientAudioFromPayload(const FMounteaDialogueContextPayload& Payload, const bool bShouldPlayRowAudio)
{
	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
		return;

	const TScriptInterface<IMounteaDialogueParticipantInterface> activeParticipant = Payload.ActiveDialogueParticipant;

	if (!bShouldPlayRowAudio)
	{
		if (bClientAudioPlaying && activeParticipant.GetObject() && activeParticipant.GetInterface())
			UMounteaDialogueParticipantStatics::SkipParticipantVoice(activeParticipant, nullptr);

		bClientAudioPlaying = false;
		LastPlayedAudioRowGUID.Invalidate();
		LastPlayedAudioRowIndex = INDEX_NONE;
		return;
	}

	if (!activeParticipant.GetObject() || !activeParticipant.GetInterface())
		return;

	if (!UMounteaDialogueTraversalStatics::IsDialogueRowValid(Payload.ActiveDialogueRow))
		return;

	if (!Payload.ActiveDialogueRow.RowData.IsValidIndex(Payload.ActiveDialogueRowDataIndex))
		return;

	const FDialogueRowData& rowData = Payload.ActiveDialogueRow.RowData[Payload.ActiveDialogueRowDataIndex];
	if (!UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(rowData))
		return;

	if (bClientAudioPlaying
		&& LastPlayedAudioRowGUID == Payload.ActiveDialogueRow.RowGUID
		&& LastPlayedAudioRowIndex == Payload.ActiveDialogueRowDataIndex)
	{
		return;
	}

	UMounteaDialogueParticipantStatics::PlayParticipantVoice(activeParticipant, rowData.RowSound);
	LastPlayedAudioRowGUID = Payload.ActiveDialogueRow.RowGUID;
	LastPlayedAudioRowIndex = Payload.ActiveDialogueRowDataIndex;
	bClientAudioPlaying = true;
}

void UMounteaDialogueManager::ReconcileClientUIFromPayload(const FMounteaDialogueContextPayload& Payload)
{
	// UI reconciliation is now owned by UMounteaDialogueParticipantUserInterfaceComponent via signal dispatch.
	// Only audio reconciliation remains here.
	const bool bShouldPlayRowAudio = ManagerState == EDialogueManagerState::EDMS_Active
		&& UMounteaDialogueTraversalStatics::IsDialogueRowValid(Payload.ActiveDialogueRow)
		&& Payload.ActiveDialogueRow.RowData.IsValidIndex(Payload.ActiveDialogueRowDataIndex)
		&& UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(
			Payload.ActiveDialogueRow.RowData[Payload.ActiveDialogueRowDataIndex]);
	ReconcileClientAudioFromPayload(Payload, bShouldPlayRowAudio);
}

bool UMounteaDialogueManager::AddDialogueUIObject_Implementation(UObject* NewDialogueObject)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] AddDialogueUIObject is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	return false;
}

bool UMounteaDialogueManager::AddDialogueUIObjects_Implementation(const TArray<UObject*>& NewDialogueObjects)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] AddDialogueUIObjects is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	return false;
}

bool UMounteaDialogueManager::RemoveDialogueUIObject_Implementation(UObject* DialogueObjectToRemove)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] RemoveDialogueUIObject is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	return false;
}

bool UMounteaDialogueManager::RemoveDialogueUIObjects_Implementation(const TArray<UObject*>& DialogueObjectsToRemove)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] RemoveDialogueUIObjects is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	return false;
}

void UMounteaDialogueManager::SetDialogueUIObjects_Implementation(const TArray<UObject*>& NewDialogueObjects)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] SetDialogueUIObjects is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
}

void UMounteaDialogueManager::ResetDialogueUIObjects_Implementation()
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] ResetDialogueUIObjects is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
}

bool UMounteaDialogueManager::CreateDialogueUI_Implementation(FString& Message)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] CreateDialogueUI is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	Message = TEXT("Deprecated: use UMounteaDialogueParticipantUserInterfaceComponent.");
	return false;
}

bool UMounteaDialogueManager::UpdateDialogueUI_Implementation(FString& Message, const FString& Command)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] UpdateDialogueUI is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	Message = TEXT("Deprecated: use UMounteaDialogueParticipantUserInterfaceComponent.");
	return false;
}

bool UMounteaDialogueManager::CloseDialogueUI_Implementation()
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] CloseDialogueUI is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	return false;
}

void UMounteaDialogueManager::ExecuteWidgetCommand_Implementation(const FString& Command)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] ExecuteWidgetCommand is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

TSubclassOf<UUserWidget> UMounteaDialogueManager::GetDialogueWidgetClass() const
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] GetDialogueWidgetClass is deprecated. Use UMounteaDialogueConfiguration::DefaultDialogueWidgetClass."))
	return nullptr;
}

void UMounteaDialogueManager::SetDialogueWidgetClass(const TSubclassOf<UUserWidget> NewWidgetClass)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] SetDialogueWidgetClass is deprecated. Use UMounteaDialogueConfiguration::DefaultDialogueWidgetClass."))
}

void UMounteaDialogueManager::SetDialogueWidget_Implementation(UUserWidget* NewDialogueWidget)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] SetDialogueWidget is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
}

UUserWidget* UMounteaDialogueManager::GetDialogueWidget_Implementation() const
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] GetDialogueWidget is deprecated. Use UMounteaDialogueParticipantUserInterfaceComponent."))
	return nullptr;
}

int32 UMounteaDialogueManager::GetDialogueWidgetZOrder_Implementation() const
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] GetDialogueWidgetZOrder is deprecated. Use UMounteaDialogueConfiguration::DefaultDialogueWidgetZOrder."))
	return 0;
}

void UMounteaDialogueManager::SetDialogueWidgetZOrder_Implementation(const int32 NewZOrder)
{
	LOG_WARNING(TEXT("[MounteaDialogueManager] SetDialogueWidgetZOrder is deprecated. Use UMounteaDialogueConfiguration::DefaultDialogueWidgetZOrder."))
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS


