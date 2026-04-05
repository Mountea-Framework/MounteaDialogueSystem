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

#include "Components/MounteaDialogueSession.h"
#include "Components/MounteaDialogueManager.h"
#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueContextStatics.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueManagerStatics.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"
#include "Interfaces/Nodes/MounteaDialogueSpeechDataInterface.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Settings/MounteaDialogueSystemSettings.h"
#include "Subsystem/MounteaDialogueWorldSubsystem.h"
#include "TimerManager.h"

UMounteaDialogueSession::UMounteaDialogueSession()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UMounteaDialogueSession::BeginPlay()
{
	Super::BeginPlay();
}

void UMounteaDialogueSession::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UMounteaDialogueSession, ContextPayload, Params);
}

void UMounteaDialogueSession::OnRep_ContextPayload()
{
	if (!ContextPayload.SessionGUID.IsValid())
		return; // Expected: initial replication before any dialogue starts.

	if (LastDeliveredSessionGUID != ContextPayload.SessionGUID)
	{
		LastDeliveredSessionGUID = ContextPayload.SessionGUID;
		LastDeliveredContextVersion = 0;
	}

	const int32 newVersion = ContextPayload.ContextVersion;
	if (newVersion <= 0 || newVersion <= LastDeliveredContextVersion)
	{
		const FString staleKey = FString::Printf(TEXT("Diag.Session.OnRep.Client.Version"));
		LOG_INFO_KEY(staleKey, TEXT("[Diag][Session][OnRep] Stale/invalid version. New=%d Last=%d"), newVersion, LastDeliveredContextVersion)
		if (bClientDispatchPending)
			TryDispatchPendingClientPayload();
		return;
	}

	const int32 versionGap = newVersion - LastDeliveredContextVersion;
	if (LastDeliveredContextVersion > 0 && versionGap >= 10)
		LOG_WARNING(TEXT("[Dialogue Session] Payload versions jumped from %d to %d."), LastDeliveredContextVersion, newVersion)

	LastDeliveredContextVersion = newVersion;
	NotifyLocalManagers();
}

void UMounteaDialogueSession::WriteContextPayload(FMounteaDialogueContextPayload NewPayload)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	if (!NewPayload.SessionGUID.IsValid())
	{
		LOG_WARNING(TEXT("[MounteaDialogueSession] WriteContextPayload skipped — invalid SessionGUID."))
		return;
	}

	const bool bSessionChanged = ContextPayload.SessionGUID.IsValid()
		&& NewPayload.SessionGUID.IsValid()
		&& ContextPayload.SessionGUID != NewPayload.SessionGUID;
	if (bSessionChanged)
	{
		RoleOverrides.Empty();
		SessionTraversedPath.Empty();
	}

	NewPayload.ContextVersion = ContextPayload.ContextVersion + 1;
	ContextPayload = MoveTemp(NewPayload);
	MARK_PROPERTY_DIRTY_FROM_NAME(UMounteaDialogueSession, ContextPayload, this);
	LastDeliveredContextVersion = ContextPayload.ContextVersion;
	LastDeliveredSessionGUID = ContextPayload.SessionGUID;

	NotifyLocalManagers();
}

void UMounteaDialogueSession::TryDispatchPendingClientPayload()
{
	if (GetOwner() && GetOwner()->HasAuthority())
		return;

	UWorld* world = GetWorld();
	if (!world)
		return;

	if (!bClientDispatchPending)
	{
		if (world->GetTimerManager().IsTimerActive(PendingDispatchRetryTimer))
			world->GetTimerManager().ClearTimer(PendingDispatchRetryTimer);
		return;
	}

	UMounteaDialogueWorldSubsystem* subsystem = world->GetSubsystem<UMounteaDialogueWorldSubsystem>();
	if (!subsystem)
		return;

	for (UMounteaDialogueManager* manager : subsystem->GetRegisteredManagers())
	{
		if (!IsValid(manager))
			continue;

		const AActor* ownerActor = manager->GetOwner();
		if (!IsValid(ownerActor))
			continue;

		const bool bIsLocal = UMounteaDialogueManagerStatics::IsLocalPlayer(ownerActor);
		if (!bIsLocal)
			continue;
		
		manager->OnContextPayloadUpdated(ContextPayload);
		bClientDispatchPending = false;
		LastPendingDispatchWarningVersion = 0;
		PendingClientDispatchVersion = 0;
		PendingClientDispatchSessionGUID.Invalidate();
		if (world->GetTimerManager().IsTimerActive(PendingDispatchRetryTimer))
			world->GetTimerManager().ClearTimer(PendingDispatchRetryTimer);
		LOG_INFO(TEXT("[Dialogue Session] Delivered pending payload version %d to local manager '%s'."), ContextPayload.ContextVersion, *GetNameSafe(manager))
		return;
	}

	if (LastPendingDispatchWarningVersion != PendingClientDispatchVersion)
	{
		LastPendingDispatchWarningVersion = PendingClientDispatchVersion;
		LOG_ERROR_KEY(FString::Printf(TEXT("Diag.Session.PendingMissing")), TEXT("[Diag][Session][TryDispatchPending] No local manager found for pending payload. Session=%s Version=%d"),
			*PendingClientDispatchSessionGUID.ToString(), PendingClientDispatchVersion)
		LOG_WARNING(TEXT("[Dialogue Session] Pending payload version %d could not be delivered yet: no local manager registered."), PendingClientDispatchVersion)
	}

	if (!world->GetTimerManager().IsTimerActive(PendingDispatchRetryTimer))
		world->GetTimerManager().SetTimer(PendingDispatchRetryTimer, this, &UMounteaDialogueSession::TryDispatchPendingClientPayload, 0.25f, true);
}

void UMounteaDialogueSession::SetAuthoritativeManager(UMounteaDialogueManager* Manager)
{
	AuthoritativeManager = Manager;
}

void UMounteaDialogueSession::FinalizeSession()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	if (SessionTraversedPath.Num() > 0)
	{
		for (const auto& participant : ContextPayload.DialogueParticipants)
		{
			if (!participant.GetObject() || !participant.GetInterface())
				continue;

			TArray<FDialogueTraversePath> participantPath = SessionTraversedPath;
			UMounteaDialogueParticipantStatics::SaveTraversedPath(participant, participantPath);
		}
	}

	AuthoritativeManager.Reset();
	RoleOverrides.Empty();
	SessionTraversedPath.Empty();
}

void UMounteaDialogueSession::SetRoleOverride(const EDialogueParticipantType Role, const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	const int32 roleMask = static_cast<int32>(Role);
	if (roleMask == 0)
		return;

	if (!Participant.GetObject() || !Participant.GetInterface())
	{
		RoleOverrides.Remove(roleMask);
		return;
	}

	RoleOverrides.FindOrAdd(roleMask) = Participant;
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSession::GetRoleOverride(const EDialogueParticipantType Role) const
{
	const int32 roleMask = static_cast<int32>(Role);
	if (roleMask == 0)
		return nullptr;

	const TScriptInterface<IMounteaDialogueParticipantInterface>* foundOverride = RoleOverrides.Find(roleMask);
	if (!foundOverride)
		return nullptr;

	if (!foundOverride->GetObject() || !foundOverride->GetInterface())
		return nullptr;

	return *foundOverride;
}

void UMounteaDialogueSession::AddTraversedNode(const UMounteaDialogueGraphNode* TraversedNode)
{
	if (!IsValid(TraversedNode))
		return;

	FDialogueTraversePath* existingEntry = SessionTraversedPath.FindByPredicate(
		[TraversedNode](const FDialogueTraversePath& Path)
		{
			return Path.NodeGuid == TraversedNode->GetNodeGUID()
				&& Path.GraphGuid == TraversedNode->GetGraphGUID();
		});

	if (existingEntry)
	{
		existingEntry->TraverseCount++;
		return;
	}

	FDialogueTraversePath newEntry;
	newEntry.NodeGuid = TraversedNode->GetNodeGUID();
	newEntry.GraphGuid = TraversedNode->GetGraphGUID();
	newEntry.TraverseCount = 1;
	SessionTraversedPath.Add(newEntry);
}

TArray<FDialogueTraversePath> UMounteaDialogueSession::GetConditionTraversedPath_Implementation() const
{
	if (SessionTraversedPath.Num() > 0)
		return SessionTraversedPath;

	TArray<FDialogueTraversePath> result;
	for (const auto& participant : ContextPayload.DialogueParticipants)
	{
		if (!participant.GetObject() || !participant.GetInterface())
			continue;
		const TArray<FDialogueTraversePath> participantPath = participant->Execute_GetTraversedPath(participant.GetObject());
		for (const auto& entry : participantPath)
		{
			result.AddUnique(entry);
		}
	}
	return result;
}

void UMounteaDialogueSession::NotifyLocalManagers()
{
	UWorld* world = GetWorld();
	if (!world)
		return;

	UMounteaDialogueWorldSubsystem* subsystem = world->GetSubsystem<UMounteaDialogueWorldSubsystem>();
	if (!subsystem)
		return;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (IsValid(AuthoritativeManager.Get()))
		{
			AuthoritativeManager->OnContextPayloadUpdated(ContextPayload);
			return;
		}

		for (UMounteaDialogueManager* manager : subsystem->GetRegisteredManagers())
		{
			if (!IsValid(manager))
				continue;
			if (!IsValid(manager->GetOwner()) || !manager->GetOwner()->HasAuthority())
				continue;

			AuthoritativeManager = manager;
			manager->OnContextPayloadUpdated(ContextPayload);
			return;
		}

		LOG_ERROR_KEY(FString::Printf(TEXT("Diag.Session.Notify.ServerMissing")), TEXT("[Diag][Session][Notify] Missing authoritative manager. Session=%s Version=%d"),
			*ContextPayload.SessionGUID.ToString(), ContextPayload.ContextVersion)
		LOG_WARNING(TEXT("[Dialogue Session] Missing authoritative manager while dispatching payload version %d."), ContextPayload.ContextVersion)
		return;
	}

	for (UMounteaDialogueManager* manager : subsystem->GetRegisteredManagers())
	{
		if (!IsValid(manager))
			continue;

		const AActor* ownerActor = manager->GetOwner();
		if (!IsValid(ownerActor))
			continue;

		const bool bIsLocal = UMounteaDialogueManagerStatics::IsLocalPlayer(ownerActor);
		if (!bIsLocal)
			continue;

		manager->OnContextPayloadUpdated(ContextPayload);
		bClientDispatchPending = false;
		LastPendingDispatchWarningVersion = 0;
		PendingClientDispatchVersion = 0;
		PendingClientDispatchSessionGUID.Invalidate();
		if (world->GetTimerManager().IsTimerActive(PendingDispatchRetryTimer))
			world->GetTimerManager().ClearTimer(PendingDispatchRetryTimer);
		return;
	}

	bClientDispatchPending = true;
	PendingClientDispatchSessionGUID = ContextPayload.SessionGUID;
	PendingClientDispatchVersion = ContextPayload.ContextVersion;
	LOG_ERROR_KEY(FString::Printf(TEXT("Diag.Session.Notify.ClientPending")), TEXT("[Diag][Session][Notify] Client dispatch pending. Session=%s Version=%d"),
		*PendingClientDispatchSessionGUID.ToString(), PendingClientDispatchVersion)
	if (!world->GetTimerManager().IsTimerActive(PendingDispatchRetryTimer))
		world->GetTimerManager().SetTimer(PendingDispatchRetryTimer, this, &UMounteaDialogueSession::TryDispatchPendingClientPayload, 0.25f, true);
}

bool UMounteaDialogueSession::IsSessionRequestValid(UMounteaDialogueManager* Manager, const FGuid& SessionGUID, const TCHAR* ActionName) const
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		LOG_ERROR_KEY(FString(ANSI_TO_TCHAR(__FUNCTION__)), TEXT("[Diag][Session][Validate] %s rejected: no authority. Manager=%s"), ActionName, *GetNameSafe(Manager))
		return false;
	}

	if (!IsValid(Manager))
	{
		LOG_ERROR_KEY(FString(ANSI_TO_TCHAR(__FUNCTION__)), TEXT("[Diag][Session][Validate] %s rejected: invalid manager."), ActionName)
		return false;
	}

	if (AuthoritativeManager.IsValid() && Manager != AuthoritativeManager.Get())
	{
		LOG_ERROR_KEY(FString(ANSI_TO_TCHAR(__FUNCTION__)), TEXT("[Diag][Session][Validate] %s rejected: manager not authoritative. Manager=%s Auth=%s"),
			ActionName, *GetNameSafe(Manager), *GetNameSafe(AuthoritativeManager.Get()))
		Manager->GetDialogueFailedEventHandle().Broadcast(FString::Printf(TEXT("[%s] Manager is not authoritative!"), ActionName));
		return false;
	}

	const FGuid activeSessionGUID = ContextPayload.SessionGUID;
	if (!activeSessionGUID.IsValid())
	{
		LOG_ERROR_KEY(FString(ANSI_TO_TCHAR(__FUNCTION__)), TEXT("[Diag][Session][Validate] %s rejected: no active session. Manager=%s"), ActionName, *GetNameSafe(Manager))
		Manager->GetDialogueFailedEventHandle().Broadcast(FString::Printf(TEXT("[%s] No active session!"), ActionName));
		return false;
	}

	if (!SessionGUID.IsValid())
	{
		LOG_ERROR_KEY(FString(ANSI_TO_TCHAR(__FUNCTION__)), TEXT("[Diag][Session][Validate] %s rejected: invalid request GUID. Active=%s"),
			ActionName, *activeSessionGUID.ToString())
		Manager->GetDialogueFailedEventHandle().Broadcast(FString::Printf(TEXT("[%s] Invalid session GUID!"), ActionName));
		return false;
	}

	if (SessionGUID != activeSessionGUID)
	{
		LOG_ERROR_KEY(FString(ANSI_TO_TCHAR(__FUNCTION__)), TEXT("[Diag][Session][Validate] %s rejected: mismatch. Requested=%s Active=%s Manager=%s"),
			ActionName, *SessionGUID.ToString(), *activeSessionGUID.ToString(), *GetNameSafe(Manager))
		Manager->GetDialogueFailedEventHandle().Broadcast(FString::Printf(TEXT("[%s] Session mismatch!"), ActionName));
		return false;
	}

	return true;
}

void UMounteaDialogueSession::ApplyNodeSwitchPayload(const UMounteaDialogueContext* DialogueContext)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(DialogueContext))
		return;

	FMounteaDialogueContextPayload newPayload = ContextPayload;

	if (DialogueContext->SessionGUID.IsValid())
		newPayload.SessionGUID = DialogueContext->SessionGUID;

	newPayload.DialogueParticipants = DialogueContext->DialogueParticipants;
	newPayload.ActiveDialogueParticipant = DialogueContext->ActiveDialogueParticipant;
	newPayload.ActiveDialogueRow = DialogueContext->ActiveDialogueRow;
	newPayload.ActiveDialogueRowDataIndex = DialogueContext->ActiveDialogueRowDataIndex;

	if (IsValid(DialogueContext->ActiveNode))
	{
		newPayload.PreviousNodeGUID = ContextPayload.ActiveNodeGUID;
		newPayload.ActiveNodeGUID = DialogueContext->ActiveNode->GetNodeGUID();
		newPayload.ActiveGraphGUID = DialogueContext->ActiveNode->GetGraphGUID();
	}
	else
	{
		newPayload.ActiveNodeGUID.Invalidate();
		newPayload.ActiveGraphGUID.Invalidate();
	}

	newPayload.AllowedChildNodeGUIDs.Reset();
	for (UMounteaDialogueGraphNode* childNode : DialogueContext->AllowedChildNodes)
	{
		if (IsValid(childNode))
			newPayload.AllowedChildNodeGUIDs.Add(childNode->GetNodeGUID());
	}

	WriteContextPayload(MoveTemp(newPayload));
}

void UMounteaDialogueSession::ApplyAllowedChildrenPayload(const UMounteaDialogueContext* DialogueContext)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(DialogueContext))
		return;

	FMounteaDialogueContextPayload newPayload = ContextPayload;

	if (DialogueContext->SessionGUID.IsValid())
		newPayload.SessionGUID = DialogueContext->SessionGUID;

	newPayload.DialogueParticipants = DialogueContext->DialogueParticipants;
	newPayload.ActiveDialogueParticipant = DialogueContext->ActiveDialogueParticipant;
	newPayload.ActiveDialogueRow = DialogueContext->ActiveDialogueRow;
	newPayload.ActiveDialogueRowDataIndex = DialogueContext->ActiveDialogueRowDataIndex;

	if (IsValid(DialogueContext->ActiveNode))
	{
		newPayload.ActiveNodeGUID = DialogueContext->ActiveNode->GetNodeGUID();
		newPayload.ActiveGraphGUID = DialogueContext->ActiveNode->GetGraphGUID();
	}

	newPayload.PreviousNodeGUID = DialogueContext->PreviousActiveNode.IsValid()
		? DialogueContext->PreviousActiveNode
		: newPayload.PreviousNodeGUID;

	newPayload.AllowedChildNodeGUIDs.Reset();
	for (UMounteaDialogueGraphNode* childNode : DialogueContext->AllowedChildNodes)
	{
		if (IsValid(childNode))
			newPayload.AllowedChildNodeGUIDs.Add(childNode->GetNodeGUID());
	}

	WriteContextPayload(MoveTemp(newPayload));
}

void UMounteaDialogueSession::ApplyRowStatePayload(const UMounteaDialogueContext* DialogueContext)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsValid(DialogueContext))
		return;

	FMounteaDialogueContextPayload newPayload = ContextPayload;

	if (DialogueContext->SessionGUID.IsValid())
		newPayload.SessionGUID = DialogueContext->SessionGUID;

	newPayload.DialogueParticipants = DialogueContext->DialogueParticipants;
	newPayload.ActiveDialogueParticipant = DialogueContext->ActiveDialogueParticipant;
	newPayload.ActiveDialogueRow = DialogueContext->ActiveDialogueRow;
	newPayload.ActiveDialogueRowDataIndex = DialogueContext->ActiveDialogueRowDataIndex;

	WriteContextPayload(MoveTemp(newPayload));
}

bool UMounteaDialogueSession::HandleSelectNode(UMounteaDialogueManager* Manager, const FGuid& SessionGUID, const FGuid& NodeGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Select Node")))
		return false;	

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Node Selected] Invalid Context!"));
		return false;
	}
	
	Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
		MounteaDialogueWidgetCommands::RemoveDialogueOptions,
		dialogueContext->SessionGUID,
		ContextPayload.ContextVersion,
		false
	});

	UMounteaDialogueGraphNode* selectedNode = nullptr;
	for (UMounteaDialogueGraphNode* childNode : dialogueContext->GetChildrenNodes())
	{
		if (IsValid(childNode) && childNode->GetNodeGUID() == NodeGUID)
		{
			selectedNode = childNode;
			break;
		}
	}

	if (!IsValid(selectedNode))
	{
		const FString errorMessage = FText::Format(FText::FromString("[Node Selected] Node with GUID {0} not found"), FText::FromString(NodeGUID.ToString())).ToString();
		LOG_ERROR(TEXT("%s"), *errorMessage);
		Manager->GetDialogueFailedEventHandle().Broadcast(errorMessage);
		return false;
	}

	TArray<UMounteaDialogueGraphNode*> allowedChildNodes = UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(selectedNode, dialogueContext);
	UMounteaDialogueTraversalStatics::SortNodes(allowedChildNodes);

	dialogueContext->SetDialogueContext(selectedNode, allowedChildNodes);
	dialogueContext->UpdateActiveDialogueRow(UMounteaDialogueTraversalStatics::GetSpeechData(selectedNode));
	dialogueContext->UpdateActiveDialogueRowDataIndex(0);
	const TScriptInterface<IMounteaDialogueParticipantInterface> newActiveParticipant = UMounteaDialogueTraversalStatics::ResolveActiveParticipant(dialogueContext);
	UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(dialogueContext, newActiveParticipant);

	ApplyNodeSwitchPayload(dialogueContext);

	Manager->GetDialogueNodeSelectedEventHandle().Broadcast(dialogueContext);
	IMounteaDialogueManagerInterface::Execute_PrepareNode(Manager);
	return true;
}

bool UMounteaDialogueSession::HandleSkipDialogueRow(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Skip Dialogue Row")))
		return false;

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Skip Dialogue Row] Invalid Dialogue Context!"));
		return false;
	}

	UWorld* world = Manager->GetWorld();
	if (!IsValid(world))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Skip Dialogue Row] World is not Valid!"));
		return false;
	}

	world->GetTimerManager().ClearTimer(Manager->GetDialogueRowTimerHandle());

	if (dialogueContext->ActiveDialogueParticipant.GetObject() && dialogueContext->ActiveDialogueParticipant.GetInterface())
	{
		dialogueContext->ActiveDialogueParticipant->Execute_SkipParticipantVoice(
			dialogueContext->ActiveDialogueParticipant.GetObject(),
			nullptr);
	}

	IMounteaDialogueManagerInterface::Execute_DialogueRowProcessed(Manager, true);
	return true;
}

bool UMounteaDialogueSession::HandleNodeProcessed(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Node Processed")))
		return false;

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Node Processed] Invalid Dialogue Context!"));
		return false;
	}

	if (!IsValid(dialogueContext->ActiveNode))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Node Processed] Invalid Active Node!"));
		return false;
	}

	Manager->GetDialogueNodeFinishedEventHandle().Broadcast(dialogueContext);
	dialogueContext->ActiveNode->CleanupNode();

	TArray<UMounteaDialogueGraphNode*> allowedChildrenNodes = UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(dialogueContext->ActiveNode, dialogueContext);
	UMounteaDialogueTraversalStatics::SortNodes(allowedChildrenNodes);

	if (allowedChildrenNodes.Num() == 0)
	{
		IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(Manager);
		return true;
	}

	UMounteaDialogueGraphNode** foundNodePtr = allowedChildrenNodes.FindByPredicate(
		[](const UMounteaDialogueGraphNode* Node)
		{
			return Node && Node->DoesAutoStart();
		});

	UMounteaDialogueGraphNode* newActiveNode = foundNodePtr ? *foundNodePtr : nullptr;
	if (newActiveNode != nullptr)
	{
		TArray<UMounteaDialogueGraphNode*> allowedChildNodes = UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(newActiveNode, dialogueContext);
		UMounteaDialogueTraversalStatics::SortNodes(allowedChildNodes);

		dialogueContext->SetDialogueContext(newActiveNode, allowedChildNodes);
		dialogueContext->UpdateActiveDialogueRow(UMounteaDialogueTraversalStatics::GetSpeechData(newActiveNode));
		dialogueContext->UpdateActiveDialogueRowDataIndex(0);
		const TScriptInterface<IMounteaDialogueParticipantInterface> newActiveParticipant = UMounteaDialogueTraversalStatics::ResolveActiveParticipant(dialogueContext);
		UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(dialogueContext, newActiveParticipant);

		Manager->GetDialogueNodeSelectedEventHandle().Broadcast(dialogueContext);
		ApplyNodeSwitchPayload(dialogueContext);
		Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
			MounteaDialogueWidgetCommands::RemoveDialogueOptions,
			dialogueContext->SessionGUID,
			ContextPayload.ContextVersion,
			false
		});
		IMounteaDialogueManagerInterface::Execute_PrepareNode(Manager);
		return true;
	}

	dialogueContext->AllowedChildNodes = allowedChildrenNodes;
	dialogueContext->UpdateActiveDialogueRow(FDialogueRow::Invalid());
	dialogueContext->UpdateActiveDialogueRowDataIndex(0);
	ApplyAllowedChildrenPayload(dialogueContext);

	Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
		MounteaDialogueWidgetCommands::AddDialogueOptions,
		dialogueContext->SessionGUID,
		ContextPayload.ContextVersion,
		false
	});

	return true;
}

bool UMounteaDialogueSession::HandleDialogueRowProcessed(UMounteaDialogueManager* Manager, const FGuid& SessionGUID, const bool bForceFinish)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Process Dialogue Row")))
		return false;

	if (IMounteaDialogueManagerInterface::Execute_GetManagerState(Manager) != EDialogueManagerState::EDMS_Active)
		return false;

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!IsValid(dialogueContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Process Dialogue Row] Invalid Dialogue Context!"));
		return false;
	}

	UWorld* world = Manager->GetWorld();
	if (!IsValid(world))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Process Dialogue Row] World is not Valid!"));
		return false;
	}

	world->GetTimerManager().ClearTimer(Manager->GetDialogueRowTimerHandle());

	const int32 currentIndex = dialogueContext->GetActiveDialogueRowDataIndex();
	const int32 nextIndex = currentIndex + 1;
	const FDialogueRow dialogueRow = dialogueContext->GetActiveDialogueRow();
	const TArray<FDialogueRowData> rowDataArray = dialogueRow.RowData;

	const bool bIsActiveRowValid = UMounteaDialogueTraversalStatics::IsDialogueRowValid(dialogueRow);
	const bool bDialogueRowDataValid = rowDataArray.IsValidIndex(nextIndex)
		&& UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(rowDataArray[nextIndex]);
	const ERowExecutionMode nextRowExecutionMode = bDialogueRowDataValid ? rowDataArray[nextIndex].RowExecutionBehaviour : ERowExecutionMode::EREM_Automatic;
	const ERowExecutionMode activeRowExecutionMode = rowDataArray.IsValidIndex(currentIndex) ? rowDataArray[currentIndex].RowExecutionBehaviour : ERowExecutionMode::EREM_Automatic;

	if (activeRowExecutionMode == ERowExecutionMode::EREM_AwaitInput && !bForceFinish)
		return true;

	Manager->GetDialogueRowFinishedEventHandle().Broadcast(dialogueContext);

	if (bIsActiveRowValid && bDialogueRowDataValid)
	{
		switch (nextRowExecutionMode)
		{
		case ERowExecutionMode::EREM_Automatic:
		case ERowExecutionMode::EREM_AwaitInput:
			dialogueContext->UpdateActiveDialogueRowDataIndex(nextIndex);
			Manager->GetDialogueContextUpdatedEventHande().Broadcast(dialogueContext);
			ApplyRowStatePayload(dialogueContext);
			Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
				MounteaDialogueWidgetCommands::ShowDialogueRow,
				dialogueContext->SessionGUID,
				ContextPayload.ContextVersion,
				false
			});
			IMounteaDialogueManagerInterface::Execute_ProcessDialogueRow(Manager);
			break;
		case ERowExecutionMode::EREM_Stopping:
			Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
				MounteaDialogueWidgetCommands::HideDialogueRow,
				dialogueContext->SessionGUID,
				ContextPayload.ContextVersion,
				false
			});
			Manager->GetDialogueNodeFinishedEventHandle().Broadcast(dialogueContext);
			break;
		case ERowExecutionMode::Default:
			break;
		}

		return true;
	}

	Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
		MounteaDialogueWidgetCommands::HideDialogueRow,
		dialogueContext->SessionGUID,
		ContextPayload.ContextVersion,
		false
	});
	IMounteaDialogueManagerInterface::Execute_NodeProcessed(Manager);
	return true;
}

bool UMounteaDialogueSession::HandleProcessDialogueRow(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Process Dialogue Row")))
		return false;

	UWorld* world = Manager->GetWorld();
	if (!IsValid(world))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Process Dialogue Row] World is not Valid!"));
		return false;
	}

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Process Dialogue Row] Invalid Dialogue Context!"));
		return false;
	}

	if (!dialogueContext->ActiveNode->IsA(UMounteaDialogueGraphNode_DialogueNodeBase::StaticClass()))
		return true;

	const int32 activeIndex = dialogueContext->GetActiveDialogueRowDataIndex();
	const FDialogueRow row = dialogueContext->GetActiveDialogueRow();
	if (!row.RowData.IsValidIndex(activeIndex))
	{
		IMounteaDialogueManagerInterface::Execute_DialogueRowProcessed(Manager, false);
		return true;
	}

	const FDialogueRowData rowData = row.RowData[activeIndex];
	if (!UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(rowData))
	{
		IMounteaDialogueManagerInterface::Execute_DialogueRowProcessed(Manager, false);
		return true;
	}

	Manager->GetDialogueRowStartedEventHandle().Broadcast(dialogueContext);
	Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
		MounteaDialogueWidgetCommands::ShowDialogueRow,
		dialogueContext->SessionGUID,
		ContextPayload.ContextVersion,
		false
	});
	if (dialogueContext->ActiveDialogueParticipant.GetObject() && dialogueContext->ActiveDialogueParticipant.GetInterface())
	{
		dialogueContext->ActiveDialogueParticipant->Execute_PlayParticipantVoice(
			dialogueContext->ActiveDialogueParticipant.GetObject(),
			rowData.RowSound);
	}

	FTimerDelegate delegate;
	delegate.BindUObject(Manager, &UMounteaDialogueManager::DialogueRowProcessed_Implementation, false);
	world->GetTimerManager().SetTimer(
		Manager->GetDialogueRowTimerHandle(),
		delegate,
		UMounteaDialogueTraversalStatics::GetRowDuration(rowData),
		false);

	return true;
}

bool UMounteaDialogueSession::HandlePrepareNode(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Prepare Node")))
		return false;

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Prepare Node] Invalid Dialogue Context!"));
		return false;
	}

	if (!IsValid(dialogueContext->ActiveNode))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Prepare Node] No Active Node!"));
		return false;
	}

	const TScriptInterface<IMounteaDialogueParticipantInterface> newActiveParticipant = UMounteaDialogueTraversalStatics::ResolveActiveParticipant(dialogueContext);
	UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(dialogueContext, newActiveParticipant);
	dialogueContext->ActiveNode->PreProcessNode(Manager);
	return true;
}

bool UMounteaDialogueSession::HandleNodePrepared(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Node Prepared")))
		return false;

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Node Prepared] Invalid Dialogue Context!"));
		return false;
	}

	AddTraversedNode(dialogueContext->ActiveNode);
	dialogueContext->AddTraversedNode(dialogueContext->ActiveNode);
 	IMounteaDialogueManagerInterface::Execute_ProcessNode(Manager);
	return true;
}

bool UMounteaDialogueSession::HandleProcessNode(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Process Node")))
		return false;

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager);
	if (!IsValid(dialogueContext) || !IsValid(dialogueContext->ActiveNode))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Process Node] Invalid Context or Active Node!"));
		return false;
	}

	UMounteaDialogueGraphNode* processingNode = dialogueContext->ActiveNode;
	if (processingNode->Implements<UMounteaDialogueSpeechDataInterface>()
		&& (!dialogueContext->ActiveDialogueParticipant.GetObject()
			|| !dialogueContext->ActiveDialogueParticipant.GetInterface()))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[Process Node] Speech node requires a valid Active Dialogue Participant!"));
		return false;
	}

	processingNode->ProcessNode(Manager);

	if (dialogueContext->ActiveNode != processingNode)
	{
		ApplyNodeSwitchPayload(dialogueContext);
		return true;
	}

	ApplyRowStatePayload(dialogueContext);
	Manager->GetDialogueNodeStartedEventHandle().Broadcast(dialogueContext);

	if (!processingNode->Implements<UMounteaDialogueSpeechDataInterface>())
	{
		Manager->Client_DispatchUISignal(FMounteaDialogueUISignal{
			MounteaDialogueWidgetCommands::HideDialogueRow,
			dialogueContext->SessionGUID,
			ContextPayload.ContextVersion,
			false
		});
	}

	if (processingNode->Implements<UMounteaDialogueSpeechDataInterface>()
		&& dialogueContext->ActiveDialogueParticipant.GetObject()
		&& dialogueContext->ActiveDialogueParticipant.GetInterface())
		dialogueContext->ActiveDialogueParticipant->GetOnParticipantBecomeActiveEventHandle().Broadcast(true);

	IMounteaDialogueManagerInterface::Execute_ProcessDialogueRow(Manager);
	return true;
}

bool UMounteaDialogueSession::HandleCloseDialogue(UMounteaDialogueManager* Manager, const FGuid& SessionGUID)
{
	if (!IsSessionRequestValid(Manager, SessionGUID, TEXT("Close Dialogue")))
		return false;

	IMounteaDialogueManagerInterface::Execute_RequestCloseDialogue(Manager);
	return true;
}


