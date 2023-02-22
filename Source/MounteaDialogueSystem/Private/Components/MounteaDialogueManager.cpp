// All rights reserved Dominik Pavlicek 2023


#include "Components/MounteaDialogueManager.h"

#include "TimerManager.h"

#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/MounteaDialogueWBPInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"

UMounteaDialogueManager::UMounteaDialogueManager()
{
	DialogueContext = nullptr;
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
}

void UMounteaDialogueManager::OnDialogueInitializedEvent_Internal(UMounteaDialogueContext* Context)
{
	if (Context)
	{
		OnDialogueInitializedEvent(Context);

		OnDialogueContextUpdated.Broadcast(Context);

		OnDialogueStarted.Broadcast(Context);
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
	OnDialogueClosedEvent(DialogueContext);

	CloseDialogue();
}

void UMounteaDialogueManager::OnDialogueNodeSelectedEvent_Internal(UMounteaDialogueContext* Context)
{
	OnDialogueNodeSelectedEvent(Context);

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

	if (bAutoActive)
	{
		const auto NewActiveNode = AllowedChildrenDialogueNodes[0];

		if (!NewActiveNode)
		{
			OnDialogueClosed.Broadcast(DialogueContext);	
		}
		
		DialogueContext->SetDialogueContext(NewActiveNode, UMounteaDialogueSystemBFC::GetAllowedChildNodes(NewActiveNode));
		DialogueContext->UpdateActiveDialogueRowDataIndex(0);
		
		OnDialogueNodeSelected.Broadcast(DialogueContext);
		return;
	}
	else
	{
		TScriptInterface<IMounteaDialogueWBPInterface> WidgetInterface;
		WidgetInterface.SetObject(DialogueWidgetPtr);
		WidgetInterface.SetInterface(DialogueWidgetPtr);
		WidgetInterface->Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::AddDialogueOptions);

		return;
	}
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
	
	//TODO: Add ability to start from specific Node
	ProcessNode();
}

void UMounteaDialogueManager::CloseDialogue()
{
	//TODO: Close dialogue, destroy Context, clean Timer
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RowTimer);

	if(!DialogueContext) return;

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

	if (UMounteaDialogueGraphNode_DialogueNodeBase* DialogueLeadNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(DialogueContext->ActiveNode) )
	{
		ProcessNode_Dialogue();
		return;
	}
	
	if (UMounteaDialogueGraphNode_CompleteNode* CompleteNode = Cast<UMounteaDialogueGraphNode_CompleteNode>(DialogueContext->ActiveNode) )
	{
		ProcessNode_Complete();
		return;
	}
}

void UMounteaDialogueManager::ProcessNode_Complete()
{
	OnDialogueClosed.Broadcast(DialogueContext);
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
		DialogueContext->UpdateActiveDialogueRowDataIndex(0);
		OnDialogueContextUpdated.Broadcast(DialogueContext);
		
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
	
	TScriptInterface<IMounteaDialogueWBPInterface> WidgetInterface;
	WidgetInterface.SetObject(DialogueWidgetPtr);
	WidgetInterface.SetInterface(DialogueWidgetPtr);

	if (WidgetInterface.GetInterface() == nullptr)
	{
		Message = TEXT("Invalid Widget Interface!");
		return false;
	}

	WidgetInterface->Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::CreateDialogueWidget);
	
	return true;
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

void UMounteaDialogueManager::SetDialogueWidgetClass(TSubclassOf<UUserWidget> NewWidgetClass)
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
			TScriptInterface<IMounteaDialogueWBPInterface> WidgetInterface;
			WidgetInterface.SetObject(DialogueWidgetPtr);
			WidgetInterface.SetInterface(DialogueWidgetPtr);
			WidgetInterface->Execute_RefreshDialogueWidget(DialogueWidgetPtr, this, MounteaDialogueWidgetCommands::ShowDialogueRow);
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