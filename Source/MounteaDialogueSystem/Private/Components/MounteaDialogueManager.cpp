// All rights reserved Dominik Pavlicek 2023

#include "Components/MounteaDialogueManager.h"

#include "TimerManager.h"

#include "Graph/MounteaDialogueGraph.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Engine/ActorChannel.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/MounteaDialogueWBPInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


UMounteaDialogueManager::UMounteaDialogueManager()
	: DefaultManagerState(EDialogueManagerState::EDMS_Enabled)
	, DialogueContext(nullptr)
	, ReplicatedDialogueContext(nullptr)
	, bWasCursorVisible(false)
	, DialogueContextReplicationKey(0)
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

	// Setup bindings
	{
		OnDialogueInitialized.							AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueInitializedEvent_Internal);
	
		OnDialogueContextUpdated.				AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueContextUpdatedEvent_Internal);
		OnDialogueUserInterfaceChanged.	AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueUserInterfaceChangedEvent_Internal);
	
		OnDialogueStarted.								AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueStartedEvent_Internal);
		OnDialogueClosed.								AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueClosedEvent_Internal);

		OnDialogueNodeSelected.					AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueNodeSelectedEvent_Internal);
		OnDialogueNodeStarted.						AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueNodeStartedEvent_Internal);
		OnDialogueNodeFinished.					AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueNodeFinishedEvent_Internal);

		OnDialogueRowStarted.						AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueRowStartedEvent_Internal);
		OnDialogueRowFinished.					AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueRowFinishedEvent_Internal);

		OnDialogueVoiceStartRequest.			AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueVoiceStartRequestEvent_Internal);
		OnDialogueVoiceSkipRequest.			AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueVoiceSkipRequestEvent_Internal);

		OnNextDialogueRowDataRequested.	AddUniqueDynamic(this, &UMounteaDialogueManager::NextDialogueRowDataRequested);
	}
	
	Execute_SetDialogueManagerState(this, GetDefaultDialogueManagerState());
}

void UMounteaDialogueManager::NetPushDialogueContext()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		DialogueContextReplicationKey++;
		if (DialogueContext)
		{
			DialogueContext->IncreaseRepKey();
		}

		UpdateDialogueContext_Client(FMounteaDialogueContextReplicatedStruct(DialogueContext));
	}
}

void UMounteaDialogueManager::CallDialogueNodeSelected_Implementation(const FGuid& NodeGUID)
{
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[CallDialogueNodeSelected] No owner!"))
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		CallDialogueNodeSelected_Server(NodeGUID);
		return;
	}
	
	UMounteaDialogueGraphNode* selectedNode = nullptr;
	if (DialogueContext)
	{
		const TArray<UMounteaDialogueGraphNode*>& childrenNodes = DialogueContext->GetChildrenNodes();
		auto foundNode = childrenNodes.FindByPredicate([NodeGUID](const UMounteaDialogueGraphNode* Node)
		{
			return Node && Node->GetNodeGUID() == NodeGUID;
		});

		if (foundNode)
		{
			selectedNode = *foundNode;
		}
		else
		{
			// Handle the case where the node is not found
			UE_LOG(LogTemp, Warning, TEXT("Node with GUID %s not found"), *NodeGUID.ToString());
		}
	}
	else
	{
		OnDialogueFailed.Broadcast(TEXT("[CallDialogueNodeSelected] Invalid Context!"));
		return;
	}

	if (!selectedNode)
	{
		OnDialogueFailed.Broadcast(TEXT("[CallDialogueNodeSelected] Cannot find Selected Option!"));
		return;
	}

	// Straight up set dialogue row from Node and index to 0
	DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, selectedNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(selectedNode));
	DialogueContext->UpdateActiveDialogueRow(UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode));
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);

	NetPushDialogueContext();
	
	OnDialogueNodeSelected.Broadcast(DialogueContext);
}

#pragma region InternalEvents

void UMounteaDialogueManager::OnDialogueInitializedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (Context)
	{
		OnDialogueInitializedEvent(Context);

		OnDialogueContextUpdated.Broadcast(Context);

		OnDialogueStarted.Broadcast(Context);

		// No need to refresh all again, just call the Event to BPs
		Context->DialogueContextUpdatedFromBlueprint.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueContextUpdatedEvent);
	}
	else
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}
}

void UMounteaDialogueManager::OnDialogueContextUpdatedEvent_Internal(UMounteaDialogueContext* NewContext)
{
	SetDialogueContext(NewContext);
}

void UMounteaDialogueManager::OnDialogueUserInterfaceChangedEvent_Internal(TSubclassOf<UUserWidget> DialogueUIClass, UUserWidget* DialogueUIWidget)
{
	OnDialogueUserInterfaceChangedEvent(DialogueUIClass, DialogueUIWidget);
}

void UMounteaDialogueManager::OnDialogueStartedEvent_Internal(UMounteaDialogueContext* Context)
{
	Execute_StartDialogue(this);

	OnDialogueStartedEvent(Context);
}

void UMounteaDialogueManager::OnDialogueClosedEvent_Internal(UMounteaDialogueContext* Context)
{
	switch (GetDialogueManagerState())
	{
		case EDialogueManagerState::EDMS_Disabled:
		case EDialogueManagerState::EDMS_Enabled:
			return;
		case EDialogueManagerState::EDMS_Active:
			break;
	}
	
	OnDialogueClosedEvent(DialogueContext);

	Execute_CloseDialogue(this);
}

void UMounteaDialogueManager::OnDialogueNodeSelectedEvent_Internal(UMounteaDialogueContext* Context)
{
	OnDialogueNodeSelectedEvent(Context);

	NetPushDialogueContext();

	LOG_INFO(TEXT("[OnDialogueNodeSelectedEvent] Node Selected: %s"), *Context->GetActiveNode()->GetNodeTitle().ToString())
	
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		if (GetOwner()->HasAuthority())
		{
			FString resultMessage;
			Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::RemoveDialogueOptions);
		}
	}
	else
	{
		UpdateDialogueContext_Client(FMounteaDialogueContextReplicatedStruct(DialogueContext));
		UpdateDialogueUI_Client(MounteaDialogueWidgetCommands::RemoveDialogueOptions);
	}

	Execute_PrepareNode(this);
}

// TODO: Implement NODE STATE for easier State Machine transitions (to avoid starting 1 node multiple times etc.)
void UMounteaDialogueManager::OnDialogueNodeStartedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}
	
	OnDialogueNodeStartedEvent(Context);
	
	Execute_StartExecuteDialogueRow(this);
}

void UMounteaDialogueManager::OnDialogueNodeFinishedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}

	// Stop Ticking Active Node and read Ticking from parent Graph
	if (DialogueContext && DialogueContext->ActiveNode)
	{
		DialogueContext->ActiveNode->Execute_UnregisterTick(DialogueContext->ActiveNode, DialogueContext->ActiveNode->Graph);
	}
	
	OnDialogueNodeFinishedEvent(DialogueContext);

	const TArray<UMounteaDialogueGraphNode*> allowedChildrenNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(DialogueContext->ActiveNode);

	// If there are only Complete Nodes left or no DialogueNodes left, just shut it down
	if (allowedChildrenNodes.Num() == 0)
	{
		OnDialogueClosed.Broadcast(DialogueContext);
		return;
	}
	
	const bool bAutoActive = allowedChildrenNodes[0]->DoesAutoStart();
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);
	
	if (bAutoActive)
	{
		const auto newActiveNode = allowedChildrenNodes[0];

		if (!newActiveNode)
		{
			OnDialogueClosed.Broadcast(DialogueContext);	
		}
		
		DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, newActiveNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(newActiveNode));
		
		OnDialogueNodeSelected.Broadcast(DialogueContext);

		NetPushDialogueContext();
	}
	else
	{
		NetPushDialogueContext();
		
		if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
		{
			if (GetOwner()->HasAuthority())
			{
				FString resultMessage;
				Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::AddDialogueOptions);
			}
			else
			{
				UpdateDialogueContext_Client(FMounteaDialogueContextReplicatedStruct(DialogueContext));
				UpdateDialogueUI_Client(MounteaDialogueWidgetCommands::AddDialogueOptions);
			}
		}
		else
		{
			UpdateDialogueContext_Client(FMounteaDialogueContextReplicatedStruct(DialogueContext));
			UpdateDialogueUI_Client(MounteaDialogueWidgetCommands::AddDialogueOptions);
		}
	}
}

void UMounteaDialogueManager::OnDialogueRowStartedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (Context == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueRowStartedEvent] Invalid Dialogue Context!"));
		return;
	}

	if (Context->GetActiveDialogueRow().DialogueRowData.Array().IsValidIndex(Context->GetActiveDialogueRowDataIndex()) == false)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueRowStartedEvent] Trying to Access Invalid Dialogue Row data!"));
		return;
	}

	int32 activeIndex = Context->GetActiveDialogueRowDataIndex();

	// Let's hope we are not approaching invalid indexes
	USoundBase* soundToStart =  Context->GetActiveDialogueRow().DialogueRowData.Array()[Context->GetActiveDialogueRowDataIndex()].RowSound;
	
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		if (GetOwner()->HasAuthority())
		{
			OnDialogueVoiceStartRequest.Broadcast(soundToStart);
		}
	}
	else
	{
		RequestVoiceStart_Client(soundToStart);
	}
}

void UMounteaDialogueManager::OnDialogueRowFinishedEvent_Internal(UMounteaDialogueContext* Context)
{
	// Not necessary needed, however, provides a nice way to add functionality later on 
}

void UMounteaDialogueManager::OnDialogueVoiceStartRequestEvent_Internal(USoundBase* VoiceToStart)
{
	if (DialogueContext == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceStartRequestEvent] Invalid Dialogue Context!"));
		return;
	}

	if (DialogueContext->ActiveDialogueParticipant.GetInterface() == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceStartRequestEvent] Invalid Dialogue Participant!"));
		return;
	}
	
	DialogueContext->ActiveDialogueParticipant->Execute_PlayParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), VoiceToStart);
	OnDialogueVoiceStartRequestEvent(VoiceToStart);
}

void UMounteaDialogueManager::OnDialogueVoiceSkipRequestEvent_Internal(USoundBase* VoiceToSkip)
{
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[OnDialogueVoiceSkipRequest] No owner!"))
		return;
	}

	if (DialogueContext == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceSkipRequestEvent] Invalid Dialogue Context!"));
		return;
	}

	if (DialogueContext->ActiveDialogueParticipant.GetInterface() == nullptr || DialogueContext->ActiveDialogueParticipant.GetObject() == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceSkipRequestEvent] Invalid Dialogue Participant!"));
		return;
	}
	
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		if (GetOwner()->HasAuthority())
		{
			DialogueContext->ActiveDialogueParticipant->Execute_SkipParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), VoiceToSkip);
			OnDialogueVoiceSkipRequestEvent(VoiceToSkip);
		}
		else
		{
			RequestVoiceStop_Client(VoiceToSkip);
		}
	}
	else
	{
		RequestVoiceStop_Client(VoiceToSkip);
	}

	/*
	if (auto dialogueSettings = UMounteaDialogueSystemBFC::GetDialogueSystemSettings())
	{
		bool bCanSkipRow = dialogueSettings->CanSkipWholeRow();
		bool bNodeInversion = UMounteaDialogueSystemBFC::DoesNodeInvertSkipSettings(DialogueContext->ActiveNode);
		bool finalValue = bCanSkipRow != bNodeInversion;

		if (!finalValue)
		{
			return;
		}
	}
	*/

	// This is brute force that I want to change in next big update, sorry for this.
	if (UMounteaDialogueSystemBFC::DoesPreviousNodeSkipActiveNode(DialogueContext->DialogueParticipant->GetDialogueGraph(), DialogueContext->PreviousActiveNode))
	{
		return;
	}
	
	OnNextDialogueRowDataRequested.Broadcast(DialogueContext);
}

#pragma endregion

void UMounteaDialogueManager::InitializeDialogue_Implementation(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants)
{
	if (Participants.MainParticipant == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Main Participant is not valid!"))
		return;
	}
	
	if (!GetOwner())
	{
		OnDialogueFailed.Broadcast(TEXT("No Owner!"));
		return;
	}
	
	if (!GetOwner()->HasAuthority())
	{
		InitializeDialogue_Server(OwningPlayerState, Participants);
		return;
	}

	TSet<TScriptInterface<IMounteaDialogueParticipantInterface> > dialogueParticipants; 
	{
		bool bMainParticipantFound = false;
		TScriptInterface<IMounteaDialogueParticipantInterface> mainParticipantInterface = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(Participants.MainParticipant, bMainParticipantFound);

		if (!bMainParticipantFound)
		{
			LOG_ERROR(TEXT("[InitializeDialogue] Main Dialogue Participant is not valid!"))
			return;
		}
		
		dialogueParticipants.Add(mainParticipantInterface);
	}

	for (AActor* Itr : Participants.OtherParticipants)
	{
		bool bParticipantFound = false;
		auto foundParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(Itr, bParticipantFound);

		if (bParticipantFound)
		{
			dialogueParticipants.Add(foundParticipant);
		}
	}

	if (dialogueParticipants.Num() == 0)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] No valid Participant found!"))
		return;
	}
	
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> dialogueParticipantsArray = dialogueParticipants.Array();
	
	UMounteaDialogueSystemBFC::StartDialogue(GetOwner(), OwningPlayerState, dialogueParticipantsArray[0], dialogueParticipantsArray);
}

void UMounteaDialogueManager::StartDialogue_Implementation()
{
	if (!GetOwner())
	{
		OnDialogueFailed.Broadcast(TEXT("No Owner!"));
		return;
	}
	
	if (!GetOwner()->HasAuthority())
	{
		StartDialogue_Server();
		return;
	}

	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}
	
	Execute_SetDialogueManagerState(this, EDialogueManagerState::EDMS_Active);

	for (const auto& Itr : DialogueContext->DialogueParticipants)
	{
		if (!Itr.GetObject() || !Itr.GetInterface()) continue;

		TScriptInterface<IMounteaDialogueTickableObject> tickableObject = Itr.GetObject();
		if (tickableObject.GetInterface() && tickableObject.GetObject())
		{
			// Register ticks for participants, no need to define Parent as Participants are the most paren ones
			tickableObject->Execute_RegisterTick(tickableObject.GetObject(), nullptr);
		}

		Itr->SetParticipantState(EDialogueParticipantState::EDPS_Active);
	}
}

void UMounteaDialogueManager::CloseDialogue_Implementation()
{
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[CloseDialogue] Invalid owner!"))
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	if (!GetOwner()->HasAuthority())
	{
		CloseDialogue_Server();
	}

	if (!GetWorld()) return;
		
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		if (GetOwner()->HasAuthority())
		{
			Execute_CloseDialogueUI(this);
		}
	}
	else
	{
		if (DialogueWidgetPtr)
		{
			DialogueWidgetPtr->RemoveFromParent();
			DialogueWidgetPtr = nullptr;
		}
		UpdateDialogueContext_Client(FMounteaDialogueContextReplicatedStruct(nullptr));
		CloseDialogueUI_Client();
	}
	
	Execute_SetDialogueManagerState(this, EDialogueManagerState::EDMS_Enabled);

	if(!DialogueContext) return;

	for (const auto& Itr : DialogueContext->DialogueParticipants)
	{
		if (!Itr.GetObject() || !Itr.GetInterface()) continue;

		TScriptInterface<IMounteaDialogueTickableObject> tickableObject = Itr.GetObject();
		if (tickableObject.GetInterface() && tickableObject.GetObject())
		{
			// Register ticks for participants, no need to define Parent as Participants are the most paren ones
			tickableObject->Execute_UnregisterTick(tickableObject.GetObject(), nullptr);
		}

		Itr->SetParticipantState(Itr->GetDefaultParticipantState());
	}
	
	if (!DialogueContext->GetDialogueParticipant().GetObject()) return;

	// Cleaning up
	UMounteaDialogueSystemBFC::CleanupGraph(this, DialogueContext->GetDialogueParticipant()->GetDialogueGraph());
	UMounteaDialogueSystemBFC::SaveTraversePathToParticipant(DialogueContext->TraversedPath, DialogueContext->GetDialogueParticipant());
	
	// Clear binding
	DialogueContext->DialogueContextUpdatedFromBlueprint.RemoveDynamic(this, &UMounteaDialogueManager::OnDialogueContextUpdatedEvent);
	
	DialogueContext->SetDialogueContext(nullptr, nullptr, TArray<UMounteaDialogueGraphNode*>());
	DialogueContext->MarkAsGarbage();
	DialogueContext = nullptr;

	NetPushDialogueContext();
}

void UMounteaDialogueManager::ProcessNode_Implementation()
{
	// Then Process Node
	if (DialogueContext && DialogueContext->ActiveNode)
	{
		DialogueContext->ActiveNode->ProcessNode(this);
	}
}

void UMounteaDialogueManager::PrepareNode_Implementation()
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}

	DialogueContext->AddTraversedNode(DialogueContext->ActiveNode);

	NetPushDialogueContext();
	
	// First PreProcess Node
	DialogueContext->ActiveNode->PreProcessNode(this);
	
	Execute_ProcessNode(this);
}

AActor* UMounteaDialogueManager::GetOwningActor_Implementation() const
{
	return GetOwner();
}

TSubclassOf<UUserWidget> UMounteaDialogueManager::GetDialogueWidgetClass() const
{
	if (DialogueWidgetClass.Get() != nullptr)
	{
		return DialogueWidgetClass;
	}

	if (UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal()->GetDefaultDialogueWidget().IsNull())
	{
		return nullptr;
	}

	return UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal()->GetDefaultDialogueWidget().LoadSynchronous();
}

void UMounteaDialogueManager::SetDialogueWidgetClass(const TSubclassOf<UUserWidget> NewWidgetClass)
{
	if (NewWidgetClass == DialogueWidgetClass) return;

	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetDialogueWidgetClass] Dialogue Manager has no Owner!"))
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		DialogueWidgetClass = NewWidgetClass;

		OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidgetPtr);
	}
	else
	{
		SetDialogueWidgetClass_Server(NewWidgetClass);

		//TODO: Client event?
	}
}

void UMounteaDialogueManager::SetDialogueWidget(UUserWidget* NewDialogueWidgetPtr)
{
	DialogueWidgetPtr = NewDialogueWidgetPtr;

	OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidgetPtr);
}

UUserWidget* UMounteaDialogueManager::GetDialogueWidget_Implementation() const
{
	return DialogueWidgetPtr;
}

void UMounteaDialogueManager::StartExecuteDialogueRow_Implementation()
{
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[StartExecuteDialogueRow] No Owner!"))
		return;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		NetPushDialogueContext();
		
		StartExecuteDialogueRow_Client();
	}
	
	// Start Ticking Active Node and read Ticking from parent Graph
	if (DialogueContext && DialogueContext->ActiveNode)
	{
		DialogueContext->ActiveNode->Execute_RegisterTick(DialogueContext->ActiveNode, DialogueContext->ActiveNode->Graph);
	}
	
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		if (GetOwner()->HasAuthority())
		{
			FString resultMessage;
			Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::ShowDialogueRow);
		}
	}
	else
	{
		UpdateDialogueContext_Client(FMounteaDialogueContextReplicatedStruct(DialogueContext));
		UpdateDialogueUI_Client(MounteaDialogueWidgetCommands::ShowDialogueRow);
	}

	if (DialogueContext->DialogueParticipant)
	{
		if (UMounteaDialogueSystemBFC::DoesPreviousNodeSkipActiveNode(DialogueContext->DialogueParticipant->GetDialogueGraph(), DialogueContext->PreviousActiveNode))
		{
			return;
		}
	}

	OnDialogueRowStarted.Broadcast(DialogueContext);
}

void UMounteaDialogueManager::FinishedExecuteDialogueRow_Implementation()
{
	if (!GetWorld())
	{
		OnDialogueFailed.Broadcast(TEXT("Cannot find World!"));
		return;
	}

	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[FinishedExecuteDialogueRow] No owner!"))
		return;;
	}

	if (!GetOwner()->HasAuthority())
	{
		FinishedExecuteDialogueRow_Server();
		
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const MounteaDialogueManagerHelpers::FDialogueRowDataInfo processInfo = MounteaDialogueManagerHelpers::GetDialogueRowDataInfo(DialogueContext);

	LOG_INFO(TEXT("[FinishedExecuteDialogueRow] Dialogue Row Finished"))

	OnDialogueRowFinished.Broadcast(DialogueContext);
	
	if (processInfo.ActiveRowExecutionMode == ERowExecutionMode::EREM_AwaitInput)
	{
		LOG_INFO(TEXT("[FinishedExecuteDialogueRow] This row has finished and next will execute only using 'TriggerNextDialogueRow'."))
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
					NetPushDialogueContext();		
					Execute_StartExecuteDialogueRow(this);
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
		OnDialogueNodeFinished.Broadcast(DialogueContext);
	}
}

void UMounteaDialogueManager::TriggerNextDialogueRow_Implementation()
{
	if (!GetWorld())
	{
		OnDialogueFailed.Broadcast(TEXT("Cannot find World!"));
		return;
	}

	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[TriggerNextDialogueRow] No owner!"))
		return;;
	}

	if (!GetOwner()->HasAuthority())
	{
		TriggerNextDialogueRow_Server();

		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const MounteaDialogueManagerHelpers::FDialogueRowDataInfo processInfo = MounteaDialogueManagerHelpers::GetDialogueRowDataInfo(DialogueContext);

	OnDialogueRowFinished.Broadcast(DialogueContext);
	
	if (processInfo.bDialogueRowDataValid && processInfo.bIsActiveRowValid)
	{
		switch (processInfo.ActiveRowExecutionMode)
		{
			case ERowExecutionMode::EREM_Automatic:
			case ERowExecutionMode::EREM_AwaitInput:
				DialogueContext->UpdateActiveDialogueRowDataIndex(processInfo.IncreasedIndex);
				OnDialogueContextUpdated.Broadcast(DialogueContext);
				NetPushDialogueContext();
				Execute_StartExecuteDialogueRow(this);
				break;
			case ERowExecutionMode::EREM_Stopping:
				OnDialogueNodeFinished.Broadcast(DialogueContext);
				break;
			case ERowExecutionMode::Default:
				OnDialogueNodeFinished.Broadcast(DialogueContext);
				break;
		}
	}
	else
	{
		OnDialogueNodeFinished.Broadcast(DialogueContext);
	}
}

void UMounteaDialogueManager::NextDialogueRowDataRequested(UMounteaDialogueContext* Context)
{
	//FinishedExecuteDialogueRow();
	
	Execute_TriggerNextDialogueRow(this);
}

void UMounteaDialogueManager::SetDialogueContext(UMounteaDialogueContext* NewContext)
{
	if (NewContext == DialogueContext) return;

	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetDialogueContext] Dialogue Manager has no Owner!"))
		return;
	}
	if (GetOwner()->HasAuthority())
	{
		DialogueContext = NewContext;

		NetPushDialogueContext();
		
		OnDialogueContextUpdatedEvent(DialogueContext);
	}
	else
	{
		SetDialogueContext_Server(NewContext);
	}
}

void UMounteaDialogueManager::SkipDialogueRow_Implementation()
{
	OnDialogueVoiceSkipRequest.Broadcast(nullptr);
}

void UMounteaDialogueManager::SetDialogueManagerState_Implementation(const EDialogueManagerState NewState)
{
	if (NewState == ManagerState) return;
	
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetDialogueManagerState] Dialogue Manager has no Owner!"))
		return;
	}
	
	if (GetOwner()->HasAuthority())
	{
		ManagerState = NewState;
	
		OnDialogueManagerStateChanged.Broadcast(NewState);

		// Invoke UI on Standalone Client directly, as OnRep does not work
		if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
		{
			FString ErrorMessage;
			if (GetOwner()->HasAuthority())
			{
				switch (ManagerState)
				{
					case EDialogueManagerState::EDMS_Disabled:
						Execute_CloseDialogueUI(this);
						break;
					case EDialogueManagerState::EDMS_Enabled:
						Execute_CloseDialogueUI(this);
						break;
					case EDialogueManagerState::EDMS_Active:
						FString resultMessage;
						Execute_InvokeDialogueUI(this, resultMessage);
						PostUIInitialized();
						break;
				}
			}
		}
	}
	else
	{
		SetDialogueManagerState_Server(NewState);
	}
}

void UMounteaDialogueManager::SetDefaultDialogueManagerState(const EDialogueManagerState NewState)
{
	if (NewState == DefaultManagerState) return;
	
	if (!GetOwner())
	{
		LOG_ERROR(TEXT("[SetDefaultDialogueManagerState] Dialogue Manager has no Owner!"))
		return;
	}
	
	if (GetOwner()->HasAuthority())
	{
		DefaultManagerState = NewState;
	}
	else
	{
		SetDialogueDefaultManagerState_Server(NewState);
	}
}

void UMounteaDialogueManager::SetDialogueManagerState_Server_Implementation(const EDialogueManagerState NewState)
{
	Execute_SetDialogueManagerState(this, NewState);
}

void UMounteaDialogueManager::SetDialogueDefaultManagerState_Server_Implementation(const EDialogueManagerState NewState)
{
	SetDefaultDialogueManagerState(NewState);
}

void UMounteaDialogueManager::SetDialogueContext_Server_Implementation(UMounteaDialogueContext* NewContext)
{
	SetDialogueContext(NewContext);
}

void UMounteaDialogueManager::SetDialogueWidgetClass_Server_Implementation(TSubclassOf<UUserWidget> NewDialogueWidgetClass)
{
	SetDialogueWidgetClass(NewDialogueWidgetClass);
}

#pragma region UI_Functions

bool UMounteaDialogueManager::InvokeDialogueUI_Implementation(FString& Message)
{
	if (UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal() == nullptr)
	{
		Message = TEXT("Cannot find Dialogue Settings!");
		return false;
	}

	if (UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal()->SubtitlesAllowed() == false)
	{
		return true;
	}
	
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
	
	APlayerController* playerController = UMounteaDialogueSystemBFC::FindPlayerController(GetOwner());
	if (playerController == nullptr)
	{
		Message = TEXT("Invalid Player Controller!");
		return false;
	}
	
	DialogueWidgetPtr = CreateWidget<UUserWidget>(playerController,  GetDialogueWidgetClass());
	
	if (DialogueWidgetPtr == nullptr)
	{
		Message = TEXT("Cannot spawn Dialogue Widget!");
		return false;
	}
	
	if (DialogueWidgetPtr->Implements<UMounteaDialogueWBPInterface>() == false)
	{
		Message = TEXT("Does not implement Diaogue Widget Interface!");
		return false;
	}

	if (DialogueWidgetPtr->AddToPlayerScreen() == false)
	{
		Message = TEXT("Cannot display Dialogue Widget!");
		return false;
	}
	
	bWasCursorVisible = playerController->bShowMouseCursor;

	// This event should be responsible for calling logic in Player Controller
	OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidgetPtr);
	
	// This Component should not be responsible for setting up Player Controller!
	playerController->SetShowMouseCursor(true);
	DialogueWidgetPtr->bStopAction = true;
	
	return Execute_UpdateDialogueUI(this, Message, MounteaDialogueWidgetCommands::CreateDialogueWidget);
}

bool UMounteaDialogueManager::UpdateDialogueUI_Implementation(FString& Message, const FString& Command)
{
	if (!DialogueWidgetPtr)
	{
		LOG_ERROR(TEXT("[Dialogue Command Request] Invalid Dialogue Widget!"))

		return false;
	}

	
	LOG_INFO(TEXT("[Dialogue Command Requested] %s"), *Command)
	
	if (DialogueWidgetPtr)
	{
		IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, Command);
		return true;
	}
	
	return true;
}

bool UMounteaDialogueManager::CloseDialogueUI_Implementation()
{
	APlayerController* playerController = UMounteaDialogueSystemBFC::FindPlayerController(GetOwner());
	if (playerController == nullptr)
	{
		LOG_ERROR(TEXT("[CloseDialogueUI] No Player Controller Found!"))
		return false;
	}
	
	if (!DialogueWidgetPtr)
	{
		LOG_ERROR(TEXT("[CloseDialogueUI] No Dialogue UI Found!"))
		return false;
	}

	playerController->SetShowMouseCursor(bWasCursorVisible);

	if (DialogueWidgetPtr->Implements<UMounteaDialogueWBPInterface>())
	{
		FString errorMessage;
		Execute_UpdateDialogueUI(this, errorMessage, MounteaDialogueWidgetCommands::CloseDialogueWidget);

		DialogueWidgetPtr = nullptr;

		return true;
	}
	
	LOG_WARNING(TEXT("[CloseDialogueUI] Using non-supported Dialogue UI, therefore brute-forcing is used to close the widget!"))
	
	DialogueWidgetPtr->RemoveFromParent();
	DialogueWidgetPtr = nullptr;

	return true;
}

void UMounteaDialogueManager::ExecuteWidgetCommand_Implementation(const FString& Command)
{
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		if (GetOwner()->HasAuthority())
		{
			FString resultMessage;
			Execute_UpdateDialogueUI(this, resultMessage, Command);
		}
		else
		{
			UpdateDialogueUI_Client(Command);
		}
	}
	else
	{
		UpdateDialogueUI_Client(Command);
	}
}

void UMounteaDialogueManager::InvokeDialogueUI_Client_Implementation()
{
	FString ErrorMessage;
	if (!Execute_InvokeDialogueUI(this, ErrorMessage))
	{
		LOG_ERROR(TEXT("[InvokeDialogueUI_Client] %s"), *ErrorMessage)
	}
}

void UMounteaDialogueManager::CloseDialogueUI_Client_Implementation()
{
	Execute_CloseDialogueUI(this);
}

void UMounteaDialogueManager::UpdateDialogueUI_Client_Implementation(const FString& Command)
{
	FString errorMessage;
	if (!Execute_UpdateDialogueUI(this, errorMessage, Command))
	{
		LOG_ERROR(TEXT("%s"), *errorMessage)
	}
}

void UMounteaDialogueManager::StartDialogue_Server_Implementation()
{
	Execute_StartDialogue(this);
}

void UMounteaDialogueManager::CloseDialogue_Server_Implementation()
{
	Execute_CloseDialogue(this);
}

void UMounteaDialogueManager::InitializeDialogue_Server_Implementation(APlayerState* OwningPlayerState, const FDialogueParticipants& Participants)
{
	Execute_InitializeDialogue(this, OwningPlayerState, Participants);
}

#pragma endregion

void UMounteaDialogueManager::PostUIInitialized_Implementation()
{
	Execute_PrepareNode(this);
}

void UMounteaDialogueManager::UpdateDialogueContext_Client_Implementation(const FMounteaDialogueContextReplicatedStruct& NewDialogueContext)
{
	if (NewDialogueContext.IsValid())
	{
		if (!DialogueContext)
		{
			LOG_WARNING(TEXT("[UpdateDialogueContext] No Context available, creating new local one."))
			DialogueContext = NewObject<UMounteaDialogueContext>();
		}
	
		if (DialogueContext)
		{
			DialogueContext->ActiveDialogueParticipant = NewDialogueContext.ActiveDialogueParticipant;
			DialogueContext->PlayerDialogueParticipant = NewDialogueContext.PlayerDialogueParticipant;
			DialogueContext->DialogueParticipant = NewDialogueContext.DialogueParticipant;
			DialogueContext->DialogueParticipants = NewDialogueContext.DialogueParticipants;
			DialogueContext->ActiveDialogueRowDataIndex = NewDialogueContext.ActiveDialogueRowDataIndex;

			// Find Active Node
			DialogueContext->ActiveNode = UMounteaDialogueSystemBFC::FindNodeByGUID(DialogueContext->DialogueParticipant->GetDialogueGraph(), NewDialogueContext.ActiveNodeGuid);

			// Find child Nodes
			DialogueContext->AllowedChildNodes = UMounteaDialogueSystemBFC::FindNodesByGUID(DialogueContext->DialogueParticipant->GetDialogueGraph(), NewDialogueContext.AllowedChildNodes);

			// Find Previous Active Node
			DialogueContext->PreviousActiveNode = NewDialogueContext.PreviousActiveNodeGuid;

			// Find data locally
			DialogueContext->ActiveDialogueRow = UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode);
		}
	}
	else
	{
		if (DialogueContext)
		{
			DialogueContext->MarkAsGarbage();
			DialogueContext = nullptr;
		}
	}
}

void UMounteaDialogueManager::CallDialogueNodeSelected_Server_Implementation(const FGuid& NodeGuid)
{
	Execute_CallDialogueNodeSelected(this, NodeGuid);
}

void UMounteaDialogueManager::RequestVoiceStart_Client_Implementation(USoundBase* SoundBase)
{
	OnDialogueVoiceStartRequest.Broadcast(SoundBase);
}

void UMounteaDialogueManager::RequestVoiceStop_Client_Implementation(USoundBase* SoundBase)
{
	if (DialogueContext)
	{
		DialogueContext->ActiveDialogueParticipant->Execute_SkipParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), SoundBase);
	}
	
	OnDialogueVoiceSkipRequestEvent(SoundBase);
}

void UMounteaDialogueManager::StartExecuteDialogueRow_Client_Implementation()
{
	const int32 Index = DialogueContext->GetActiveDialogueRowDataIndex();
	const auto Row = DialogueContext->GetActiveDialogueRow();
	const auto RowData = Row.DialogueRowData.Array()[Index];
	
	if (DialogueContext->DialogueParticipant)
	{
		if (UMounteaDialogueSystemBFC::DoesPreviousNodeSkipActiveNode(DialogueContext->DialogueParticipant->GetDialogueGraph(), DialogueContext->PreviousActiveNode))
		{
			Execute_FinishedExecuteDialogueRow(this);

			return;
		}
	}
	
	if (RowData.RowDurationMode != ERowDurationMode::ERDM_Manual)
	{
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &UMounteaDialogueManager::FinishedExecuteDialogueRow_Implementation);
		
		GetWorld()->GetTimerManager().SetTimer
		(
			TimerHandle_RowTimer,
			Delegate,
			UMounteaDialogueSystemBFC::GetRowDuration(RowData),
			false
		);
	}
}

void UMounteaDialogueManager::FinishedExecuteDialogueRow_Server_Implementation()
{
	Execute_FinishedExecuteDialogueRow(this);
}

void UMounteaDialogueManager::TriggerNextDialogueRow_Server_Implementation()
{
	Execute_TriggerNextDialogueRow(this);
}

void UMounteaDialogueManager::OnRep_ManagerState()
{
	switch (ManagerState)
	{
		case EDialogueManagerState::EDMS_Disabled:
			Execute_CloseDialogueUI(this);
			break;
		case EDialogueManagerState::EDMS_Enabled:
			Execute_CloseDialogueUI(this);
			break;
		case EDialogueManagerState::EDMS_Active:
			FString resultMessage;
			Execute_InvokeDialogueUI(this, resultMessage);
			PostUIInitialized();
			break;
	}
}

/*
void UMounteaDialogueManager::OnRep_DialogueContext()
{
	if (!DialogueContext)
	{
		LOG_WARNING(TEXT("[OnRep_DialogueContext] No Context available, creating new local one. This shout not happen!"))
		DialogueContext = NewObject<UMounteaDialogueContext>();
	}
	
	if (DialogueContext)
	{
		DialogueContext->ActiveDialogueParticipant = ReplicatedDialogueContext.ActiveDialogueParticipant;
		DialogueContext->PlayerDialogueParticipant = ReplicatedDialogueContext.PlayerDialogueParticipant;
		DialogueContext->DialogueParticipant = ReplicatedDialogueContext.DialogueParticipant;
		DialogueContext->ActiveNode = ReplicatedDialogueContext.ActiveNode;
		DialogueContext->AllowedChildNodes = ReplicatedDialogueContext.AllowedChildNodes;
		DialogueContext->ActiveDialogueRowDataIndex = ReplicatedDialogueContext.ActiveDialogueRowDataIndex;
		DialogueContext->DialogueWidgetCommand = ReplicatedDialogueContext.DialogueWidgetCommand;

		// Find data locally
		DialogueContext->ActiveDialogueRow = UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode);
	}
}
*/

void UMounteaDialogueManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMounteaDialogueManager, ManagerState, COND_InitialOrOwner);
}

bool UMounteaDialogueManager::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bUpdated = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	/*
	if (Channel->KeyNeedsToReplicate(0, DialogueContextReplicationKey))
	{
		if (DialogueContext && Channel->KeyNeedsToReplicate(DialogueContext->GetUniqueID(), DialogueContext->GetRepKey()))
		{
			bUpdated |= Channel->ReplicateSubobject(DialogueContext, *Bunch, *RepFlags);
		}
	}
	*/

	return bUpdated;
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
