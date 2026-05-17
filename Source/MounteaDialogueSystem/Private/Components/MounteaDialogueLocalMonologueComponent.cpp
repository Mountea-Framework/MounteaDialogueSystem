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

#include "Components/MounteaDialogueLocalMonologueComponent.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueContextStatics.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueManagerStatics.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"
#include "Interfaces/Nodes/MounteaDialogueSpeechDataInterface.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Settings/MounteaDialogueSystemSettings.h"
#include "Subsystem/MounteaDialogueLocalMonologueSubsystem.h"
#include "TimerManager.h"

UMounteaDialogueLocalMonologueComponent::UMounteaDialogueLocalMonologueComponent()
{
	SetIsReplicatedByDefault(false);
	SetIsReplicated(false);
	DialogueManagerType = EDialogueManagerType::EDMT_EnvironmentDialogue;
}

void UMounteaDialogueLocalMonologueComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	ManagerState = Execute_GetDefaultManagerState(this);
	OnDialogueFailed.AddUniqueDynamic(this, &UMounteaDialogueLocalMonologueComponent::HandleLocalDialogueFailed);

	if (UMounteaDialogueLocalMonologueSubsystem* localSubsystem = ResolveLocalMonologueSubsystem())
		localSubsystem->RegisterMonologueComponent(this);
}

void UMounteaDialogueLocalMonologueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMounteaDialogueLocalMonologueSubsystem* localSubsystem = ResolveLocalMonologueSubsystem())
		localSubsystem->UnregisterMonologueComponent(this);

	OnDialogueStartRequestedResult.Clear();
	OnDialogueFailed.Clear();
	OnDialogueStarted.Clear();
	OnDialogueClosed.Clear();
	OnDialogueNodeSelected.Clear();
	OnDialogueNodeFinished.Clear();
	OnDialogueRowStarted.Clear();
	OnDialogueRowFinished.Clear();

	UActorComponent::EndPlay(EndPlayReason);
}

void UMounteaDialogueLocalMonologueComponent::HandleLocalDialogueFailed(const FString& ErrorMessage)
{
	LOG_ERROR(TEXT("[Local Monologue Failed] %s"), *ErrorMessage)
	SetManagerState(DefaultManagerState);
}

bool UMounteaDialogueLocalMonologueComponent::RequestStartLocalMonologue(const FDialogueStartRequest& Request, FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (!Execute_CanStartDialogue(this))
	{
		OutErrorMessage = TEXT("[RequestStartLocalMonologue] Cannot start monologue — manager is not enabled.");
		OnDialogueStartRequestedResult.Broadcast(false, OutErrorMessage);
		OnDialogueFailed.Broadcast(OutErrorMessage);
		return false;
	}

	UMounteaDialogueLocalMonologueSubsystem* localSubsystem = ResolveLocalMonologueSubsystem();
	if (!localSubsystem)
	{
		OutErrorMessage = TEXT("[RequestStartLocalMonologue] Missing local monologue subsystem.");
		OnDialogueStartRequestedResult.Broadcast(false, OutErrorMessage);
		OnDialogueFailed.Broadcast(OutErrorMessage);
		return false;
	}

	if (!localSubsystem->TryStartMonologue(this, Request, OutErrorMessage))
	{
		if (OutErrorMessage.IsEmpty())
			OutErrorMessage = TEXT("[RequestStartLocalMonologue] Failed to start local monologue.");
		OnDialogueStartRequestedResult.Broadcast(false, OutErrorMessage);
		OnDialogueFailed.Broadcast(OutErrorMessage);
		return false;
	}

	return true;
}

void UMounteaDialogueLocalMonologueComponent::RequestCloseLocalMonologue()
{
	if (!IsLocalMonologueActive())
		return;

	Execute_RequestCloseDialogue(this);
}

bool UMounteaDialogueLocalMonologueComponent::IsLocalMonologueActive() const
{
	return ManagerState == EDialogueManagerState::EDMS_Active
		&& IsValid(DialogueContext)
		&& DialogueContext->SessionGUID.IsValid();
}

FGuid UMounteaDialogueLocalMonologueComponent::GetActiveLocalMonologueSessionGUID() const
{
	return IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();
}

bool UMounteaDialogueLocalMonologueComponent::StartLocalMonologueInternal(const FDialogueStartRequest& Request, FString& OutErrorMessage)
{
	OutErrorMessage.Empty();

	if (!Request.IsValid())
	{
		OutErrorMessage = TEXT("[StartLocalMonologueInternal] Invalid start request. MainParticipantActor must be provided.");
		return false;
	}

	TScriptInterface<IMounteaDialogueParticipantInterface> mainParticipant;
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> allParticipants;
	if (!ResolveDialogueParticipants(Request, mainParticipant, allParticipants, OutErrorMessage))
		return false;

	UMounteaDialogueGraph* resolvedGraph = ResolveActiveDialogueGraph(Request, mainParticipant, OutErrorMessage);
	if (!IsValid(resolvedGraph))
		return false;

	UMounteaDialogueGraphNode* graphStartNode = resolvedGraph->GetStartNode();
	if (!IsValid(graphStartNode))
	{
		OutErrorMessage = FString::Printf(
			TEXT("[StartLocalMonologueInternal] Dialogue graph '%s' has no start node."),
			*resolvedGraph->GetName());
		return false;
	}

	UMounteaDialogueGraphNode* firstRuntimeNode = UMounteaDialogueTraversalStatics::GetFirstChildNode(graphStartNode);
	if (!IsValid(firstRuntimeNode))
	{
		OutErrorMessage = FString::Printf(
			TEXT("[StartLocalMonologueInternal] Dialogue graph '%s' start node has no valid child to begin from."),
			*resolvedGraph->GetName());
		return false;
	}

	UMounteaDialogueContext* tempContext = NewObject<UMounteaDialogueContext>(this);
	if (!IsValid(tempContext))
	{
		OutErrorMessage = TEXT("[StartLocalMonologueInternal] Failed to create local dialogue context.");
		return false;
	}

	tempContext->SessionGUID = FGuid::NewGuid();
	tempContext->DialogueParticipants = allParticipants;

	TArray<UMounteaDialogueGraphNode*> filteredChildren =
		UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(firstRuntimeNode, tempContext);
	UMounteaDialogueTraversalStatics::SortNodes(filteredChildren);

	tempContext->SetDialogueContext(firstRuntimeNode, filteredChildren);
	tempContext->UpdateActiveDialogueRow(UMounteaDialogueTraversalStatics::GetSpeechData(firstRuntimeNode));
	tempContext->UpdateActiveDialogueRowDataIndex(0);

	TScriptInterface<IMounteaDialogueParticipantInterface> initialActiveParticipant = mainParticipant;
	const TScriptInterface<IMounteaDialogueParticipantInterface> rolePreferredParticipant =
		UMounteaDialogueParticipantStatics::GetParticipantByType(
			allParticipants,
			EDialogueParticipantType::NPC,
			this);
	if (rolePreferredParticipant.GetObject() && rolePreferredParticipant.GetInterface())
		initialActiveParticipant = rolePreferredParticipant;

	tempContext->SetActiveDialogueParticipant(initialActiveParticipant);
	const TScriptInterface<IMounteaDialogueParticipantInterface> resolvedActiveParticipant =
		UMounteaDialogueTraversalStatics::ResolveActiveParticipant(tempContext);
	UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(tempContext, resolvedActiveParticipant);

	SetDialogueContext(tempContext);
	OnDialogueStartRequestedResult.Broadcast(true, TEXT("OK"));
	SetManagerState(EDialogueManagerState::EDMS_Active);
	return true;
}

void UMounteaDialogueLocalMonologueComponent::SetManagerState(const EDialogueManagerState NewState)
{
	if (NewState == ManagerState)
	{
		LOG_INFO(TEXT("[Set Manager State] New State `%s` is same as current State. Update aborted."),
			*(UMounteaDialogueSystemBFC::GetEnumFriendlyName(NewState)))
		return;
	}

	ManagerState = NewState;
	ProcessStateUpdated();
}

void UMounteaDialogueLocalMonologueComponent::SetDefaultManagerState(const EDialogueManagerState NewState)
{
	if (NewState == DefaultManagerState)
	{
		LOG_WARNING(TEXT("[Set Default Manager State] New State `%s` is same as current State. Update aborted."),
			*(UMounteaDialogueSystemBFC::GetEnumFriendlyName(NewState)))
		return;
	}

	DefaultManagerState = NewState;
}

void UMounteaDialogueLocalMonologueComponent::RequestStartDialogue_Implementation(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
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

	FString errorMessage;
	RequestStartLocalMonologue(request, errorMessage);
}

void UMounteaDialogueLocalMonologueComponent::RequestCloseDialogue_Implementation()
{
	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	SetManagerState(DefaultManagerState);
}

void UMounteaDialogueLocalMonologueComponent::StartDialogue_Implementation()
{
	StartParticipants();

	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueStarted.Broadcast(DialogueContext);

	ExecuteWidgetCommand(MounteaDialogueWidgetCommands::CreateDialogueWidget);
	Execute_PrepareNode(this);
}

void UMounteaDialogueLocalMonologueComponent::CloseDialogue_Implementation()
{
	const FGuid closingSessionGuid = IsValid(DialogueContext) ? DialogueContext->SessionGUID : FGuid();

	StopParticipants();
	ExecuteWidgetCommand(MounteaDialogueWidgetCommands::CloseDialogueWidget);
	Execute_CleanupDialogue(this);

	if (IsValid(DialogueContext))
		DialogueContext->TraversedPath.Empty();

	SetDialogueContext(nullptr);

	if (UMounteaDialogueManagerStatics::ShouldExecuteCosmetics(GetOwner()))
		OnDialogueClosed.Broadcast(DialogueContext);

	DialogueInstigator = nullptr;

	if (UMounteaDialogueLocalMonologueSubsystem* localSubsystem = ResolveLocalMonologueSubsystem())
		localSubsystem->ReleaseMonologueLock(this, closingSessionGuid);
}

void UMounteaDialogueLocalMonologueComponent::PrepareNode_Implementation()
{
	UMounteaDialogueContext* dialogueContext = Execute_GetDialogueContext(this);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Prepare Node] Invalid local dialogue context."));
		return;
	}

	if (!IsValid(dialogueContext->ActiveNode))
	{
		OnDialogueFailed.Broadcast(TEXT("[Prepare Node] No active node."));
		return;
	}

	const TScriptInterface<IMounteaDialogueParticipantInterface> newActiveParticipant =
		UMounteaDialogueTraversalStatics::ResolveActiveParticipant(dialogueContext);
	UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(dialogueContext, newActiveParticipant);
	dialogueContext->ActiveNode->PreProcessNode(this);
}

void UMounteaDialogueLocalMonologueComponent::NodePrepared_Implementation()
{
	UMounteaDialogueContext* dialogueContext = Execute_GetDialogueContext(this);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Prepared] Invalid local dialogue context."));
		return;
	}

	if (!IsValid(dialogueContext->ActiveNode))
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Prepared] No active node."));
		return;
	}

	dialogueContext->AddTraversedNode(dialogueContext->ActiveNode);
	Execute_ProcessNode(this);
}

void UMounteaDialogueLocalMonologueComponent::ProcessNode_Implementation()
{
	UMounteaDialogueContext* dialogueContext = Execute_GetDialogueContext(this);
	if (!IsValid(dialogueContext) || !IsValid(dialogueContext->ActiveNode))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Node] Invalid context or active node."));
		return;
	}

	UMounteaDialogueGraphNode* processingNode = dialogueContext->ActiveNode;
	if (processingNode->Implements<UMounteaDialogueSpeechDataInterface>()
		&& (!dialogueContext->ActiveDialogueParticipant.GetObject()
			|| !dialogueContext->ActiveDialogueParticipant.GetInterface()))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Node] Speech node requires a valid active participant."));
		return;
	}

	processingNode->ProcessNode(this);

	if (dialogueContext->ActiveNode != processingNode)
	{
		if (!IsValid(dialogueContext->ActiveNode))
		{
			Execute_RequestCloseDialogue(this);
			return;
		}

		ApplyNodeSwitchForLinearMonologue(dialogueContext, dialogueContext->ActiveNode);
		ExecuteWidgetCommand(MounteaDialogueWidgetCommands::HideDialogueRow);
		Execute_PrepareNode(this);
		return;
	}

	TArray<UMounteaDialogueGraphNode*> allowedChildrenNodes =
		UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(dialogueContext->ActiveNode, dialogueContext);
	UMounteaDialogueTraversalStatics::SortNodes(allowedChildrenNodes);

	dialogueContext->UpdateAllowedChildrenNodes(allowedChildrenNodes);
	dialogueContext->UpdateActiveDialogueRow(UMounteaDialogueTraversalStatics::GetSpeechData(processingNode));
	dialogueContext->UpdateActiveDialogueRowDataIndex(0);
	OnDialogueContextUpdated.Broadcast(dialogueContext);

	OnDialogueNodeStarted.Broadcast(dialogueContext);

	if (!processingNode->Implements<UMounteaDialogueSpeechDataInterface>())
		ExecuteWidgetCommand(MounteaDialogueWidgetCommands::HideDialogueRow);

	if (processingNode->Implements<UMounteaDialogueSpeechDataInterface>()
		&& dialogueContext->ActiveDialogueParticipant.GetObject()
		&& dialogueContext->ActiveDialogueParticipant.GetInterface())
	{
		dialogueContext->ActiveDialogueParticipant->GetOnParticipantBecomeActiveEventHandle().Broadcast(true);
	}

	Execute_ProcessDialogueRow(this);
}

void UMounteaDialogueLocalMonologueComponent::NodeProcessed_Implementation()
{
	UMounteaDialogueContext* dialogueContext = Execute_GetDialogueContext(this);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Processed] Invalid local dialogue context."));
		return;
	}

	if (!IsValid(dialogueContext->ActiveNode))
	{
		OnDialogueFailed.Broadcast(TEXT("[Node Processed] Invalid active node."));
		return;
	}

	OnDialogueNodeFinished.Broadcast(dialogueContext);
	dialogueContext->ActiveNode->CleanupNode();

	TArray<UMounteaDialogueGraphNode*> allowedChildrenNodes =
		UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(dialogueContext->ActiveNode, dialogueContext);
	UMounteaDialogueTraversalStatics::SortNodes(allowedChildrenNodes);

	UMounteaDialogueGraphNode* newActiveNode = nullptr;
	for (UMounteaDialogueGraphNode* childNode : allowedChildrenNodes)
	{
		if (IsValid(childNode) && childNode->DoesAutoStart())
		{
			newActiveNode = childNode;
			break;
		}
	}

	if (IsValid(newActiveNode))
	{
		ApplyNodeSwitchForLinearMonologue(dialogueContext, newActiveNode);
		ExecuteWidgetCommand(MounteaDialogueWidgetCommands::RemoveDialogueOptions);
		Execute_PrepareNode(this);
		return;
	}

	Execute_RequestCloseDialogue(this);
}

void UMounteaDialogueLocalMonologueComponent::SelectNode_Implementation(const FGuid& NodeGuid)
{
	LOG_WARNING(TEXT("[Select Node] Local monologue is linear-only; option selection is ignored."))
}

void UMounteaDialogueLocalMonologueComponent::ProcessDialogueRow_Implementation()
{
	UMounteaDialogueContext* dialogueContext = Execute_GetDialogueContext(this);
	if (!UMounteaDialogueContextStatics::IsContextValid(dialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Invalid local dialogue context."));
		return;
	}

	UWorld* world = GetWorld();
	if (!IsValid(world))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] World is not valid."));
		return;
	}

	if (!dialogueContext->ActiveNode->IsA(UMounteaDialogueGraphNode_DialogueNodeBase::StaticClass()))
		return;

	const int32 activeIndex = dialogueContext->GetActiveDialogueRowDataIndex();
	const FDialogueRow row = dialogueContext->GetActiveDialogueRow();
	if (!row.RowData.IsValidIndex(activeIndex))
	{
		Execute_DialogueRowProcessed(this, false);
		return;
	}

	const FDialogueRowData rowData = row.RowData[activeIndex];
	if (!UMounteaDialogueTraversalStatics::IsDialogueRowDataValid(rowData))
	{
		Execute_DialogueRowProcessed(this, false);
		return;
	}

	OnDialogueRowStarted.Broadcast(dialogueContext);
	ExecuteWidgetCommand(MounteaDialogueWidgetCommands::ShowDialogueRow);

	if (dialogueContext->ActiveDialogueParticipant.GetObject() && dialogueContext->ActiveDialogueParticipant.GetInterface())
	{
		dialogueContext->ActiveDialogueParticipant->Execute_PlayParticipantVoice(
			dialogueContext->ActiveDialogueParticipant.GetObject(),
			rowData.RowSound);
	}

	FTimerDelegate rowDelegate;
	rowDelegate.BindUObject(this, &UMounteaDialogueLocalMonologueComponent::DialogueRowProcessed_Implementation, false);
	world->GetTimerManager().SetTimer(
		TimerHandle_RowTimer,
		rowDelegate,
		UMounteaDialogueTraversalStatics::GetRowDuration(rowData),
		false);
}

void UMounteaDialogueLocalMonologueComponent::DialogueRowProcessed_Implementation(const bool bForceFinish)
{
	if (ManagerState != EDialogueManagerState::EDMS_Active)
		return;

	UMounteaDialogueContext* dialogueContext = Execute_GetDialogueContext(this);
	if (!IsValid(dialogueContext))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] Invalid local dialogue context."));
		return;
	}

	UWorld* world = GetWorld();
	if (!IsValid(world))
	{
		OnDialogueFailed.Broadcast(TEXT("[Process Dialogue Row] World is not valid."));
		return;
	}

	world->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const FMounteaDialogueRowDataInfo rowInfo = GetDialogueRowDataInfo(dialogueContext);

	if (rowInfo.ActiveRowExecutionMode == ERowExecutionMode::EREM_AwaitInput && !bForceFinish)
		return;

	OnDialogueRowFinished.Broadcast(dialogueContext);

	if (rowInfo.bIsActiveRowValid && rowInfo.bDialogueRowDataValid)
	{
		switch (rowInfo.NextRowExecutionMode)
		{
		case ERowExecutionMode::EREM_Automatic:
		case ERowExecutionMode::EREM_AwaitInput:
			dialogueContext->UpdateActiveDialogueRowDataIndex(rowInfo.IncreasedIndex);
			OnDialogueContextUpdated.Broadcast(dialogueContext);
			ExecuteWidgetCommand(MounteaDialogueWidgetCommands::ShowDialogueRow);
			Execute_ProcessDialogueRow(this);
			break;
		case ERowExecutionMode::EREM_Stopping:
			ExecuteWidgetCommand(MounteaDialogueWidgetCommands::HideDialogueRow);
			Execute_NodeProcessed(this);
			break;
		case ERowExecutionMode::Default:
			break;
		}

		return;
	}

	ExecuteWidgetCommand(MounteaDialogueWidgetCommands::HideDialogueRow);
	Execute_NodeProcessed(this);
}

void UMounteaDialogueLocalMonologueComponent::SkipDialogueRow_Implementation()
{
	Execute_DialogueRowProcessed(this, true);
}

void UMounteaDialogueLocalMonologueComponent::ExecuteWidgetCommand_Implementation(const FString& Command)
{
	if (Command.IsEmpty())
	{
		LOG_WARNING(TEXT("[MounteaDialogueLocalMonologue] ExecuteWidgetCommand received empty command. Skipping."));
		return;
	}

	FGuid sessionGuid;
	if (IsValid(DialogueContext))
		sessionGuid = DialogueContext->SessionGUID;

	if (!sessionGuid.IsValid() && Command != MounteaDialogueWidgetCommands::CloseDialogueWidget)
	{
		LOG_WARNING(TEXT("[MounteaDialogueLocalMonologue] ExecuteWidgetCommand('%s') skipped: no valid local session."), *Command);
		return;
	}

	FMounteaDialogueUISignal signal;
	signal.Command = Command;
	signal.SessionGUID = sessionGuid;
	signal.RequiredContextVersion = 0;
	signal.bForceReconcile = false;
	OnDialogueUISignalRequested.Broadcast(signal);

	if (Command == MounteaDialogueWidgetCommands::CloseDialogueWidget)
	{
		FMounteaDialogueUISignal clearSignal;
		clearSignal.SessionGUID = sessionGuid;
		clearSignal.RequiredContextVersion = INT32_MAX;
		OnDialogueUISignalRequested.Broadcast(clearSignal);
	}
}

ULocalPlayer* UMounteaDialogueLocalMonologueComponent::ResolveOwningLocalPlayer() const
{
	const AActor* ownerActor = GetOwner();
	if (!IsValid(ownerActor))
		return nullptr;

	if (const APlayerController* playerController = Cast<APlayerController>(ownerActor))
		return playerController->GetLocalPlayer();

	if (const APawn* pawn = Cast<APawn>(ownerActor))
	{
		if (const APlayerController* pawnController = Cast<APlayerController>(pawn->GetController()))
			return pawnController->GetLocalPlayer();
	}

	if (const APlayerState* playerState = Cast<APlayerState>(ownerActor))
	{
		if (const APlayerController* directController = playerState->GetPlayerController())
			return directController->GetLocalPlayer();

		if (const UWorld* world = ownerActor->GetWorld())
		{
			for (FConstPlayerControllerIterator It = world->GetPlayerControllerIterator(); It; ++It)
			{
				const APlayerController* playerController = It->Get();
				if (!IsValid(playerController) || !playerController->IsLocalController())
					continue;

				if (playerController->PlayerState == playerState)
					return playerController->GetLocalPlayer();
			}
		}
	}

	return nullptr;
}

UMounteaDialogueLocalMonologueSubsystem* UMounteaDialogueLocalMonologueComponent::ResolveLocalMonologueSubsystem() const
{
	if (const ULocalPlayer* localPlayer = ResolveOwningLocalPlayer())
		return localPlayer->GetSubsystem<UMounteaDialogueLocalMonologueSubsystem>();

	return nullptr;
}

bool UMounteaDialogueLocalMonologueComponent::ResolveDialogueParticipants(
	const FDialogueStartRequest& Request,
	TScriptInterface<IMounteaDialogueParticipantInterface>& OutMainParticipant,
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& OutAllParticipants,
	FString& OutErrorMessage) const
{
	OutErrorMessage.Empty();

	const UWorld* managerWorld = GetWorld();
	const auto ResolveRequestActor = [managerWorld](AActor* ActorRef, const TSoftObjectPtr<AActor>& SoftRef) -> AActor*
	{
		if (IsValid(ActorRef))
		{
			if (!managerWorld || ActorRef->GetWorld() == managerWorld)
				return ActorRef;

			LOG_WARNING(TEXT("[ResolveDialogueParticipants] Ignoring actor-ref '%s' from different world."), *GetNameSafe(ActorRef))
		}

		if (!SoftRef.IsNull())
		{
			AActor* softResolvedActor = SoftRef.Get();
			if (IsValid(softResolvedActor) && (!managerWorld || softResolvedActor->GetWorld() == managerWorld))
				return softResolvedActor;

			if (IsValid(softResolvedActor))
				LOG_WARNING(TEXT("[ResolveDialogueParticipants] Ignoring soft-ref actor '%s' from different world."), *GetNameSafe(softResolvedActor))
		}

		return nullptr;
	};

	AActor* mainActor = ResolveRequestActor(Request.MainParticipantActorRef, Request.MainParticipantActor);
	if (!mainActor)
	{
		OutErrorMessage = TEXT("[ResolveDialogueParticipants] MainParticipantActor could not be resolved.");
		return false;
	}

	bool bMainFound = false;
	OutMainParticipant = UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(mainActor, bMainFound);
	if (!bMainFound || !OutMainParticipant.GetObject())
	{
		OutErrorMessage = TEXT("[ResolveDialogueParticipants] Main participant does not implement IMounteaDialogueParticipantInterface.");
		return false;
	}

	if (!OutMainParticipant->Execute_CanStartDialogue(OutMainParticipant.GetObject()))
	{
		OutErrorMessage = TEXT("[ResolveDialogueParticipants] Main participant cannot start dialogue.");
		return false;
	}

	OutAllParticipants.Add(OutMainParticipant);

	AActor* ownerActor = GetOwner();
	if (IsValid(ownerActor))
	{
		int32 searchDepth = 0;
		if (APawn* playerPawn = UMounteaDialogueParticipantStatics::FindPlayerPawn(ownerActor, searchDepth))
		{
			bool bPlayerFound = false;
			TScriptInterface<IMounteaDialogueParticipantInterface> playerParticipant =
				UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(playerPawn, bPlayerFound);
			if (bPlayerFound
				&& playerParticipant.GetObject()
				&& playerParticipant->Execute_CanParticipateInDialogue(playerParticipant.GetObject()))
			{
				OutAllParticipants.AddUnique(playerParticipant);
			}
		}
	}

	for (AActor* otherActorRef : Request.OtherParticipantActorRefs)
	{
		AActor* otherActor = ResolveRequestActor(otherActorRef, TSoftObjectPtr<AActor>());
		if (!IsValid(otherActor))
			continue;

		bool bOtherFound = false;
		TScriptInterface<IMounteaDialogueParticipantInterface> otherParticipant =
			UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(otherActor, bOtherFound);
		if (bOtherFound
			&& otherParticipant.GetObject()
			&& otherParticipant->Execute_CanParticipateInDialogue(otherParticipant.GetObject()))
		{
			OutAllParticipants.AddUnique(otherParticipant);
		}
	}

	for (const TSoftObjectPtr<AActor>& softRef : Request.OtherParticipantActors)
	{
		AActor* otherActor = ResolveRequestActor(nullptr, softRef);
		if (!IsValid(otherActor))
			continue;

		bool bOtherFound = false;
		TScriptInterface<IMounteaDialogueParticipantInterface> otherParticipant =
			UMounteaDialogueParticipantStatics::FindDialogueParticipantInterface(otherActor, bOtherFound);
		if (bOtherFound
			&& otherParticipant.GetObject()
			&& otherParticipant->Execute_CanParticipateInDialogue(otherParticipant.GetObject()))
		{
			OutAllParticipants.AddUnique(otherParticipant);
		}
	}

	return true;
}

UMounteaDialogueGraph* UMounteaDialogueLocalMonologueComponent::ResolveActiveDialogueGraph(
	const FDialogueStartRequest& Request,
	const TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant,
	FString& OutErrorMessage) const
{
	OutErrorMessage.Empty();

	if (!Request.DialogueGraph.IsNull())
	{
		UMounteaDialogueGraph* graphOverride = Request.DialogueGraph.LoadSynchronous();
		if (!IsValid(graphOverride))
		{
			OutErrorMessage = TEXT("[ResolveActiveDialogueGraph] DialogueGraph override is set but failed to load.");
			return nullptr;
		}
		return graphOverride;
	}

	UMounteaDialogueGraph* participantGraph =
		MainParticipant.GetObject() ? MainParticipant->Execute_GetDialogueGraph(MainParticipant.GetObject()) : nullptr;
	if (!IsValid(participantGraph))
	{
		OutErrorMessage = TEXT("[ResolveActiveDialogueGraph] Unable to resolve active dialogue graph.");
		return nullptr;
	}

	return participantGraph;
}

void UMounteaDialogueLocalMonologueComponent::ApplyNodeSwitchForLinearMonologue(
	UMounteaDialogueContext* Context,
	UMounteaDialogueGraphNode* NewActiveNode)
{
	if (!IsValid(Context) || !IsValid(NewActiveNode))
		return;

	TArray<UMounteaDialogueGraphNode*> allowedChildNodes =
		UMounteaDialogueTraversalStatics::GetAllowedChildNodesFiltered(NewActiveNode, Context);
	UMounteaDialogueTraversalStatics::SortNodes(allowedChildNodes);

	Context->SetDialogueContext(NewActiveNode, allowedChildNodes);
	Context->UpdateActiveDialogueRow(UMounteaDialogueTraversalStatics::GetSpeechData(NewActiveNode));
	Context->UpdateActiveDialogueRowDataIndex(0);

	const TScriptInterface<IMounteaDialogueParticipantInterface> newActiveParticipant =
		UMounteaDialogueTraversalStatics::ResolveActiveParticipant(Context);
	UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(Context, newActiveParticipant);

	OnDialogueNodeSelected.Broadcast(Context);
	OnDialogueContextUpdated.Broadcast(Context);
}
