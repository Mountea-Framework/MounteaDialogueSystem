// All rights reserved Dominik Pavlicek 2023

#include "Components/MounteaDialogueManager.h"

#include "TimerManager.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/MounteaDialogueWBPInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Nodes/MounteaDialogueGraphNode_AutoCompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"

UMounteaDialogueManager::UMounteaDialogueManager()
{
	DialogueContext = nullptr;
	DefaultManagerState = EDialogueManagerState::EDMS_Enabled;
}

void UMounteaDialogueManager::BeginPlay()
{
	Super::BeginPlay();

	OnDialogueInitialized.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueInitializedEvent_Internal);
	
	OnDialogueContextUpdated.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueContextUpdatedEvent_Internal);
	OnDialogueUserInterfaceChanged.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueUserInterfaceChangedEvent_Internal);
	
	OnDialogueStarted.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueStartedEvent_Internal);
	OnDialogueClosed.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueClosedEvent_Internal);

	OnDialogueNodeSelected.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueNodeSelectedEvent_Internal);
	OnDialogueNodeStarted.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueNodeStartedEvent_Internal);
	OnDialogueNodeFinished.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueNodeFinishedEvent_Internal);

	OnDialogueRowStarted.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueRowStartedEvent_Internal);
	OnDialogueRowFinished.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueRowFinishedEvent_Internal);

	OnDialogueVoiceStartRequest.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueVoiceStartRequestEvent_Internal);
	OnDialogueVoiceSkipRequest.AddUniqueDynamic(this, &UMounteaDialogueManager::OnDialogueVoiceSkipRequestEvent_Internal);
	
	SetDialogueManagerState(GetDefaultDialogueManagerState());
}

void UMounteaDialogueManager::CallDialogueNodeSelected_Implementation(const FGuid& NodeGUID)
{
	UMounteaDialogueGraphNode* SelectedNode = nullptr;
	if (DialogueContext)
	{
		for (UMounteaDialogueGraphNode* Itr : DialogueContext->GetChildrenNodes())
		{
			if (Itr && Itr->GetNodeGUID() == NodeGUID)
			{
				SelectedNode = Itr;
				break;
			}
		}
	}

	if (!SelectedNode)
	{
		OnDialogueFailed.Broadcast(TEXT("Cannot find Selected Option!"));
	}
		
	DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, SelectedNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(SelectedNode));
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);
	
	OnDialogueNodeSelected.Broadcast(DialogueContext);
}

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
	StartDialogue();

	OnDialogueStartedEvent(Context);
}

void UMounteaDialogueManager::OnDialogueClosedEvent_Internal(UMounteaDialogueContext* Context)
{
	switch (GetDialogueManagerState())
	{
		case EDialogueManagerState::EDMS_Disabled:
		case EDialogueManagerState::EDMS_Enabled:
			return;
	}
	
	OnDialogueClosedEvent(DialogueContext);

	CloseDialogue();
}

void UMounteaDialogueManager::OnDialogueNodeSelectedEvent_Internal(UMounteaDialogueContext* Context)
{
	OnDialogueNodeSelectedEvent(Context);

	if (DialogueWidgetPtr)
	{
		IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::RemoveDialogueOptions);
	}
	else
	{
		OnDialogueFailed.Broadcast(TEXT("No Dialogue Widget!"));
		return;
	}

	ProcessNode();
}

void UMounteaDialogueManager::OnDialogueNodeStartedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}

	OnDialogueNodeStartedEvent(Context);
}

void UMounteaDialogueManager::OnDialogueNodeFinishedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}
	
	OnDialogueNodeFinishedEvent(Context);

	const TArray<UMounteaDialogueGraphNode*> AllowedChildrenNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(Context->ActiveNode);

	if (AllowedChildrenNodes.Num() == 0)
	{
		OnDialogueClosed.Broadcast(Context);
	}

	TArray<UMounteaDialogueGraphNode_CompleteNode*> AllowedChildrenCompleteNodes;
	for (const auto Itr : AllowedChildrenNodes)
	{
		if (UMounteaDialogueGraphNode_CompleteNode* TempNode = Cast<UMounteaDialogueGraphNode_CompleteNode>(Itr))
		{
			AllowedChildrenCompleteNodes.Add(TempNode);
		}
	}
	
	TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> AllowedChildrenDialogueNodes;
	for (const auto Itr : AllowedChildrenNodes)
	{
		if (UMounteaDialogueGraphNode_DialogueNodeBase* TempNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Itr))
		{
			AllowedChildrenDialogueNodes.Add(TempNode);
		}
	}

	// If there are only Complete Nodes left or no DialogueNodes left, just shut it down
	if (AllowedChildrenDialogueNodes.Num() == 0)
	{
		OnDialogueClosed.Broadcast(DialogueContext);
		return;
	}
	
	const bool bAutoActive = AllowedChildrenDialogueNodes[0]->DoesAutoStart();
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);
	
	if (bAutoActive)
	{
		const auto NewActiveNode = AllowedChildrenDialogueNodes[0];

		if (!NewActiveNode)
		{
			OnDialogueClosed.Broadcast(DialogueContext);	
		}
		
		DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, NewActiveNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(NewActiveNode));
		
		OnDialogueNodeSelected.Broadcast(DialogueContext);
		return;
	}
	else
	{
		IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::AddDialogueOptions);
		return;
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

	// Let's hope we are not approaching invalid indexes
	USoundBase* SoundToStart =  Context->GetActiveDialogueRow().DialogueRowData.Array()[Context->GetActiveDialogueRowDataIndex()].RowSound;
	OnDialogueVoiceStartRequest.Broadcast(SoundToStart);
}

void UMounteaDialogueManager::OnDialogueRowFinishedEvent_Internal(UMounteaDialogueContext* Context)
{
	//...
}

void UMounteaDialogueManager::OnDialogueVoiceStartRequestEvent_Internal(USoundBase* VoiceToStart)
{
	if (DialogueContext == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceStartRequestEvent] Invalid Dialogue Context!"));
		return;
	}

	if (DialogueContext->DialogueParticipant.GetInterface() == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceStartRequestEvent] Invalid Dialogue Participant!"));
		return;
	}
	
	DialogueContext->ActiveDialogueParticipant->PlayParticipantVoice(VoiceToStart);
	OnDialogueVoiceStartRequestEvent(VoiceToStart);
}

void UMounteaDialogueManager::OnDialogueVoiceSkipRequestEvent_Internal(USoundBase* VoiceToSkip)
{
	if (DialogueContext == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceSkipRequestEvent] Invalid Dialogue Context!"));
		return;
	}

	if (DialogueContext->DialogueParticipant.GetInterface() == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("[DialogueVoiceSkipRequestEvent] Invalid Dialogue Participant!"));
		return;
	}

	
	DialogueContext->ActiveDialogueParticipant->SkipParticipantVoice(VoiceToSkip);

	OnDialogueVoiceSkipRequestEvent(VoiceToSkip);
	
	FinishedExecuteDialogueRow();
}

bool UMounteaDialogueManager::EvaluateNodeDecorators()
{
	return true;
}

void UMounteaDialogueManager::ExecuteNodeDecorators()
{
	//TODO: Call BFC to execute all from Context
}

void UMounteaDialogueManager::StartDialogue()
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}

	FString ErrorMessage;
	if (!InvokeDialogueUI(ErrorMessage))
	{
		OnDialogueFailed.Broadcast(ErrorMessage);
		return;
	}

	SetDialogueManagerState(EDialogueManagerState::EDMS_Active);
	
	//TODO: Add ability to start from specific Node
	ProcessNode();
}

void UMounteaDialogueManager::CloseDialogue()
{
	if (DialogueWidgetPtr)
	{
		IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::CloseDialogueWidget);
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
	{
		OnDialogueFailed.Broadcast(TEXT("No Player Controller found!"));
		return;
	}

	//TODO: Cache previous settings to return to them
	PlayerController->SetInputMode(FInputModeGameOnly());
	PlayerController->SetShowMouseCursor(false);
	
	//TODO: Close dialogue, destroy Context, clean Timer
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	SetDialogueManagerState(EDialogueManagerState::EDMS_Enabled);

	if(!DialogueContext) return;

	if (!DialogueContext->GetDialogueParticipant().GetObject()) return;

	// Clear binding
	DialogueContext->DialogueContextUpdatedFromBlueprint.RemoveDynamic(this, &UMounteaDialogueManager::OnDialogueContextUpdatedEvent);

	const auto ParticipantDefaultState = DialogueContext->GetDialogueParticipant()->GetDefaultParticipantState();
	const auto PlayerParticipantDefaultState = DialogueContext->GetDialoguePlayerParticipant()->GetDefaultParticipantState();
	
	DialogueContext->GetDialogueParticipant()->SetParticipantState(ParticipantDefaultState);
	DialogueContext->GetDialoguePlayerParticipant()->SetParticipantState(PlayerParticipantDefaultState);
	
	DialogueContext->SetDialogueContext(nullptr, nullptr, TArray<UMounteaDialogueGraphNode*>());
	DialogueContext->ConditionalBeginDestroy();
	DialogueContext = nullptr;
}

void UMounteaDialogueManager::ProcessNode()
{
	if (!DialogueContext)
	{
		OnDialogueFailed.Broadcast(TEXT("Invalid Dialogue Context!"));
		return;
	}

	if (DialogueContext->ActiveNode->GetClass()->IsChildOf(UMounteaDialogueGraphNode_LeadNode::StaticClass()))
	{
		DialogueContext->UpdateActiveDialogueParticipant(DialogueContext->GetDialogueParticipant());
	}
	else
	{
		DialogueContext->UpdateActiveDialogueParticipant(DialogueContext->GetDialoguePlayerParticipant());
	}
	
	ProcessNode_Dialogue();
}

void UMounteaDialogueManager::ProcessNode_Dialogue()
{
	if (!GetWorld())
	{
		OnDialogueFailed.Broadcast(TEXT("Cannot find World!"));
		return;
	}
	
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const FDialogueRow DialogueRow = UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode);
	if (UMounteaDialogueSystemBFC::IsDialogueRowValid(DialogueRow) && DialogueRow.DialogueRowData.Array().IsValidIndex(DialogueContext->GetActiveDialogueRowDataIndex()))
	{
		DialogueContext->UpdateActiveDialogueRow(DialogueRow);
		DialogueContext->UpdateActiveDialogueRowDataIndex(DialogueContext->ActiveDialogueRowDataIndex);
		OnDialogueContextUpdated.Broadcast(DialogueContext);
		
		UMounteaDialogueSystemBFC::ExecuteDecorators(this, DialogueContext);
		
		StartExecuteDialogueRow();
	}
	else
	{
		OnDialogueFailed.Broadcast(TEXT("Dialogue Row data contain Invalid Rows!"));
		return;
	}

	OnDialogueNodeStarted.Broadcast(DialogueContext);
}

bool UMounteaDialogueManager::InvokeDialogueUI(FString& Message)
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

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
	{
		Message = TEXT("Invalid Player Controller!");
		return false;
	}
	
	DialogueWidgetPtr = CreateWidget<UUserWidget>(PlayerController,  GetDialogueWidgetClass());
	if (DialogueWidgetPtr->Implements<UMounteaDialogueWBPInterface>() == false)
	{
		Message = TEXT("Does not implement Diaogue Widget Interface!");
		return false;
	}

	if (DialogueWidgetPtr == nullptr)
	{
		Message = TEXT("Cannot spawn Dialogue Widget!");
		return false;
	}

	if (DialogueWidgetPtr->AddToPlayerScreen() == false)
	{
		Message = TEXT("Cannot display Dialogue Widget!");
		return false;
	}

	// This event should be responsible for calling logic in Player Controller
	OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidgetPtr);
	
	// This Component should not be responsible for setting up Player Controller!
	PlayerController->SetShowMouseCursor(true);
	DialogueWidgetPtr->bStopAction = true;

	IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::CreateDialogueWidget);
	
	return true;
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
	DialogueWidgetClass = NewWidgetClass;

	OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidgetPtr);
}

void UMounteaDialogueManager::SetDialogueUIPtr(UUserWidget* NewDialogueWidgetPtr)
{
	DialogueWidgetPtr = NewDialogueWidgetPtr;

	OnDialogueUserInterfaceChanged.Broadcast(DialogueWidgetClass, DialogueWidgetPtr);
}

void UMounteaDialogueManager::StartExecuteDialogueRow()
{
	if (!DialogueWidgetPtr)
	{
		OnDialogueFailed.Broadcast("Invalid Dialogue Widget Pointer!");
	}
	
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &UMounteaDialogueManager::FinishedExecuteDialogueRow);

	const int32 Index = DialogueContext->GetActiveDialogueRowDataIndex();
	const auto Row = DialogueContext->GetActiveDialogueRow();
	const auto RowData = Row.DialogueRowData.Array()[Index];

	GetWorld()->GetTimerManager().SetTimer
	(
		TimerHandle_RowTimer,
		Delegate,
		UMounteaDialogueSystemBFC::GetRowDuration(RowData),
		false
	);
	
	OnDialogueRowStarted.Broadcast(DialogueContext);

	// Show Subtitle Row only if allowed
	if (UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal())
	{
		if (UMounteaDialogueSystemBFC::GetDialogueSystemSettings_Internal()->SubtitlesAllowed())
		{
			IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::ShowDialogueRow);
		}
	}
}

void UMounteaDialogueManager::FinishedExecuteDialogueRow()
{
	if (!GetWorld())
	{
		OnDialogueFailed.Broadcast(TEXT("Cannot find World!"));
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	const int32 IncreasedIndex = DialogueContext->GetActiveDialogueRowDataIndex() + 1;

	const bool bIsActiveRowValid = UMounteaDialogueSystemBFC::IsDialogueRowValid(DialogueContext->GetActiveDialogueRow());
	const bool bDialogueRowDataValid = DialogueContext->GetActiveDialogueRow().DialogueRowData.Array().IsValidIndex(IncreasedIndex);

	if (bIsActiveRowValid && bDialogueRowDataValid)
	{
		DialogueContext->UpdateActiveDialogueRowDataIndex(IncreasedIndex);
		OnDialogueContextUpdated.Broadcast(DialogueContext);
		
		StartExecuteDialogueRow();
	}
	else
	{
		OnDialogueNodeFinished.Broadcast(DialogueContext);
	}

	OnDialogueRowFinished.Broadcast(DialogueContext);
}

void UMounteaDialogueManager::SetDialogueContext(UMounteaDialogueContext* NewContext)
{
	DialogueContext = NewContext;

	OnDialogueContextUpdatedEvent(DialogueContext);
}

void UMounteaDialogueManager::SetDialogueManagerState(const EDialogueManagerState NewState)
{
	ManagerState = NewState;
	
	OnDialogueManagerStateChanged.Broadcast(NewState);
}

void UMounteaDialogueManager::SetDefaultDialogueManagerState(const EDialogueManagerState NewState)
{
	DefaultManagerState = NewState;
}
