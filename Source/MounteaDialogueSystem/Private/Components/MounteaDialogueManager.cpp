// All rights reserved Dominik Pavlicek 2023

#include "Components/MounteaDialogueManager.h"

#include "TimerManager.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Components/MounteaDialogueDialogueNetSync.h"

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
	CalculateManagerType();
	
	// Force replicate Owner to avoid setup issues with less experienced users
	const auto owningActor = GetOwner();
	if (IsValid(owningActor) && !owningActor->GetIsReplicated() && GetIsReplicated())
	{
		GetOwner()->SetReplicates(true);
	}
	
	if (IsAuthority())
	{
		OnDialogueStartRequestedResult.AddUniqueDynamic(this, &UMounteaDialogueManager::DialogueStartRequestReceived);
	}

	OnDialogueFailed.AddUniqueDynamic(this, &UMounteaDialogueManager::DialogueFailed);

	// Binding Broadcasting Events
	if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
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
	DOREPLIFETIME(UMounteaDialogueManager, TransientDialogueContext);
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
	{
		switch (DialogueManagerType)
		{
			case EDialogueManagerType::EDMT_PlayerDialogue:
				SetManagerState_Server(NewState);
				break;
			case EDialogueManagerType::EDMT_EnvironmentDialogue:
				SetManagerState_Environment(NewState);
				break;
			case EDialogueManagerType::Default:
				break;
		}
	}
	else
	{
		ManagerState = NewState; // State can only be changed on server side!
		ProcessStateUpdated();
	}
}

void UMounteaDialogueManager::OnRep_ManagerState()
{
	OnDialogueManagerStateChanged.Broadcast(ManagerState);
	ProcessStateUpdated();
}

void UMounteaDialogueManager::ProcessStateUpdated()
{
	if (IsAuthority() && !UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
	{
		return;
	}

	// Await the Context
	if (ManagerState == EDialogueManagerState::EDMS_Active && (!IsValid(DialogueContext)))
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UMounteaDialogueManager::ProcessStateUpdated);
		return;
	}
	
	OnDialogueManagerStateChanged.Broadcast(ManagerState);

	LOG_WARNING(TEXT("State Updated"))
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

void UMounteaDialogueManager::OnRep_DialogueContext()
{
	if (!IsValid(DialogueContext))
		DialogueContext = NewObject<UMounteaDialogueContext>(this);
		
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> participants = TransientDialogueContext.DialogueParticipants;

	*DialogueContext += TransientDialogueContext;
	
	NotifyParticipants(participants);

	FTimerHandle TimerHandle_ResetContext;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_ResetContext, [this]()
	{
		TransientDialogueContext.Reset();
	}, 0.2f, false);

	FTimerHandle TimerHandle_UpdateWorldWidget;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_UpdateWorldWidget, [this]()
	{
		if (IsValid(DialogueContext))
			ProcessWorldWidgetUpdate(DialogueContext->LastWidgetCommand);
	}, 0.1f, false);
}

void UMounteaDialogueManager::RequestStartDialogue_Environment(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants)
{
	int32 searchDepth = 0;
	APlayerController* playerController = UMounteaDialogueSystemBFC::FindPlayerController(DialogueInitiator, searchDepth);
	if (!IsValid(playerController))
	{
		OnDialogueFailed.Broadcast(FString::Printf(TEXT("[Request Start Dialogue] Environmental Dialogue cannot find Player Controller for Initiator %s"), DialogueInitiator ? *DialogueInitiator->GetName() : TEXT("INVALID")));
		return;
	}

	auto netSync = playerController->FindComponentByClass<UMounteaDialogueDialogueNetSync>();
	if (!IsValid(netSync))
	{
		OnDialogueFailed.Broadcast(FString::Printf(TEXT("[Request Start Dialogue] Environmental Dialogue cannot find valid Sync Component in Player Controller %s"), *DialogueInitiator->GetName()));
		return;
	}
	
	netSync->ReceiveStartRequest(this, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueManager::SetManagerState_Environment(const EDialogueManagerState NewState)
{
	int32 searchDepth = 0;
	APlayerController* playerController = UMounteaDialogueSystemBFC::FindPlayerController(Cast<AActor>(DialogueInstigator), searchDepth);
	if (!IsValid(playerController))
	{
		OnDialogueFailed.Broadcast(FString::Printf(TEXT("[Request Start Dialogue] Environmental Dialogue cannot find Player Controller for Initiator %s"), DialogueInstigator ? *DialogueInstigator->GetName() : TEXT("INVALID")));
		return;
	}

	auto netSync = playerController->FindComponentByClass<UMounteaDialogueDialogueNetSync>();
	if (!IsValid(netSync))
	{
		OnDialogueFailed.Broadcast(FString::Printf(TEXT("[Request Start Dialogue] Environmental Dialogue cannot find valid Sync Component in Player Controller %s"), *DialogueInstigator->GetName()));
		return;
	}
	
	netSync->ReceiveSetState(this, NewState);
}

bool UMounteaDialogueManager::SetupPlayerDialogue(TSet<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants, TArray<FText>& ErrorMessages) const
{
	int searchDepth = 0;
	APawn* playerPawn = UMounteaDialogueSystemBFC::FindPlayerPawn(GetOwner(), searchDepth);
	if (!playerPawn)
	{
		ErrorMessages.Add(NSLOCTEXT("RequestStartDialogue", "NoPawn", "Unable to find Player Pawn!"));
		return false;
	}

	bool bPlayerParticipantFound = true;
	const TScriptInterface<IMounteaDialogueParticipantInterface> playerParticipant = 
		UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(playerPawn, bPlayerParticipantFound);
    
	if (!bPlayerParticipantFound || !playerParticipant.GetObject())
	{
		ErrorMessages.Add(NSLOCTEXT("RequestStartDialogue", "InvalidPawn", "Player Pawn doesn't have `Dialogue Participant` component or doesn't implement the `IMounteaDialogueParticipantInterface`!"));
		return false;
	}

	DialogueParticipants.Add(playerParticipant);
	return true;
}

bool UMounteaDialogueManager::SetupEnvironmentDialogue(AActor* DialogueInitiator, const TSet<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants, TArray<FText>& ErrorMessages)
{
	int searchDepth = 0;
	APlayerController* playerController = UMounteaDialogueSystemBFC::FindPlayerController(DialogueInitiator, searchDepth);
	if (!playerController)
	{
		ErrorMessages.Add(NSLOCTEXT("RequestStartDialogue", "NoPawn", "Unable to find Player Controller!"));
		return false;
	}
	
	UMounteaDialogueDialogueNetSync* netSync = playerController->FindComponentByClass<UMounteaDialogueDialogueNetSync>();
	if (!netSync)
	{
		ErrorMessages.Add(NSLOCTEXT("RequestStartDialogue", "NoNetSync", "Unable to find NetSync component on Player Controller!"));
		return false;
	}

	return true;
}

bool UMounteaDialogueManager::ValidateMainParticipant(AActor* MainParticipant, TScriptInterface<IMounteaDialogueParticipantInterface>& OutParticipant, TArray<FText>& ErrorMessages)
{
	bool bFound = true;
	OutParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(MainParticipant, bFound);
    
	if (!bFound || !OutParticipant.GetObject())
	{
		ErrorMessages.Add(NSLOCTEXT("RequestStartDialogue", "InvalidParticipant", "Main Participant doesn't have `Dialogue Participant` component or doesn't implement the `IMounteaDialogueParticipantInterface`!"));
		return false;
	}

	if (!OutParticipant->Execute_CanStartDialogue(OutParticipant.GetObject()))
	{
		ErrorMessages.Add(NSLOCTEXT("RequestStartDialogue", "ParticipantCannotStart", "Main Participant Cannot Start Dialogue!"));
		return false;
	}

	return true;
}

void UMounteaDialogueManager::GatherOtherParticipants(const TArray<TObjectPtr<UObject>>& OtherParticipants, TSet<TScriptInterface<IMounteaDialogueParticipantInterface>>& OutParticipants)
{
	for (const auto& Participant : OtherParticipants)
	{
		if (!IsValid(Participant))
			continue;

		bool bFound = true;
		const auto NewParticipant = UMounteaDialogueSystemBFC::FindDialogueParticipantInterface(Participant, bFound);
		if (bFound && NewParticipant->Execute_CanParticipateInDialogue(NewParticipant.GetObject()))
			OutParticipants.Add(NewParticipant);
	}
}

void UMounteaDialogueManager::RequestBroadcastContext(UMounteaDialogueContext* Context)
{
	if (!IsAuthority())
		RequestBroadcastContext_Server(FMounteaDialogueContextReplicatedStruct(Context));
	else
		ProcessContextUpdated(FMounteaDialogueContextReplicatedStruct(Context));
}

void UMounteaDialogueManager::RequestBroadcastContext_Server_Implementation(const FMounteaDialogueContextReplicatedStruct& Context)
{
	ProcessContextUpdated(Context);
}

void UMounteaDialogueManager::ProcessContextUpdated(const FMounteaDialogueContextReplicatedStruct& Context)
{
	TransientDialogueContext = FMounteaDialogueContextReplicatedStruct(Context);
	MARK_PROPERTY_DIRTY_FROM_NAME(UMounteaDialogueManager, TransientDialogueContext, this);
	*DialogueContext += TransientDialogueContext;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
	{
		NotifyParticipants(TransientDialogueContext.DialogueParticipants);
	}, 0.1f, false);
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

void UMounteaDialogueManager::CalculateManagerType()
{
	auto ownerActor = GetOwner();
	if (!IsValid(ownerActor))
		DialogueManagerType = EDialogueManagerType::Default;

	auto ownerClass = ownerActor->GetClass();
	
	if (ownerClass->IsChildOf(APawn::StaticClass()) || 
		ownerClass->IsChildOf(APlayerState::StaticClass()) || 
		ownerClass->IsChildOf(APlayerController::StaticClass()))
	{
		DialogueManagerType = EDialogueManagerType::EDMT_PlayerDialogue;
		return;
	}

	DialogueManagerType = EDialogueManagerType::EDMT_EnvironmentDialogue;
}

bool UMounteaDialogueManager::IsAuthority() const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetWorld())
		return false;
    
	const ENetMode NetMode = Owner->GetWorld()->GetNetMode();
	
	if (NetMode == NM_Standalone)
		return true;
	
	if (Owner->HasAuthority())
		return true;
        
	return false;
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
	if (NewContext == DialogueContext) return;
	
	if (!IsAuthority())
		SetDialogueContext_Server(NewContext);

	DialogueContext = NewContext;

	TransientDialogueContext = FMounteaDialogueContextReplicatedStruct(DialogueContext);

	OnDialogueContextUpdated.Broadcast(NewContext);
}

void UMounteaDialogueManager::SetDialogueContext_Server_Implementation(UMounteaDialogueContext* NewContext)
{
	SetDialogueContext(NewContext);
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

	TSet<TScriptInterface<IMounteaDialogueParticipantInterface>> dialogueParticipants;
	TScriptInterface<IMounteaDialogueParticipantInterface> mainParticipant;

	if (!Execute_CanStartDialogue(this))
	{
		errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "CannotStart", "Cannot Start Dialogue!"));
		bSatisfied = false;
	}

	if (ValidateMainParticipant(InitialParticipants.MainParticipant, mainParticipant, errorMessages))
	{
		dialogueParticipants.Add(mainParticipant);
		GatherOtherParticipants(InitialParticipants.OtherParticipants, dialogueParticipants);
	}
	else
		bSatisfied = false;

	bool bSetupSuccess = false;
	switch (DialogueManagerType)
	{
		case EDialogueManagerType::EDMT_PlayerDialogue:
			bSetupSuccess = SetupPlayerDialogue(dialogueParticipants, errorMessages);
			break;
		case EDialogueManagerType::EDMT_EnvironmentDialogue:
			bSetupSuccess = SetupEnvironmentDialogue(DialogueInitiator, dialogueParticipants, errorMessages);
			break;
		default:
			errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "WrongManager", "This Manager Type is not valid!"));
			bSetupSuccess = false;
			break;
	}
	bSatisfied &= bSetupSuccess;

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
		if (IsAuthority())
			SetDialogueContext(UMounteaDialogueSystemBFC::CreateDialogueContext(this, mainParticipant, dialogueParticipants.Array()));
		errorMessages.Add(NSLOCTEXT("RequestStartDialogue", "OK", "OK"));
	}
	const FText finalErrorMessage = FText::Join(FText::FromString("\n"), errorMessages);
	
	if (bSatisfied)
	{
		DialogueInstigator = DialogueInitiator;
		
		// Request Start on Server
		switch (DialogueManagerType)
		{
			case EDialogueManagerType::EDMT_PlayerDialogue:
				if (!IsAuthority())
					RequestStartDialogue_Server(DialogueInitiator, InitialParticipants);
				break;
			case EDialogueManagerType::EDMT_EnvironmentDialogue:
				if (!IsAuthority())
					RequestStartDialogue_Environment(DialogueInitiator, InitialParticipants);
				break;
		}

		OnDialogueStartRequestedResult.Broadcast(bSatisfied, finalErrorMessage.ToString());
	}
	else
		OnDialogueFailed.Broadcast(finalErrorMessage.ToString());
}

void UMounteaDialogueManager::RequestStartDialogue_Server_Implementation(AActor* DialogueInitiator,const FDialogueParticipants& InitialParticipants)
{
	Execute_RequestStartDialogue(this, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueManager::RequestCloseDialogue_Implementation()
{
	if (!IsAuthority())
		SetManagerState(DefaultManagerState); // Let's close Dialogue by changing state
	
	Execute_CloseDialogue(this);
}

void UMounteaDialogueManager::StartParticipants()
{
	if (!IsValid(DialogueContext))
		return;
	
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
		dialogueParticipant->Execute_InitializeParticipant(dialogueParticipant.GetObject(), this);
	}
}

void UMounteaDialogueManager::StartParticipants_Server_Implementation()
{
	StartParticipants();
}

void UMounteaDialogueManager::StopParticipants() const
{
	if (!IsValid(DialogueContext))
		return;

	if (!IsAuthority())
		StopParticipants_Server();
	
	for (const auto& dialogueParticipant : DialogueContext->DialogueParticipants)
	{
		auto participantObject = dialogueParticipant.GetObject();
		if (!IsValid(participantObject) || !dialogueParticipant.GetInterface()) continue;
		
		TScriptInterface<IMounteaDialogueTickableObject> tickableObject = dialogueParticipant.GetObject();
		if (tickableObject.GetInterface() && tickableObject.GetObject())
		{
			// Register ticks for participants, no need to define Parent as Participants are the most paren ones
			tickableObject->Execute_UnregisterTick(tickableObject.GetObject(), nullptr);
		}
		
		dialogueParticipant->Execute_SetParticipantState(participantObject, dialogueParticipant->Execute_GetDefaultParticipantState(participantObject));
	}
}

void UMounteaDialogueManager::StopParticipants_Server_Implementation() const
{
	StopParticipants();
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
	// TODO: this might lead to infinite loop! Implement safety check
	if (!IsAuthority() && !UMounteaDialogueSystemBFC::IsContextValid(DialogueContext))
	{
		FTimerHandle TimerHandle_AwaitContext;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_AwaitContext, [this]()
		{
			Execute_StartDialogue(this);
		}, 0.1f, false);
		return;
	}
	
	StartParticipants();
		
	FString resultMessage;
	if (!Execute_CreateDialogueUI(this, resultMessage))
		LOG_WARNING(TEXT("[Create Dialogue UI] %s"), *(resultMessage))

	if (!IsAuthority())
		OnDialogueStarted.Broadcast(DialogueContext);
	
	Execute_PrepareNode(this);
}

void UMounteaDialogueManager::CloseDialogue_Implementation()
{
	StopParticipants();
	
	Execute_CloseDialogueUI(this);
	
	Execute_CleanupDialogue(this);

	SetDialogueContext(nullptr);

	DialogueInstigator = nullptr;

	if (!IsAuthority())
	{
		OnDialogueClosed.Broadcast(DialogueContext);
	}

	if (GetOwner() && GetDialogueManagerType() == EDialogueManagerType::EDMT_EnvironmentDialogue)
	{
		GetOwner()->SetOwner(nullptr);
	}
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
		dialogueGraph->ShutdownGraph();
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
		auto newActiveDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(newActiveNode);
		auto allowedChildNodes = UMounteaDialogueSystemBFC::GetAllowedChildNodes(newActiveNode);
		UMounteaDialogueSystemBFC::SortNodes(allowedChildNodes);
		
		if (const auto selectedDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(newActiveNode))
		{
			FDataTableRowHandle newDialogueTableHandle = FDataTableRowHandle();
			newDialogueTableHandle.DataTable = selectedDialogueNode->GetDataTable();
			newDialogueTableHandle.RowName = selectedDialogueNode->GetRowName();
		
			DialogueContext->UpdateActiveDialogueTable(newActiveNode ? newDialogueTableHandle : FDataTableRowHandle());
		}
	
		DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, newActiveNode, allowedChildNodes);
		DialogueContext->UpdateActiveDialogueRow(UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode));
		DialogueContext->UpdateActiveDialogueRowDataIndex(0);
		const auto newActiveParticipant = UMounteaDialogueSystemBFC::SwitchActiveParticipant(DialogueContext);
		UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(DialogueContext, newActiveParticipant);
		
		OnDialogueNodeSelected.Broadcast(DialogueContext);

		Execute_PrepareNode(this);
	}
	else
	{
		FString resultMessage;
		if (!Execute_UpdateDialogueUI(this, resultMessage, MounteaDialogueWidgetCommands::AddDialogueOptions))
			LOG_INFO(TEXT("[Node Selected] UpdateUI Message: %s"), *resultMessage)
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
	
	DialogueContext->SetDialogueContext(DialogueContext->DialogueParticipant, selectedNode, allowedChildNodes);
	DialogueContext->UpdateActiveDialogueRow(UMounteaDialogueSystemBFC::GetDialogueRow(DialogueContext->ActiveNode));
	DialogueContext->UpdateActiveDialogueRowDataIndex(0);
	const auto newActiveParticipant = UMounteaDialogueSystemBFC::SwitchActiveParticipant(DialogueContext);
	UMounteaDialogueSystemBFC::UpdateMatchingDialogueParticipant(DialogueContext, newActiveParticipant);

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
		Execute_DialogueRowProcessed(this, false);
		return;
	}
	
	const FDialogueRowData RowData = Row.DialogueRowData.Array()[activeIndex];
	bValidRowData = UMounteaDialogueSystemBFC::IsDialogueRowDataValid(RowData);

	if (!bValidRowData)
	{
		LOG_WARNING(TEXT("[Process Dialogue Row] Invalid Dialogue Row Data! Skipping Row. Next Row will be processed instead."))
		Execute_DialogueRowProcessed(this, false);
		return;
	}

	OnDialogueRowStarted.Broadcast(DialogueContext);
	
	DialogueContext->ActiveDialogueParticipant->Execute_PlayParticipantVoice(DialogueContext->ActiveDialogueParticipant.GetObject(), RowData.RowSound);

	if (bValidRowData)
	{
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
					Execute_ProcessDialogueRow(this); // Continue in the loop, just with another row
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
	if (!IsAuthority())
	{
		ProcessWorldWidgetUpdate(Command);
	}
	else
	{
		if (UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(GetWorld()))
			ProcessWorldWidgetUpdate(Command);
	}
}

void UMounteaDialogueManager::ProcessWorldWidgetUpdate(const FString& Command)
{
	if (!IsAuthority())
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