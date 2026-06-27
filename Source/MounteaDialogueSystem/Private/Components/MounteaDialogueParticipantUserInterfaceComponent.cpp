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

#include "Components/MounteaDialogueParticipantUserInterfaceComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/MounteaDialogueSession.h"
#include "TimerManager.h"
#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueContextPayload.h"
#include "Helpers/MounteaDialogueManagerStatics.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Interfaces/HUD/MounteaDialogueUIBaseInterface.h"
#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "Settings/MounteaDialogueConfiguration.h"
#include "Settings/MounteaDialogueSystemSettings.h"
#include "Subsystem/MounteaDialogueViewportHUDSubsystem.h"
#include "Subsystem/MounteaDialogueWorldSubsystem.h"

static constexpr uint8 UICompViewMode_None    = 0;
static constexpr uint8 UICompViewMode_Row     = 1;
static constexpr uint8 UICompViewMode_Options = 2;
static constexpr uint8 UICompViewMode_Neutral = 3;
static constexpr uint8 UICompViewMode_Closed  = 4;

UMounteaDialogueParticipantUserInterfaceComponent::UMounteaDialogueParticipantUserInterfaceComponent()
{
	bAutoActivate = true;

	SetIsReplicatedByDefault(false);
	SetActiveFlag(true);

	PrimaryComponentTick.bStartWithTickEnabled = false;

	ComponentTags.Add(FName("Mountea"));
	ComponentTags.Add(FName("Dialogue"));
	ComponentTags.Add(FName("Participant Interface"));
}

void UMounteaDialogueParticipantUserInterfaceComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMounteaDialogueParticipantUserInterfaceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ParentManager.GetObject())
		Execute_UnbindFromManager(this);

	Super::EndPlay(EndPlayReason);
}

// --- Manager Binding ----------------------------------------------------------

void UMounteaDialogueParticipantUserInterfaceComponent::SetParentManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager)
{
	ParentManager = NewManager;
}

TScriptInterface<IMounteaDialogueManagerInterface>
UMounteaDialogueParticipantUserInterfaceComponent::GetParentManager_Implementation() const
{
	return ParentManager;
}

void UMounteaDialogueParticipantUserInterfaceComponent::BindToManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (!IsValid(Manager.GetObject()))
	{
		LOG_WARNING(TEXT("[MounteaDialogueParticipantUIComponent] BindToManager called with invalid Manager."))
		return;
	}

	// Unbind previous manager first to prevent double-binding.
	if (ParentManager.GetObject())
		Execute_UnbindFromManager(this);

	Execute_SetParentManager(this, Manager);
	Manager->GetDialogueUISignalEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnUISignalReceived);
	BindLifecycleDelegates(Manager);
}

void UMounteaDialogueParticipantUserInterfaceComponent::UnbindFromManager_Implementation()
{
	if (!ParentManager.GetObject())
		return;

	ParentManager->GetDialogueUISignalEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnUISignalReceived);
	UnbindLifecycleDelegates(ParentManager);
	ParentManager = nullptr;
}

// --- UI Target ----------------------------------------------------------------

void UMounteaDialogueParticipantUserInterfaceComponent::SetUserInterface_Implementation(UObject* NewUserInterface)
{
	if (NewUserInterface != UserInterface)
		UserInterface = NewUserInterface;
}

UObject* UMounteaDialogueParticipantUserInterfaceComponent::GetUserInterface_Implementation() const
{
	return UserInterface;
}

// --- Screen UI Lifecycle ------------------------------------------------------

bool UMounteaDialogueParticipantUserInterfaceComponent::CreateDialogueUI_Implementation(FString& Message)
{
	if (!UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner()))
	{
		Message = TEXT("ShouldExecuteCosmetics returned false — server-side component, skipping UI creation.");
		return true;
	}

	// Already exists — re-bind events and treat as success.
	if (IsValid(UserInterface))
	{
		IMounteaDialogueUIBaseInterface::Execute_BindEvents(UserInterface);
		return true;
	}

	const UMounteaDialogueSystemSettings* settings = GetDefault<UMounteaDialogueSystemSettings>();
	TSubclassOf<UUserWidget> widgetClass = settings ? settings->GetDefaultDialogueWidget().LoadSynchronous() : nullptr;
	if (!widgetClass)
	{
		Message = TEXT("No DefaultDialogueWidgetClass set in MounteaDialogueConfiguration.");
		LOG_ERROR(TEXT("[MounteaDialogueParticipantUIComponent] %s"), *Message)
		return false;
	}

	APlayerController* playerController = UMounteaDialogueSystemBFC::GetOwnerPlayerController(GetOwner());
	if (!playerController)
	{
		Message = TEXT("Cannot find a local PlayerController from the owner chain.");
		LOG_ERROR(TEXT("[MounteaDialogueParticipantUIComponent] %s"), *Message)
		return false;
	}

	UUserWidget* newWidget = CreateWidget<UUserWidget>(playerController, widgetClass);
	if (!newWidget || !newWidget->Implements<UMounteaDialogueWBPInterface>())
	{
		Message = !newWidget
			? TEXT("Failed to spawn dialogue widget.")
			: TEXT("Spawned widget does not implement IMounteaDialogueWBPInterface.");
		LOG_ERROR(TEXT("[MounteaDialogueParticipantUIComponent] %s"), *Message)
		return false;
	}

	// Broadcast the newly created widget so we can either:
	// * show it using AddToPlayerScreen
	// * show it using HUD Class
	OnDialogueWidgetCreated.Broadcast(newWidget);

	if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
	{
		if (UMounteaDialogueViewportHUDSubsystem* localPlayerSubsystem = localPlayer->GetSubsystem<UMounteaDialogueViewportHUDSubsystem>())
			IMounteaDialogueHUDClassInterface::Execute_AddChildWidgetToViewport(localPlayerSubsystem, newWidget, 0, FAnchors(0,0,1,1), FMargin());
		
		else
			newWidget->AddToPlayerScreen();
	}
	else
		newWidget->AddToPlayerScreen();

	Execute_SetUserInterface(this, newWidget);
	IMounteaDialogueUIBaseInterface::Execute_BindEvents(UserInterface);

	return Execute_UpdateDialogueUI(this, Message, MounteaDialogueWidgetCommands::CreateDialogueWidget);
}

bool UMounteaDialogueParticipantUserInterfaceComponent::UpdateDialogueUI_Implementation(FString& Message, const FString& Command)
{
	if (!UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner()))
		return true;

	if (!IsValid(UserInterface))
	{
		Execute_CreateDialogueUI(this, Message);
		if (!IsValid(UserInterface))
			return false;
	}

	IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(UserInterface, ParentManager, Command);
	return true;
}

bool UMounteaDialogueParticipantUserInterfaceComponent::CloseDialogueUI_Implementation()
{
	if (!IsValid(UserInterface))
		return false;

	if (UUserWidget* userWidget = Cast<UUserWidget>(UserInterface))
	{
		if (APlayerController* playerController = UMounteaDialogueSystemBFC::GetOwnerPlayerController(GetOwner()))
		{
			if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
			{
				if (UMounteaDialogueViewportHUDSubsystem* localPlayerSubsystem = localPlayer->GetSubsystem<UMounteaDialogueViewportHUDSubsystem>())
					IMounteaDialogueHUDClassInterface::Execute_RemoveChildWidgetFromViewport(localPlayerSubsystem, userWidget);
			}
		}
	}

	IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(UserInterface, ParentManager, MounteaDialogueWidgetCommands::CloseDialogueWidget);
	IMounteaDialogueUIBaseInterface::Execute_UnbindEvents(UserInterface);
	UserInterface = nullptr;
	return true;
}

void UMounteaDialogueParticipantUserInterfaceComponent::ExecuteWidgetCommand_Implementation(const FString& Command)
{
	FString resultMsg;
	Execute_UpdateDialogueUI(this, resultMsg, Command);
}

// --- Input Forwarding ---------------------------------------------------------

void UMounteaDialogueParticipantUserInterfaceComponent::RequestSelectNode_Implementation(const FGuid& NodeGuid)
{
	BeginSelectPrediction(NodeGuid);
	UMounteaDialogueManagerStatics::SelectNode(ParentManager, NodeGuid);
}

void UMounteaDialogueParticipantUserInterfaceComponent::RequestSkipDialogueRow_Implementation()
{
	UMounteaDialogueManagerStatics::SkipDialogueRow(ParentManager);
}

void UMounteaDialogueParticipantUserInterfaceComponent::RequestCloseDialogue_Implementation()
{
	if (ParentManager.GetObject())
	{
		const UMounteaDialogueContext* dialogueContext = ParentManager->Execute_GetDialogueContext(ParentManager.GetObject());
		const FGuid sessionGuid = IsValid(dialogueContext) ? dialogueContext->SessionGUID : FGuid();
		BeginClosePrediction(sessionGuid);
	}
	UMounteaDialogueManagerStatics::RequestCloseDialogue(ParentManager);
}

void UMounteaDialogueParticipantUserInterfaceComponent::RequestProcessDialogueRow_Implementation()
{
	UMounteaDialogueManagerStatics::ProcessDialogueRow(ParentManager);
}

// --- Signal Dispatch ----------------------------------------------------------

void UMounteaDialogueParticipantUserInterfaceComponent::DispatchUISignal_Implementation(const FMounteaDialogueUISignal& Signal)
{
	// Sentinel from Client_ClearUISignals: flush all pending signals for this session.
	if (Signal.RequiredContextVersion == INT32_MAX)
	{
		PendingUISignals.RemoveAll([&](const FMounteaDialogueUISignal& Queued)
		{
			return Queued.SessionGUID == Signal.SessionGUID;
		});
		return;
	}

	// Session changed — discard stale queue.
	if (LastDispatchedSessionGUID.IsValid() && Signal.SessionGUID != LastDispatchedSessionGUID)
	{
		PendingUISignals.Empty();
		LastDispatchedContextVersion = 0;
	}

	if (Signal.RequiredContextVersion > LastDispatchedContextVersion)
	{
		PendingUISignals.Add(Signal);
		return;
	}

	ExecuteUISignal(Signal);
}

void UMounteaDialogueParticipantUserInterfaceComponent::ExecuteUISignal(const FMounteaDialogueUISignal& Signal)
{
	LastDispatchedContextVersion = FMath::Max(LastDispatchedContextVersion, Signal.RequiredContextVersion);
	LastDispatchedSessionGUID = Signal.SessionGUID;

	if (Signal.bForceReconcile)
	{
		// Pull current payload from session and perform full UI state reconcile.
		const UWorld* world = GetWorld();
		const UMounteaDialogueWorldSubsystem* subsystem = world ? world->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
		const UMounteaDialogueSession* session = subsystem ? subsystem->GetGameStateSession() : nullptr;
		if (session)
			ReconcileFromPayload(session->GetContextPayload());
		return;
	}

	FString signalMessage;
	if (Signal.Command == MounteaDialogueWidgetCommands::CreateDialogueWidget)
		Execute_CreateDialogueUI(this, signalMessage);
	else if (Signal.Command == MounteaDialogueWidgetCommands::CloseDialogueWidget)
		Execute_CloseDialogueUI(this);
	else if (!Signal.Command.IsEmpty())
		Execute_UpdateDialogueUI(this, signalMessage, Signal.Command);
}

void UMounteaDialogueParticipantUserInterfaceComponent::DrainPendingSignals(int32 CurrentVersion, const FGuid& SessionGUID)
{
	// Drop signals from a different session.
	PendingUISignals.RemoveAll([&](const FMounteaDialogueUISignal& Queued)
	{
		return Queued.SessionGUID != SessionGUID;
	});

	// Collect signals that are now ready, sorted ascending by version.
	TArray<FMounteaDialogueUISignal> ready;
	for (int32 i = PendingUISignals.Num() - 1; i >= 0; --i)
	{
		if (PendingUISignals[i].RequiredContextVersion <= CurrentVersion)
		{
			ready.Add(PendingUISignals[i]);
			PendingUISignals.RemoveAt(i);
		}
	}

	ready.Sort([](const FMounteaDialogueUISignal& A, const FMounteaDialogueUISignal& B)
	{
		return A.RequiredContextVersion < B.RequiredContextVersion;
	});

	for (const FMounteaDialogueUISignal& signal : ready)
		ExecuteUISignal(signal);
}

// --- Private helpers ----------------------------------------------------------

void UMounteaDialogueParticipantUserInterfaceComponent::OnUISignalReceived(const FMounteaDialogueUISignal& Signal)
{
	Execute_DispatchUISignal(this, Signal);
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnContextVersionUpdated(UMounteaDialogueContext* Context)
{
	if (!IsValid(Context))
		return;

	const UWorld* world = GetWorld();
	if (!world)
		return;

	const UMounteaDialogueWorldSubsystem* subsystem = world->GetSubsystem<UMounteaDialogueWorldSubsystem>();
	if (!subsystem)
		return;

	const UMounteaDialogueSession* session = subsystem->GetGameStateSession();
	if (!session)
		return;

	const FMounteaDialogueContextPayload& payload = session->GetContextPayload();
	const int32 currentVersion = payload.ContextVersion;

	LastDispatchedContextVersion = FMath::Max(LastDispatchedContextVersion, currentVersion);
	LastDispatchedSessionGUID = Context->SessionGUID;

	ResolvePredictionFromPayload(payload);

	if (!PendingUISignals.IsEmpty())
		DrainPendingSignals(currentVersion, Context->SessionGUID);
}

void UMounteaDialogueParticipantUserInterfaceComponent::BindLifecycleDelegates(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	Manager->GetDialogueContextUpdatedEventHande().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnContextVersionUpdated);
	Manager->GetDialogueStartedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueStarted);
	Manager->GetDialogueClosedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueClosed);
	Manager->GetDialogueFailedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueFailed);
	Manager->GetDialogueNodeStartedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeStarted);
	Manager->GetDialogueNodeFinishedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeFinished);
	Manager->GetDialogueNodeSelectedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeSelected);
	Manager->GetDialogueRowStartedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowStarted);
	Manager->GetDialogueRowFinishedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowFinished);
}

void UMounteaDialogueParticipantUserInterfaceComponent::UnbindLifecycleDelegates(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	Manager->GetDialogueContextUpdatedEventHande().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnContextVersionUpdated);
	Manager->GetDialogueStartedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueStarted);
	Manager->GetDialogueClosedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueClosed);
	Manager->GetDialogueFailedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueFailed);
	Manager->GetDialogueNodeStartedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeStarted);
	Manager->GetDialogueNodeFinishedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeFinished);
	Manager->GetDialogueNodeSelectedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeSelected);
	Manager->GetDialogueRowStartedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowStarted);
	Manager->GetDialogueRowFinishedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowFinished);
}

// --- Reconcile helpers --------------------------------------------------------

void UMounteaDialogueParticipantUserInterfaceComponent::ApplyWidgetCommand(const FString& Command)
{
	if (!UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner()))
		return;

	if (Command.IsEmpty())
		return;

	if (IsValid(UserInterface))
		IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(UserInterface, ParentManager, Command);
}

void UMounteaDialogueParticipantUserInterfaceComponent::ResetClientSyncCaches(const FGuid& SessionGUID)
{
	LastClientSyncSessionGUID = SessionGUID;
	LastAppliedPayloadCommandVersion = 0;
	LastReconciledRowGUID.Invalidate();
	LastReconciledRowIndex = INDEX_NONE;
	LastReconciledOptionsHash = 0;
	LastReconciledViewMode = UICompViewMode_None;
	LastAppliedCommand.Empty();
}

void UMounteaDialogueParticipantUserInterfaceComponent::ReconcileFromPayload(const FMounteaDialogueContextPayload& Payload)
{
	if (!UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner()))
		return;

	if (LastClientSyncSessionGUID != Payload.SessionGUID)
		ResetClientSyncCaches(Payload.SessionGUID);

	if (!ParentManager.GetObject())
		return;

	const EDialogueManagerState managerState = ParentManager->Execute_GetManagerState(ParentManager.GetObject());

	if (managerState == EDialogueManagerState::EDMS_Active && !IsValid(UserInterface))
	{
		FString resultMsg;
		Execute_CreateDialogueUI(this, resultMsg);
	}

	if (managerState != EDialogueManagerState::EDMS_Active)
	{
		if (LastReconciledViewMode != UICompViewMode_Closed)
			ApplyWidgetCommand(MounteaDialogueWidgetCommands::CloseDialogueWidget);

		LastReconciledViewMode = UICompViewMode_Closed;
		return;
	}

	const bool bHasOptions = Payload.AllowedChildNodeGUIDs.Num() > 0;
	const bool bCanRenderRow = UMounteaDialogueTraversalStatics::IsDialogueRowValid(Payload.ActiveDialogueRow)
		&& Payload.ActiveDialogueRow.RowData.IsValidIndex(Payload.ActiveDialogueRowDataIndex)
		&& UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(
			Payload.ActiveDialogueRow.RowData[Payload.ActiveDialogueRowDataIndex]);
	const bool bShouldShowOptions = bHasOptions && !bCanRenderRow;

	if (bCanRenderRow)
	{
		const bool bNeedsRefresh = LastReconciledViewMode != UICompViewMode_Row
			|| LastReconciledRowGUID != Payload.ActiveDialogueRow.RowGUID
			|| LastReconciledRowIndex != Payload.ActiveDialogueRowDataIndex;

		if (bNeedsRefresh)
		{
			ApplyWidgetCommand(MounteaDialogueWidgetCommands::RemoveDialogueOptions);
			ApplyWidgetCommand(MounteaDialogueWidgetCommands::ShowDialogueRow);
		}

		LastReconciledRowGUID = Payload.ActiveDialogueRow.RowGUID;
		LastReconciledRowIndex = Payload.ActiveDialogueRowDataIndex;
		LastReconciledOptionsHash = 0;
		LastReconciledViewMode = UICompViewMode_Row;
		return;
	}

	if (bShouldShowOptions)
	{
		uint32 optionsHash = 0;
		for (const FGuid& guid : Payload.AllowedChildNodeGUIDs)
			optionsHash = HashCombine(optionsHash, GetTypeHash(guid));

		const bool bNeedsRefresh = LastReconciledViewMode != UICompViewMode_Options
			|| LastReconciledOptionsHash != optionsHash;

		if (bNeedsRefresh)
		{
			ApplyWidgetCommand(MounteaDialogueWidgetCommands::HideDialogueRow);
			ApplyWidgetCommand(MounteaDialogueWidgetCommands::AddDialogueOptions);
		}

		LastReconciledOptionsHash = optionsHash;
		LastReconciledRowGUID.Invalidate();
		LastReconciledRowIndex = INDEX_NONE;
		LastReconciledViewMode = UICompViewMode_Options;
		return;
	}

	if (LastReconciledViewMode != UICompViewMode_Neutral)
	{
		ApplyWidgetCommand(MounteaDialogueWidgetCommands::HideDialogueRow);
		ApplyWidgetCommand(MounteaDialogueWidgetCommands::RemoveDialogueOptions);
	}

	LastReconciledRowGUID.Invalidate();
	LastReconciledRowIndex = INDEX_NONE;
	LastReconciledOptionsHash = 0;
	LastReconciledViewMode = UICompViewMode_Neutral;
}

// --- Client prediction --------------------------------------------------------

bool UMounteaDialogueParticipantUserInterfaceComponent::IsPredictionEnabled() const
{
	const UMounteaDialogueSystemSettings* settings = GetDefault<UMounteaDialogueSystemSettings>();
	return bPredictionEnabled
		&& settings && settings->IsClientPredictionEnabled()
		&& !UMounteaDialogueManagerStatics::IsServer(GetOwner())
		&& UMounteaDialogueSystemBFC::ShouldExecuteCosmetics(GetOwner());
}

void UMounteaDialogueParticipantUserInterfaceComponent::BeginSelectPrediction(const FGuid& NodeGuid)
{
	if (!IsPredictionEnabled())
		return;

	if (!ParentManager.GetObject())
		return;

	const UMounteaDialogueContext* dialogueContext = ParentManager->Execute_GetDialogueContext(ParentManager.GetObject());
	if (!IsValid(dialogueContext) || !dialogueContext->SessionGUID.IsValid())
		return;

	ClearPredictionState();

	PendingPredictionType = EDialogueClientPredictionType::Select;
	PendingPredictionSessionGUID = dialogueContext->SessionGUID;
	PendingPredictionNodeGUID = NodeGuid;

	const UWorld* world = GetWorld();
	const UMounteaDialogueWorldSubsystem* subsystem = world ? world->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	const UMounteaDialogueSession* session = subsystem ? subsystem->GetGameStateSession() : nullptr;
	PendingPredictionStartContextVersion = session ? session->GetContextPayload().ContextVersion : 0;

	const UMounteaDialogueSystemSettings* settings = GetDefault<UMounteaDialogueSystemSettings>();
	const float timeout = settings ? settings->GetClientPredictionTimeoutSeconds() : 0.75f;
	if (world)
		world->GetTimerManager().SetTimer(PendingPredictionHandle, this,
			&UMounteaDialogueParticipantUserInterfaceComponent::OnPredictionTimeout, timeout, false);

	ApplyPredictedUICommand(MounteaDialogueWidgetCommands::RemoveDialogueOptions);
	ApplyPredictedUICommand(MounteaDialogueWidgetCommands::HideDialogueRow);
}

void UMounteaDialogueParticipantUserInterfaceComponent::BeginClosePrediction(const FGuid& SessionGuid)
{
	if (!IsPredictionEnabled())
		return;

	if (!SessionGuid.IsValid())
		return;

	ClearPredictionState();

	PendingPredictionType = EDialogueClientPredictionType::Close;
	PendingPredictionSessionGUID = SessionGuid;

	const UWorld* world = GetWorld();
	const UMounteaDialogueWorldSubsystem* subsystem = world ? world->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	const UMounteaDialogueSession* session = subsystem ? subsystem->GetGameStateSession() : nullptr;
	PendingPredictionStartContextVersion = session ? session->GetContextPayload().ContextVersion : 0;

	const UMounteaDialogueSystemSettings* settings = GetDefault<UMounteaDialogueSystemSettings>();
	const float timeout = settings ? settings->GetClientPredictionTimeoutSeconds() : 0.75f;
	if (world)
		world->GetTimerManager().SetTimer(PendingPredictionHandle, this,
			&UMounteaDialogueParticipantUserInterfaceComponent::OnPredictionTimeout, timeout, false);

	ApplyPredictedUICommand(MounteaDialogueWidgetCommands::CloseDialogueWidget);

	if (!bPredictedCloseEventFired || PredictedCloseSessionGUID != SessionGuid)
	{
		bPredictedCloseEventFired = true;
		PredictedCloseSessionGUID = SessionGuid;
	}
}

void UMounteaDialogueParticipantUserInterfaceComponent::ResolvePredictionFromPayload(const FMounteaDialogueContextPayload& Payload)
{
	if (PendingPredictionType == EDialogueClientPredictionType::None)
		return;

	if (Payload.SessionGUID != PendingPredictionSessionGUID)
	{
		ClearPredictionState();
		return;
	}

	if (PendingPredictionType == EDialogueClientPredictionType::Select
		&& Payload.ContextVersion > PendingPredictionStartContextVersion)
	{
		ClearPredictionState();
	}

	if (PendingPredictionType == EDialogueClientPredictionType::Close
		&& !ParentManager.GetObject())
	{
		ClearPredictionState();
	}
}

void UMounteaDialogueParticipantUserInterfaceComponent::RollbackPrediction(const FString& Reason)
{
	if (PendingPredictionType == EDialogueClientPredictionType::None)
		return;

	LOG_WARNING(TEXT("[MounteaDialogueParticipantUIComponent] Rolling back '%s' prediction: %s"),
		PendingPredictionType == EDialogueClientPredictionType::Select ? TEXT("Select") : TEXT("Close"),
		*Reason)

	const UWorld* world = GetWorld();
	const UMounteaDialogueWorldSubsystem* subsystem = world ? world->GetSubsystem<UMounteaDialogueWorldSubsystem>() : nullptr;
	if (subsystem)
	{
		if (const UMounteaDialogueSession* session = subsystem->GetGameStateSession())
		{
			const FMounteaDialogueContextPayload& payload = session->GetContextPayload();
			if (payload.IsValid())
				ReconcileFromPayload(payload);
		}
	}

	if (PendingPredictionType == EDialogueClientPredictionType::Close)
	{
		bPredictedCloseEventFired = false;
		PredictedCloseSessionGUID.Invalidate();
	}

	ClearPredictionState();
}

void UMounteaDialogueParticipantUserInterfaceComponent::ClearPredictionState()
{
	if (const UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(PendingPredictionHandle);

	PendingPredictionType = EDialogueClientPredictionType::None;
	PendingPredictionSessionGUID.Invalidate();
	PendingPredictionNodeGUID.Invalidate();
	PendingPredictionStartContextVersion = 0;
}

void UMounteaDialogueParticipantUserInterfaceComponent::ApplyPredictedUICommand(const FString& Command)
{
	if (!IsPredictionEnabled())
		return;

	FMounteaDialogueUISignal signal;
	signal.Command = Command;
	if (ParentManager.GetObject())
	{
		const UMounteaDialogueContext* dialogueContext = ParentManager->Execute_GetDialogueContext(ParentManager.GetObject());
		signal.SessionGUID = IsValid(dialogueContext) ? dialogueContext->SessionGUID : FGuid();
	}
	signal.RequiredContextVersion = 0;
	signal.bForceReconcile = false;
	ExecuteUISignal(signal);
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnPredictionTimeout()
{
	if (PendingPredictionType == EDialogueClientPredictionType::None)
		return;

	RollbackPrediction(TEXT("Timeout waiting for authoritative update."));
}

// --- Lifecycle delegate handlers ----------------------------------------------
// These fire locally on whichever process the manager runs on.
// On a dedicated server the manager runs server-side only, so these never fire
// on the client — Client_DispatchUISignal is the authoritative UI delivery path.
// On a listen server / local play both the delegate AND the signal fire in-process;
// all UI methods are idempotent so double-firing is safe.

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueStarted(UMounteaDialogueContext* Context)
{
	// Fallback creation for listen-server / local play.
	// On a dedicated-server client this is a no-op because the delegate never fires there.
	// CreateDialogueUI guards against double-creation via IsValid(UserInterface).
	FString message;
	Execute_CreateDialogueUI(this, message);
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueClosed(UMounteaDialogueContext* Context)
{
	// Ensure UI is torn down even if the close signal was missed or arrived out-of-order.
	// CloseDialogueUI checks IsValid(UserInterface) so safe to call redundantly.
	Execute_CloseDialogueUI(this);
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueFailed(const FString& ErrorMessage)
{
	// Dialogue failed — clean up any open UI unconditionally.
	Execute_CloseDialogueUI(this);
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeStarted(UMounteaDialogueContext* Context)
{
	// UI updates for node transitions are carried by FMounteaDialogueUISignal commands
	// dispatched from the server via Client_DispatchUISignal. No action needed here.
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeFinished(UMounteaDialogueContext* Context)
{
	// See OnDialogueNodeStarted — signals drive all UI state changes.
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeSelected(UMounteaDialogueContext* Context)
{
	// Node selection confirmation arrives via OnContextVersionUpdated, which drains
	// PendingUISignals. No additional UI action required here.
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowStarted(UMounteaDialogueContext* Context)
{
	// Row display is handled by ReconcileFromPayload triggered from OnContextVersionUpdated.
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowFinished(UMounteaDialogueContext* Context)
{
	// Row completion state is reconciled from the next payload version.
}
