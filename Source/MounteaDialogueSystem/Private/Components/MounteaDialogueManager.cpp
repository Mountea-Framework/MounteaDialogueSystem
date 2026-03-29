// All rights reserved Dominik Pavlicek 2023

#include "Components/MounteaDialogueManager.h"

#include "TimerManager.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Graph/MounteaDialogueGraph.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "GameFramework/PlayerState.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
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

	if (UMounteaDialogueSystemBFC::IsServer(GetOwner()))
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
	Info.bIsActiveRowValid = UMounteaDialogueSystemBFC::IsDialogueRowValid(dialogueRow);

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

	if (!UMounteaDialogueSystemBFC::IsServer(GetOwner()))
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
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
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
	DialogueContext->ActiveDialogueParticipant = Payload.ActiveDialogueParticipant;
	DialogueContext->DialogueParticipants = Payload.DialogueParticipants;

	UMounteaDialogueGraph* activeGraph = nullptr;
	const TScriptInterface<IMounteaDialogueParticipantInterface> graphSource =
		UMounteaDialogueSystemBFC::GetGraphOwnerParticipant(Payload.DialogueParticipants);
	if (graphSource.GetObject() && graphSource.GetInterface())
		activeGraph = graphSource->Execute_GetDialogueGraph(graphSource.GetObject());

	if (activeGraph)
	{
		DialogueContext->ActiveNode = UMounteaDialogueSystemBFC::FindNodeByGUID(activeGraph, Payload.ActiveNodeGUID);
		DialogueContext->PreviousActiveNode = Payload.PreviousNodeGUID;
		DialogueContext->AllowedChildNodes = UMounteaDialogueSystemBFC::FindNodesByGUID(activeGraph, Payload.AllowedChildNodeGUIDs);
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

	NotifyParticipants(Payload.DialogueParticipants);

	if (!Payload.LastWidgetCommand.IsEmpty())
		ProcessWorldWidgetUpdate(Payload.LastWidgetCommand);

	// Client-side deferred start: if the initial payload arrives after state was already set to Active
	// (i.e. the replication order was state-first, payload-second), kick off the traversal loop now.
	// ContextVersion == 1 identifies the initial payload written by HandleStartRequest.
	// The server never uses this path — it calls PrepareNode directly from StartDialogue_Implementation.
	if (!UMounteaDialogueSystemBFC::IsServer(GetOwner())
		&& Payload.ContextVersion == 1
		&& ManagerState == EDialogueManagerState::EDMS_Active
		&& IsValid(DialogueContext) && IsValid(DialogueContext->ActiveNode))
	{
		Execute_PrepareNode(this);
	}
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

void UMounteaDialogueManager::SyncContext(const FMounteaDialogueContextReplicatedStruct& Context)
{
	// Deprecated: context sync now handled by FMounteaDialogueContextPayload via UMounteaDialogueSession.
}

void UMounteaDialogueManager::WritePayloadFromContext()
{
	if (!UMounteaDialogueSystemBFC::IsServer(GetOwner()))
		return;
	if (!IsValid(DialogueContext))
		return;

	auto* subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (!subsystem) 
		return;

	UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session) 
		return;

	FMounteaDialogueContextPayload newPayload = session->GetContextPayload();

	if (IsValid(DialogueContext->ActiveNode))
	{
		newPayload.PreviousNodeGUID = newPayload.ActiveNodeGUID;
		newPayload.ActiveNodeGUID = DialogueContext->ActiveNode->GetNodeGUID();
	}

	newPayload.AllowedChildNodeGUIDs.Reset();
	for (const auto& child : DialogueContext->AllowedChildNodes)
		if (IsValid(child)) 
			newPayload.AllowedChildNodeGUIDs.Add(child->GetNodeGUID());

	newPayload.ActiveDialogueParticipant = DialogueContext->ActiveDialogueParticipant;
	newPayload.ActiveDialogueRow = DialogueContext->ActiveDialogueRow;
	newPayload.ActiveDialogueRowDataIndex = DialogueContext->ActiveDialogueRowDataIndex;

	session->WriteContextPayload(MoveTemp(newPayload));
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
	
	if (!UMounteaDialogueSystemBFC::IsServer(GetOwner()))
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

	OnDialogueContextUpdated.Broadcast(NewContext);
}

void UMounteaDialogueManager::UpdateDialogueContext_Implementation(UMounteaDialogueContext* NewContext)
{
	if (NewContext == DialogueContext) 
		return;

	(*DialogueContext) += NewContext;

	OnDialogueContextUpdated.Broadcast(NewContext);
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

	if (!UMounteaDialogueSystemBFC::IsServer(GetOwner()))
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
	Execute_SelectNode(this, NodeGUID);
}

void UMounteaDialogueManager::RequestSkipRow_Server_Implementation(FGuid SessionGUID)
{
	Execute_SkipDialogueRow(this);
}

void UMounteaDialogueManager::RequestCloseDialogue_Server_Implementation(FGuid SessionGUID)
{
	Execute_RequestCloseDialogue(this);
}

void UMounteaDialogueManager::RequestCloseDialogue_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	if (!UMounteaDialogueSystemBFC::IsServer(GetOwner()))
	{
		RequestCloseDialogue_Server(FGuid());
		return;
	}

	SetManagerState(DefaultManagerState);
}

void UMounteaDialogueManager::StartParticipants()
{
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
}

void UMounteaDialogueManager::StopParticipants() const
{
	if (!IsValid(DialogueContext))
		return;

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
		
		UMounteaDialogueSystemBFC::SaveTraversePathToParticipant(DialogueContext->TraversedPath, dialogueParticipant);
		
		dialogueParticipant->Execute_SetParticipantState(participantObject, dialogueParticipant->Execute_GetDefaultParticipantState(participantObject));
		dialogueParticipant->GetOnDialogueEndedEventHandle().Broadcast();
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
		StopParticipants();
		OnDialogueFailed.Broadcast(ResultMessage);
	}
}

void UMounteaDialogueManager::StartDialogue_Implementation()
{
	StartParticipants();

	FString resultMessage;
	if (!Execute_CreateDialogueUI(this, resultMessage))
		LOG_WARNING(TEXT("[Create Dialogue UI] %s"), *(resultMessage))

	if (UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueStarted.Broadcast(DialogueContext);

	// Server: context was written to session before Broadcast(true) fired, so it's guaranteed valid here.
	// Client: context may not have arrived yet via OnRep_ContextPayload. OnContextPayloadUpdated will
	// trigger PrepareNode when the initial payload (ContextVersion == 1) arrives while state is Active.
	if (UMounteaDialogueSystemBFC::IsServer(GetOwner()))
		Execute_PrepareNode(this);
}

void UMounteaDialogueManager::CloseDialogue_Implementation()
{
	StopParticipants();
	
	Execute_CloseDialogueUI(this);
	
	Execute_CleanupDialogue(this);

	SetDialogueContext(nullptr);
	
	if (UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueClosed.Broadcast(DialogueContext);

	DialogueInstigator = nullptr;
}

void UMounteaDialogueManager::CleanupDialogue_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);
	
	if (!UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
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

	const auto newActiveParticipant = UMounteaDialogueSystemBFC::SwitchActiveParticipant(DialogueContext);
	
	UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(DialogueContext, newActiveParticipant);
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
	if (DialogueContext && DialogueContext->ActiveNode && DialogueContext->ActiveDialogueParticipant.GetObject())
	{
		DialogueContext->ActiveNode->ProcessNode(this);

		DialogueContext->ActiveDialogueParticipant->GetOnParticipantBecomeActiveEventHandle().Broadcast(true);
		OnDialogueNodeStarted.Broadcast(DialogueContext);

		Execute_ProcessDialogueRow(this);
	}
	else
		OnDialogueFailed.Broadcast(TEXT("[Process Node] Invalid Context or Active Node or Active Dialogue Participant!"));
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
	
	OnDialogueNodeFinished.Broadcast(DialogueContext);
	DialogueContext->ActiveNode->CleanupNode();
	
	// TODO: This is extremely similar to NodeSelected!
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
		auto allowedChildNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(newActiveNode);
		UMounteaDialogueSystemBFC::SortNodes(allowedChildNodes);

		DialogueContext->SetDialogueContext(newActiveNode, allowedChildNodes);
		DialogueContext->UpdateActiveDialogueRow(UMounteaDialogueSystemBFC::GetSpeechData(newActiveNode));
		DialogueContext->UpdateActiveDialogueRowDataIndex(0);
		const auto newActiveParticipant = UMounteaDialogueSystemBFC::SwitchActiveParticipant(DialogueContext);
		UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(DialogueContext, newActiveParticipant);

		OnDialogueNodeSelected.Broadcast(DialogueContext);

		WritePayloadFromContext();

		Execute_PrepareNode(this);
	}
	else
	{
		DialogueContext->AllowedChildNodes = allowedChildrenNodes;
		WritePayloadFromContext();

		FString resultMessage;
		if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::AddDialogueOptions))
			LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
	}
}

void UMounteaDialogueManager::SelectNode_Implementation(const FGuid& NodeGuid)
{
	if (!UMounteaDialogueSystemBFC::IsServer(GetOwner()))
	{
		RequestSelectNode_Server(FGuid(), NodeGuid);
		return;
	}

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

	DialogueContext->SetDialogueContext(selectedNode, allowedChildNodes);
	DialogueContext->UpdateActiveDialogueRow(UMounteaDialogueSystemBFC::GetSpeechData(selectedNode));
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);
	const auto newActiveParticipant = UMounteaDialogueSystemBFC::SwitchActiveParticipant(DialogueContext);
	UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(DialogueContext, newActiveParticipant);
	
	WritePayloadFromContext();

	FString resultMessage;
	if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::RemoveDialogueOptions))
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)

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
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)

	if (DialogueContext->GetActiveDialogueRow().RowData.IsValidIndex(DialogueContext->GetActiveDialogueRowDataIndex()) == false)
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Trying to Access Invalid Dialogue Row data!"));
		return;
	}

	const int32 activeIndex = DialogueContext->GetActiveDialogueRowDataIndex();
	const auto Row = DialogueContext->GetActiveDialogueRow();
	bool bValidRowData = Row.RowData.IsValidIndex(activeIndex);

	if (!bValidRowData)
	{
		LOG_WARNING(TEXT("[Process Dialogue Row] Invalid Dialogue Row Data at index %d! Skipping Row. Next Row will be processed instead."), activeIndex)
		Execute_DialogueRowProcessed(this, false);
		return;
	}
	
	const FDialogueRowData RowData = Row.RowData[activeIndex];
	bValidRowData = UMounteaDialogueSystemBFC::IsDialogueRowDataValid(RowData);

	if (!bValidRowData)
	{
		LOG_WARNING(TEXT("[Process Dialogue Row] Invalid Dialogue Row Data! Skipping Row. Next Row will be processed instead."))
		Execute_DialogueRowProcessed(this, false);
		return;
	}

	OnDialogueRowStarted.Broadcast(DialogueContext);
	
	if (bValidRowData)
	{
		DialogueContext->ActiveDialogueParticipant->Execute_PlayParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), RowData.RowSound);
		
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UMounteaDialogueManager::DialogueRowProcessed_Implementation, false);
		
		GetWorld()->GetTimerManager().SetTimer
		(
			TimerHandle_RowTimer,
			Delegate,
			UMounteaDialogueSystemBFC::GetRowDuration(RowData),
			false
		);
	}
}

void UMounteaDialogueManager::DialogueRowProcessed_Implementation(const bool bForceFinish)
{
	// To avoid race conditions simply return if active
	if (ManagerState != EDialogueManagerState::EDMS_Active)
		return;

	if (!IsValid(DialogueContext))
	{
		LOG_ERROR(TEXT("[Process Dialogue Row] Invalid Dialogue Context!"))
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Invalid Dialogue Context!"));
		return;
	}
	
	FString resultMessage;
	if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::HideDialogueRow))
		LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
	
	if (!IsValid(GetWorld()))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] World is not Valid!"));
		return;
	}
	
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const auto processInfo = MounteaDialogueManagerHelpers::GetDialogueRowDataInfo(DialogueContext);
	
	if (processInfo.ActiveRowExecutionMode == ERowExecutionMode::EREM_AwaitInput && !bForceFinish)
	{
		LOG_INFO(TEXT("[Process Dialogue Row] Manual Input is Required to Skip/Finish this Row!"))
		return;
	}

	OnDialogueRowFinished.Broadcast(DialogueContext);
	
	if (processInfo.bIsActiveRowValid && processInfo.bDialogueRowDataValid)
	{
		switch (processInfo.NextRowExecutionMode)
		{
			case ERowExecutionMode::EREM_Automatic:
			case ERowExecutionMode::EREM_AwaitInput:
				{
					DialogueContext->UpdateActiveDialogueRowDataIndex(processInfo.IncreasedIndex);
					OnDialogueContextUpdated.Broadcast(DialogueContext);

					WritePayloadFromContext();
					Execute_ProcessDialogueRow(this);
				}
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
		OnDialogueFailed.Broadcast(TEXT("[Skip Dialogue Row] Invalid Dialogue Context!"));
		return;
	}
	
	if (!IsValid(GetWorld()))
	{
		OnDialogueFailed.Broadcast(TEXT("[Skip Dialogue Row] World is not Valid!"));
		return;
	}
	
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	DialogueContext->ActiveDialogueParticipant->Execute_SkipParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), nullptr);

	Execute_DialogueRowProcessed(this, true);
}

void UMounteaDialogueManager::UpdateWorldDialogueUI_Implementation(const FString& Command)
{
	if (UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner()))
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
	APlayerController* playerController = UMounteaDialogueSystemBFC::FindPlayerController(GetOwner(), seachDepth);
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
