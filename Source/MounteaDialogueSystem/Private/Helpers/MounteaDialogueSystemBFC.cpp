// All rights reserved Dominik Pavlicek 2023


#include "Helpers/MounteaDialogueSystemBFC.h"

#include "Kismet/KismetSystemLibrary.h"

#include "Graph/MounteaDialogueGraph.h"

#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

void UMounteaDialogueSystemBFC::CleanupGraph(const UObject* WorldContextObject, const UMounteaDialogueGraph* GraphToClean)
{
	if (!GraphToClean) return;
		
	// Cleanup Decorators
	for (auto Itr : GraphToClean->GetAllDecorators())
	{
		Itr.CleanupDecorator();
	}
}

void UMounteaDialogueSystemBFC::InitializeDecorators(UWorld* WorldContext, const TScriptInterface<IMounteaDialogueParticipantInterface> Participant)
{
	if (!WorldContext) return;
	if (!Participant) return;

	if (!Participant->GetDialogueGraph()) return;
		
	// Initialize Decorators
	for (auto Itr : Participant->GetDialogueGraph()->GetAllDecorators())
	{
		Itr.InitializeDecorator(WorldContext, Participant);
	}
}

UAudioComponent* UMounteaDialogueSystemBFC::FindAudioComponentByName(const AActor* ActorContext, const FName& Arg)
{
	if (ActorContext == nullptr) return nullptr;

	TArray<UAudioComponent*> OwnerComponents;
	ActorContext->GetComponents<UAudioComponent>(OwnerComponents);
	
	if (OwnerComponents.Num() == 0) return nullptr;

	for (const auto& Itr : OwnerComponents)
	{
		if (Itr && Itr->GetName().Equals(Arg.ToString()))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

UAudioComponent* UMounteaDialogueSystemBFC::FindAudioComponentByTag(const AActor* ActorContext, const FName& Arg)
{
	if (ActorContext == nullptr) return nullptr;

	TArray<UAudioComponent*> OwnerComponents;
	ActorContext->GetComponents<UAudioComponent>(OwnerComponents);
	
	if (OwnerComponents.Num() == 0) return nullptr;
		
	for (const auto& Itr : OwnerComponents)
	{
		if (Itr && Itr->ComponentHasTag(Arg))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

bool UMounteaDialogueSystemBFC::ExecuteDecorators(const UObject* WorldContextObject, const UMounteaDialogueContext* DialogueContext)
{
	if (DialogueContext == nullptr)
	{
		return false;
	}

	if (DialogueContext->DialogueParticipant.GetInterface() == nullptr)
	{
		return false;
	}

	if (DialogueContext->DialogueParticipant->GetDialogueGraph() == nullptr)
	{
		return false;
	}

	const auto ActiveNode = DialogueContext->GetActiveNode();
	if (ActiveNode == nullptr)
	{
		return false;
	}

	// First process Node Decorators, then Graph Decorators
	// TODO: add weight to them so we can sort them by Weight and execute in correct order
	TArray<FMounteaDialogueDecorator> AllDecorators;

	AllDecorators.Append(DialogueContext->GetActiveNode()->GetNodeDecorators());
	if (ActiveNode->DoesInheritDecorators())
	{
		AllDecorators.Append(DialogueContext->DialogueParticipant->GetDialogueGraph()->GetGraphDecorators());
	}
		
	for (auto Itr : AllDecorators)
	{
		Itr.ExecuteDecorator();
	}

	return true;
}

bool UMounteaDialogueSystemBFC::StartDialogue(const UObject* WorldContextObject, APlayerController* Initiator,TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant, TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants)
{
	if (!WorldContextObject)
	{
		LOG_ERROR(TEXT("[StartDialogue] Missing WorldContextObject. Cannot start dialogue."));
		return false;
	}

	if (!Initiator)
	{
		LOG_ERROR(TEXT("[StartDialogue] Missing Initiator Player Contorller. Cannot start dialogue."));
		return false;
	}

	if (MainParticipant.GetInterface() == nullptr || MainParticipant.GetObject() == nullptr)
	{
		LOG_ERROR(TEXT("[StartDialogue] Invalid Main Participant!. Cannot start dialogue."));
		return false;
	}

	if (MainParticipant->Execute_CanStartDialogueEvent(MainParticipant.GetObject()) == false)
	{
		LOG_ERROR(TEXT("[StartDialogue]  Main Participant cannot starti Dialogue!. Cannot start dialogue."));
		return false;
	}

	if (DialogueParticipants.Contains(MainParticipant) == false)
	{
		DialogueParticipants.Add(MainParticipant);
	}

	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> UnavailableParticipants;
	for (const auto Itr : DialogueParticipants)
	{
		if (Itr.GetInterface() == nullptr || Itr.GetObject() == nullptr)
		{
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Participant %d is invalid! Cannot Initialize dialogue."), DialogueParticipants.Find(Itr));
			return false;
		}
		
		if (!Itr->Execute_GetOwningActor(Itr.GetObject()))
		{
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Participant %d has no Owning Actor. Check whether function `GetOwningActor` is implemented. Cannot Initialize dialogue."), DialogueParticipants.Find(Itr));
			return false;
		}

		if (!Itr->Execute_CanStartDialogueEvent(Itr.GetObject()))
		{
			UnavailableParticipants.Add(Itr);
			LOG_INFO(TEXT("[StartDialogue] Dialogie Participant %d cannot Start Dialogue, so it will be ignored"), DialogueParticipants.Find(Itr))
		}
	}

	if (UnavailableParticipants.Num() == DialogueParticipants.Num())
	{
		LOG_ERROR(TEXT("[StartDialogue] None of %d Dialogue Participants can Start Dialogue! Dialogue will not launch!"), DialogueParticipants.Num())
		return false;
	}

	UWorld* TempWorld = WorldContextObject->GetWorld();
	if (!TempWorld) TempWorld = Initiator->GetWorld();

	const TScriptInterface<IMounteaDialogueManagerInterface> DialogueManager = GetDialogueManager(Initiator);
	if (DialogueManager == nullptr)
	{
		LOG_ERROR(TEXT("[StartDialogue] Unable to find Dialogue Manager. Cannot Initialize dialogue."));
		return false;
	}

	switch (DialogueManager->Execute_GetState(DialogueManager.GetObject() ))
	{
		case EDialogueManagerState::EDMS_Disabled:
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Manager is Disabled!"))
			return false;
		case EDialogueManagerState::EDMS_Enabled:
			break;
		case EDialogueManagerState::EDMS_Active:
			LOG_ERROR(TEXT("[StartDialogue] Dialogue Manager is already Active!"))
			return false;
	}
	
	const UMounteaDialogueGraph* Graph = MainParticipant->GetDialogueGraph();

	if (!Graph)
	{
		LOG_ERROR(TEXT("[StartDialogue] WorldContextObject is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	if (Graph->CanStartDialogueGraph() == false)
	{
		LOG_ERROR(TEXT("[StartDialogue] Dialogue Graph cannot Start. Cannot Initialize dialogue."));
		return false;
	}
	
	for (const auto& Itr : Graph->GetAllNodes())
	{
		if (Itr)
		{
			Itr->InitializeNode(TempWorld);
		}
	}

	for (auto Itr : Graph->GetAllDecorators())
	{
		Itr.InitializeDecorator(TempWorld, MainParticipant);
	}
	
	return true;
}

bool UMounteaDialogueSystemBFC::InitializeDialogue(const UObject* WorldContextObject, UObject* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface>& DialogueParticipant)
{
	if (!DialogueParticipant)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Missing DialogueParticipant. Cannot Initialize dialogue."));
		return false;
	}

	if (!DialogueParticipant->Execute_GetOwningActor(DialogueParticipant.GetObject()))
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Participant found no Owning Actor. Check whether function `GetOwningActor` is implemented. Cannot Initialize dialogue."));
		return false;
	}
	
	UWorld* TempWorld = WorldContextObject->GetWorld();
	if (!TempWorld) TempWorld = DialogueParticipant->Execute_GetOwningActor(DialogueParticipant.GetObject())->GetWorld();
	
	if (Initiator == nullptr && DialogueParticipant.GetInterface() == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Initiator is empty AND Participant is invalid. Cannot Initialize dialogue."));
		return false;
	}

	if (GetDialogueManager(WorldContextObject) == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] WorldContextObject is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	if (DialogueParticipant->CanStartDialogue() == false)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] WorldContextObject is Invalid. Cannot Initialize dialogue."));
		return false;
	}

	const UMounteaDialogueGraph* Graph = DialogueParticipant->GetDialogueGraph();

	if (Graph == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue participant has no Graph. Cannot Initialize dialogue."));
		return false;
	}

	for (const auto& Itr : Graph->GetAllNodes())
	{
		if (Itr)
		{
			Itr->InitializeNode(TempWorld);
		}
	}

	for (auto Itr : Graph->GetAllDecorators())
	{
		Itr.InitializeDecorator(TempWorld, DialogueParticipant);
	}

	if (Graph->CanStartDialogueGraph() == false)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Graph cannot Start. Cannot Initialize dialogue."));
		return false;
	}

	UMounteaDialogueGraphNode* NodeToStart = DialogueParticipant->GetSavedStartingNode();
	if (!NodeToStart || NodeToStart->CanStartNode() == false)
	{
		NodeToStart = Graph->GetStartNode();
	}
	
	if (NodeToStart == nullptr)
	{
		LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Graph has no Nodes to start. Cannot Initialize dialogue."));
		return false;
	}
	
	if (NodeToStart->GetClass()->IsChildOf(UMounteaDialogueGraphNode_StartNode::StaticClass()))
	{
		if (GetFirstChildNode(NodeToStart) == nullptr)
		{
			LOG_ERROR(TEXT("[InitializeDialogue] Dialogue Graph has only Start Node and no Nodes to start. Cannot Initialize dialogue."));
			return false;
		}

		NodeToStart = GetFirstChildNode(NodeToStart);
	}
	
	const TArray<UMounteaDialogueGraphNode*> StartNode_Children = GetAllowedChildNodes(NodeToStart);

	UMounteaDialogueContext* Context = NewObject<UMounteaDialogueContext>();
	Context->SetDialogueContext(DialogueParticipant, NodeToStart, StartNode_Children);
	Context->UpdateDialoguePlayerParticipant(GetPlayerDialogueParticipant(WorldContextObject));
	
	return  InitializeDialogueWithContext(WorldContextObject, Initiator, DialogueParticipant, Context);
}

TScriptInterface<IMounteaDialogueParticipantInterface> UMounteaDialogueSystemBFC::FindBestMatchingParticipant(const UObject* WorldContextObject, const UMounteaDialogueContext* Context)
{
	if (!Context)
	{
		return nullptr;
	}

	if (!Context->ActiveNode)
	{
		return nullptr;
	}

	const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Context->ActiveNode);
	if (!DialogueNode)
	{
		return nullptr;
	}

	for (auto const& Participant : Context->GetDialogueParticipants())
	{
		const FGameplayTag Tag = Participant->Execute_GetTag(Participant.GetObject());

		const FDialogueRow Row = GetDialogueRow(DialogueNode);
		if (Row.CompatibleTags.HasTagExact(Tag))
		{
			return Participant;
		}
	}

	LOG_ERROR(TEXT("[FindBestMatchingParticipant] Unable to find Dialogue Participant based on Gameplay Tags, returning first (index 0) Participant from Dilaogue Context!"))
	return Context->DialogueParticipants[0];
}

UMounteaDialogueGraphNode* UMounteaDialogueSystemBFC::FindNodeByGUID(const UMounteaDialogueGraph* FromGraph, const FGuid ByGUID)
{
	if (!FromGraph) return nullptr;
	if (!ByGUID.IsValid()) return nullptr;

	for (const auto& Itr : FromGraph->GetAllNodes())
	{
		if (Itr && Itr->GetNodeGUID() == ByGUID) return Itr;
	}

	return nullptr;
}

FDialogueRow UMounteaDialogueSystemBFC::GetDialogueRow(const UMounteaDialogueGraphNode* Node)
{
	const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		
	if (!DialogueNodeBase) return FDialogueRow();
	if (DialogueNodeBase->GetDataTable() == nullptr) return FDialogueRow();
	if (DialogueNodeBase->GetDataTable()->RowStruct->IsChildOf(FDialogueRow::StaticStruct()) == false) return FDialogueRow();

	const FDialogueRow* Row = DialogueNodeBase->GetDataTable()->FindRow<FDialogueRow>(DialogueNodeBase->GetRowName(), FString("") );
	if (!Row) return FDialogueRow();
	if (IsDialogueRowValid(*Row) == false) return FDialogueRow();

	return *Row;
}

float UMounteaDialogueSystemBFC::GetRowDuration(const FDialogueRowData& Row)
{
	float ReturnValue = 1.f;
	switch (Row.RowDurationMode)
	{
		case ERowDurationMode::ERDM_Duration:
			{
				if (Row.RowSound)
				{
					ReturnValue = Row.RowSound->Duration;
					break;
				}
						
				ReturnValue =  Row.RowDuration;
			}
			break;
		case ERowDurationMode::EDRM_Override:
			{
				ReturnValue = Row.RowDurationOverride;
			}
			break;
		case ERowDurationMode::EDRM_Add:
			{
				if (Row.RowSound)
				{
					ReturnValue = Row.RowSound->Duration;
					ReturnValue = ReturnValue + Row.RowDurationOverride;
					break;
				}
				ReturnValue = Row.RowDurationOverride;
			}
			break;
		case ERowDurationMode::ERDM_AutoCalculate:
			{
				//TODO: Make 8:100 ratio editable in Settings!
				if (GetDialogueSystemSettings_Internal())
				{
					ReturnValue= ((Row.RowText.ToString().Len() * GetDialogueSystemSettings_Internal()->GetDurationCoefficient()) / 100.f);
				}
				else
				{
					ReturnValue= ((Row.RowText.ToString().Len() * 8.f) / 100.f);
				}
				break;
			}
	}

	ReturnValue = FMath::Max(1.f, ReturnValue);
		
	return ReturnValue;
}

TArray<FMounteaDialogueDecorator> UMounteaDialogueSystemBFC::GetAllDialogueDecorators(const UMounteaDialogueGraph* FromGraph)
{
	TArray<FMounteaDialogueDecorator> Decorators;

	if (FromGraph == nullptr) return Decorators;
		
	Decorators.Append(FromGraph->GetGraphDecorators());

	for (const auto& Itr : FromGraph->GetAllNodes())
	{
		if (Itr)
		{
			Decorators.Append(Itr->GetNodeDecorators());
		}
	}
		
	return Decorators;
}

bool UMounteaDialogueSystemBFC::CanExecuteCosmeticEvents(const UWorld* WorldContext)
{
	return !UKismetSystemLibrary::IsDedicatedServer(WorldContext);
}
