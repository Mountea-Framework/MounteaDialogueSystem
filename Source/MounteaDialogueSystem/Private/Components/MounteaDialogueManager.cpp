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
#include "Blueprint/GameViewportSubsystem.h"
#include "Graph/MounteaDialogueGraph.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueContextStatics.h"
#include "GameFramework/PlayerState.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueManagerStatics.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Settings/MounteaDialogueSystemSettings.h"
#include "Components/MounteaDialogueSession.h"
#include "GameFramework/GameStateBase.h"
#include "Subsystem/MounteaDialogueWorldSubsystem.h"


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
	DialogueContext->LastWidgetCommand = Payload.LastWidgetCommand;

	OnDialogueContextUpdated.Broadcast(DialogueContext);

	if (ManagerState == EDialogueManagerState::EDMS_Active)
		StartParticipants();

	NotifyParticipants(Payload.DialogueParticipants);

	if (!Payload.LastWidgetCommand.IsEmpty())
		ProcessWorldWidgetUpdate(Payload.LastWidgetCommand);
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
	request.MainParticipantActor = TSoftObjectPtr<AActor>(InitialParticipants.MainParticipant);
	for (const auto& other : InitialParticipants.OtherParticipants)
	{
		if (AActor* otherActor = Cast<AActor>(other))
			request.OtherParticipantActors.Add(TSoftObjectPtr<AActor>(otherActor));
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
		RequestCloseDialogue_Server(FGuid());
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

	FString resultMessage;
	if (!Execute_CreateDialogueUI(this, resultMessage))
		LOG_WARNING(TEXT("[Create Dialogue UI] %s"), *(resultMessage))

	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueStarted.Broadcast(DialogueContext);

	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		Execute_PrepareNode(this);
		return;
	}
}

void UMounteaDialogueManager::CloseDialogue_Implementation()
{
	if (UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		const FGuid closingSessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
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
	
	Execute_CloseDialogueUI(this);
	
	Execute_CleanupDialogue(this);

	SetDialogueContext(nullptr);
	
	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueClosed.Broadcast(DialogueContext);

	DialogueInstigator = nullptr;
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

	RequestNodeProcessed_Server(sessionGuid);
}

void UMounteaDialogueManager::SelectNode_Implementation(const FGuid& NodeGuid)
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		RequestSelectNode_Server(sessionGuid, NodeGuid);
		return;
	}

	RequestSelectNode_Server(sessionGuid, NodeGuid);
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

	RequestDialogueRowProcessed_Server(sessionGuid, bForceFinish);
}

void UMounteaDialogueManager::SkipDialogueRow_Implementation()
{
	const FGuid sessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	if (!UMounteaDialogueManagerStatics::IsServer(GetOwner()))
	{
		RequestSkipRow_Server(sessionGuid);
		return;
	}

	RequestSkipRow_Server(sessionGuid);
}

void UMounteaDialogueManager::UpdateWorldDialogueUI_Implementation(const FString& Command)
{
	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		ProcessWorldWidgetUpdate(Command);
}

void UMounteaDialogueManager::ProcessWorldWidgetUpdate(const FString& Command)
{
	if (LastDialogueCommand == Command)
		return;

	for (const auto& dialogueObject : DialogueObjects)
	{
		if (dialogueObject)
			IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(dialogueObject, this, Command);
	}

	LastDialogueCommand = Command;
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
			bAllAdded = false;
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
			bAllRemoved = false;
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
	bool bSuccess = true;
   
	if (GetDialogueWidgetClass() == nullptr)
	{
		Message = TEXT("Invalid Widget Class! Setup Widget class at least in Project settings!");
		bSuccess = false;
	}
   
	if (!GetWorld())
	{
		Message = TEXT("Invalid World!");
		bSuccess = false;
	}
   
	int seachDepth = 0;
	APlayerController* playerController = UMounteaDialogueParticipantStatics::FindPlayerController(GetOwner(), seachDepth);
	if (!playerController || !playerController->IsLocalController())
	{
		Message = !playerController ? TEXT("Invalid Player Controller!") : TEXT("UI can be shown only to Local Players!");
		bSuccess = false;
	}

	if (bSuccess)
	{
		auto newWidget = CreateWidget<UUserWidget>(playerController, GetDialogueWidgetClass());
		if (!newWidget || !newWidget->Implements<UMounteaDialogueWBPInterface>())
		{
			Message = !newWidget ? TEXT("Cannot spawn Dialogue Widget!") : TEXT("Does not implement Dialogue Widget Interface!");
			bSuccess = false;
		}
		else
			Execute_SetDialogueWidget(this, newWidget);
	}

	return Execute_UpdateDialogueUI(this, Message, MounteaDialogueWidgetCommands::CreateDialogueWidget);
}

bool UMounteaDialogueManager::UpdateDialogueUI_Implementation(FString& Message, const FString& Command)
{
	if (IsValid(DialogueContext))
		DialogueContext->LastWidgetCommand = Command;

	if (DialogueWidget)
		IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidget, this, Command);

	Execute_UpdateWorldDialogueUI(this, Command);
	return true;
}

bool UMounteaDialogueManager::CloseDialogueUI_Implementation()
{
	FString dialogueMessage;
	const bool bSatisfied = Execute_UpdateDialogueUI(this, dialogueMessage, MounteaDialogueWidgetCommands::CloseDialogueWidget);

	if (IsValid((DialogueWidget)))
	{
		DialogueWidget->MarkAsGarbage();
		DialogueWidget->RemoveFromParent();
	}

	Execute_SetDialogueWidget(this, nullptr);
	
	return bSatisfied;
}

void UMounteaDialogueManager::ExecuteWidgetCommand_Implementation(const FString& Command)
{
	FString resultMessage;
	if (!Execute_UpdateDialogueUI(this, resultMessage, Command))
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
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
	if (NewZOrder == DialogueWidgetZOrder) return;

	DialogueWidgetZOrder = NewZOrder;
	
	auto dialogueWidget = Execute_GetDialogueWidget(this);
	if (!dialogueWidget) return;

	ULocalPlayer* localPlayer = dialogueWidget->GetOwningLocalPlayer();
	if (!localPlayer) return;

	UGameViewportSubsystem* viewportSubsystem = UGameViewportSubsystem::Get(GetWorld());
	if (!viewportSubsystem) return;

	FGameViewportWidgetSlot widgetSlot = viewportSubsystem->GetWidgetSlot(dialogueWidget);
	widgetSlot.ZOrder = NewZOrder;

	viewportSubsystem->AddWidgetForPlayer(dialogueWidget, localPlayer, widgetSlot);
}
