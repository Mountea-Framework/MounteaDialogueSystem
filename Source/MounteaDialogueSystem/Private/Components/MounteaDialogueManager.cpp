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

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		OnDialogueStartRequested.AddUniqueDynamic(this, &UMounteaDialogueManager::DialogueStartRequestReceived);
	}
}

void UMounteaDialogueManager::OnRep_ManagerState()
{
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

void UMounteaDialogueManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMounteaDialogueManager, ManagerState, COND_InitialOrOwner);
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
	
	if (GetOwner() && !GetOwner()->HasAuthority())
		SetManagerState_Server(NewState);
	else
	{
		ManagerState = NewState; // State can only be changed on server side!
	}

	OnDialogueManagerStateChanged.Broadcast(NewState);
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
		LOG_INFO(TEXT("[Set Default Manager State] New State `%s` is same as current State. Update aborted."), *(UMounteaDialogueSystemBFC::GetEnumFriendlyName(NewState)))
		return;
	}
	
	if (GetOwner() && !GetOwner()->HasAuthority())
		SetManagerState_Server(NewState);
	
	DefaultManagerState = NewState;
}

EDialogueManagerType UMounteaDialogueManager::GetDialogueManagerType() const
{
	return (GetOwner() && GetOwner()->StaticClass()->IsChildOf(APlayerState::StaticClass()))
	? EDialogueManagerType::EDMT_PlayerDialogue
	: EDialogueManagerType::EDMT_EnvironmentDialogue;
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

	if (GetOwner() && !GetOwner()->HasAuthority())
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
	
	if (GetOwner() && !GetOwner()->HasAuthority())
		UpdateDialogueContext_Server(NewContext);

	DialogueContext->UpdateDialogueContext(NewContext);

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

		if (!mainParticipant->Execute_CanStartDialogue(InitialParticipants.MainParticipant))
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

		// TODO: construct DialogueContext here
	}

	// Request Start on Server
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		RequestStartDialogue_Server(DialogueInitiator, InitialParticipants);
	}

	const FText finalErrorMessage = FText::Join(FText::FromString("\n"), errorMessages);
	OnDialogueStartRequested.Broadcast(bSatisfied, finalErrorMessage.ToString());
}

void UMounteaDialogueManager::RequestStartDialogue_Server_Implementation(AActor* DialogueInitiator,const FDialogueParticipants& InitialParticipants)
{
	Execute_RequestStartDialogue(this, DialogueInitiator, InitialParticipants);
}

void UMounteaDialogueManager::RequestCloseDialogue_Implementation()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		SetManagerState(DefaultManagerState); // Let's close Dialogue by changing state
	}

	// Don't wait for Server, close dialogue anyways?
	// Execute_CloseDialogue(this);
}

void UMounteaDialogueManager::DialogueStartRequestReceived(const bool bResult, const FString& ResultMessage)
{
	if (bResult)
	{
		SetManagerState(EDialogueManagerState::EDMS_Active);
	}
	else
	{
		SetManagerState(DefaultManagerState);
		OnDialogueFailed.Broadcast(ResultMessage);
	}
}

void UMounteaDialogueManager::StartDialogue_Implementation()
{
	FString resultMessage;
	Execute_CreateDialogueUI(this, resultMessage);

	// Start Node Loop
}

void UMounteaDialogueManager::CloseDialogue_Implementation()
{
	// Unbind all UI Objects
	Execute_CloseDialogueUI(this);

	// Close Node Loop
}


void UMounteaDialogueManager::UpdateWorldDialogueUI_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, FString& Message, const FString& Command)
{
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






