// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueSystemSettings.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"

#include "Graph/MounteaDialogueGraph.h"

#include "Interfaces/MounteaDialogueManagerInterface.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"

#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
#include "Sound/SoundBase.h"

#include "MounteaDialogueSystemBFC.generated.h"


/**
 * Runtime helper functions library for Mountea Dialogue System.
 */
UCLASS(meta=(DisplayName="Mountea Dialogue Blueprint Function Library"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSystemBFC : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	static void CleanupGraph(const UObject* WorldContextObject, const UMounteaDialogueGraph* GraphToClean)
	{
		if (!GraphToClean) return;
		
		// Cleanup Decorators
		for (auto Itr : GraphToClean->GetAllDecorators())
		{
			Itr.CleanupDecorator();
		}
	}

	static void InitializeDecorators(UWorld* WorldContext, const TScriptInterface<IMounteaDialogueParticipantInterface> Participant)
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

	static void SaveTraversePathToParticipant(TMap<FGuid,int32>& InPath, const TScriptInterface<IMounteaDialogueParticipantInterface> Participant)
	{
		if (!Participant || !Participant.GetInterface()) return;

		Participant->Execute_SaveTraversedPath(Participant.GetObject(), InPath);
	}

	/**
	 * Returns whether selected Node for selected Participant has been already Traversed or not.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="node, traverse, open, active"))
	static bool HasNodeBeenTraversed(const UMounteaDialogueGraphNode* Node, const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant)
	{
		bool bTraversed = false;

		if (!Node) return bTraversed;
		if (!Participant || !Participant.GetObject()) return bTraversed;

		bTraversed = Participant->GetTraversedPath().Contains(Node->GetNodeGUID());
		
		return bTraversed;
	}
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="audio, tag, search"))
	static UAudioComponent* FindAudioComponentByName(const AActor* ActorContext, const FName& Arg)
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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="audio, tag, search"))
	static UAudioComponent* FindAudioComponentByTag(const AActor* ActorContext, const FName& Arg)
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

	/**
	 * Returns first 'Mountea Dialogue Particiapnt' Component from Player Pawn.
	 * ❗ Might return Null❗
	 * 
	 * @param WorldContextObject	World Context Object 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", CompactNodeTitle="Player Participant", Keywords="participant, dialogue, player"))
	static TScriptInterface<IMounteaDialogueParticipantInterface> GetPlayerDialogueParticipant(const UObject* WorldContextObject)
	{
		if (WorldContextObject == nullptr) return nullptr;

		const APlayerController* PlayerController = WorldContextObject->GetWorld()->GetFirstPlayerController();

		if (!PlayerController) return nullptr;

		const APawn* PlayerPawn = PlayerController->GetPawn();

		if (PlayerPawn == nullptr) return nullptr;

		auto Components = PlayerPawn->GetComponentsByInterface(UMounteaDialogueParticipantInterface::StaticClass());

		if (Components.Num() == 0) return nullptr;

		TScriptInterface<IMounteaDialogueParticipantInterface> ReturnValue;
		ReturnValue.SetObject(Components[0]);
		ReturnValue.SetInterface(Cast<IMounteaDialogueParticipantInterface>(Components[0]));

		return ReturnValue;
	}
	
	/**
	 * Returns Dialogue System Settings.
	 * ❗ Might return null❗
	 */
	static const UMounteaDialogueSystemSettings* GetDialogueSystemSettings_Internal()
	{
		return GetDefault<UMounteaDialogueSystemSettings>();
	}
	
	/**
	 * Returns Dialogue System Settings.
	 * ❗ Might return null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="settings, setup, config"))
	static UMounteaDialogueSystemSettings* GetDialogueSystemSettings()
	{
		return GetMutableDefault<UMounteaDialogueSystemSettings>();
	};
	
	/**
	 * Tries to get default Dialogue Widget from Project Settings.
	 * 
	 * ❗ Will return null if settings are not accessible❗
	 * ❗ Will return null if no Default Widget is selected❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="widget, UI"))
	static TSubclassOf<UUserWidget>  GetDefaultDialogueWidget()
	{
		if (GetDialogueSystemSettings_Internal() == nullptr) return nullptr;
		
		const TSubclassOf<UUserWidget> DefaultClass = GetDialogueSystemSettings_Internal()->GetDefaultDialogueWidget().LoadSynchronous();
		return DefaultClass;
	}
	
	/**
	 * Tries to validate given Dialogue Context.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Is Dialogue Context Valid", Keywords="dialogue, null, validate, valid, check"))
	static bool IsContextValid(const UMounteaDialogueContext* Context)
	{
		if (Context == nullptr) return false;

		return Context->IsValid();
	}

	/**
	 * Requests Execution for all Decorators for Graph and Context Node
	 */
	static bool ExecuteDecorators(const UObject* WorldContextObject, const UMounteaDialogueContext* DialogueContext)
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
		// TODO: all weight to them so we can sort them by Weight and execute in correct order
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
	
	/**
	 * Tries to close Dialogue.
	 * 
	 * ❗ Returns false if Dialogue Manager is not accessible❗
	 * @param WorldContextObject	World Context Object
	 * @param DialogueParticipant	Dialogue with which Participant to close
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", Keywords="close, exit, dialogue"))
	static bool CloseDialogue(const UObject* WorldContextObject, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant)
	{
		if (!GetDialogueManager(WorldContextObject)) return false;
		
		UMounteaDialogueContext* Context = NewObject<UMounteaDialogueContext>();
		Context->SetDialogueContext(DialogueParticipant, nullptr, TArray<UMounteaDialogueGraphNode*>());
		
		GetDialogueManager(WorldContextObject)->GetDialogueClosedEventHandle().Broadcast(Context);
		return true;
	}
	
	/**
	 * Tries to initialize Dialogue.
	 * ❗ Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * 
	 * @param WorldContextObject	World Context Object
	 * @param Initiator						Usually Player Controller
	 * @param DialogueParticipant	Other person, could be NPC or other Player
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", Keywords="start, initialize, dialogue"))
	static bool InitializeDialogue(const UObject* WorldContextObject, UObject* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface>& DialogueParticipant)
	{
		if (!DialogueParticipant) return false;

		if (!DialogueParticipant->Execute_GetOwningActor(DialogueParticipant.GetObject())) return false;
		
		UWorld* TempWorld = WorldContextObject->GetWorld();
		if (!TempWorld) TempWorld = DialogueParticipant->Execute_GetOwningActor(DialogueParticipant.GetObject())->GetWorld();
		
		if (Initiator == nullptr && DialogueParticipant.GetInterface() == nullptr) return false;

		if (GetDialogueManager(WorldContextObject) == nullptr) return false;

		if (DialogueParticipant->CanStartDialogue() == false) return false;

		const UMounteaDialogueGraph* Graph = DialogueParticipant->GetDialogueGraph();

		if (Graph == nullptr) return false;

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

		if (Graph->CanStartDialogueGraph() == false) return false;

		UMounteaDialogueGraphNode* NodeToStart = DialogueParticipant->GetSavedStartingNode();
		if (!NodeToStart || NodeToStart->CanStartNode() == false)
		{
			NodeToStart = Graph->GetStartNode();
		}
		
		if (NodeToStart == nullptr) return false;
		
		if (NodeToStart->GetClass()->IsChildOf(UMounteaDialogueGraphNode_StartNode::StaticClass()))
		{
			if (GetFirstChildNode(NodeToStart) == nullptr) return false;

			NodeToStart = GetFirstChildNode(NodeToStart);
		}
		
		const TArray<UMounteaDialogueGraphNode*> StartNode_Children = GetAllowedChildNodes(NodeToStart);

		UMounteaDialogueContext* Context = NewObject<UMounteaDialogueContext>();
		Context->SetDialogueContext(DialogueParticipant, NodeToStart, StartNode_Children);
		Context->UpdateDialoguePlayerParticipant(GetPlayerDialogueParticipant(WorldContextObject));
		
		return  InitializeDialogueWithContext(WorldContextObject, Initiator, DialogueParticipant, Context);
	}
	
	/**
	 * Tries to initialize Dialogue with given Context.
	 * ❗ Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * ❗ Preferred way to Initialize Dialogue is to call 'InitializeDialogue' instead❗
	 * 
	 * @param WorldContextObject	World Context Object
	 * @param Initiator						Usually Player Controller
	 * @param DialogueParticipant	Other person, could be NPC or other Player
	 * @param Context					Dialogue Context which is passed to Dialogue Manager
	 */
	static bool InitializeDialogueWithContext(const UObject* WorldContextObject, UObject* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant, UMounteaDialogueContext* Context)
	{
		if (DialogueParticipant == nullptr) return false;
		if (Context == nullptr) return false;
		if (IsContextValid(Context) == false) return false;

		const auto DialogueManager = GetDialogueManager(WorldContextObject);
		if (DialogueManager == nullptr) return false;

		DialogueManager->GetDialogueInitializedEventHandle().Broadcast(Context);
		return true;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", CompactNodeTitle="Dialogue Manager", Keywords="manager, dialogue, master, initialize"))
	static bool AddParticipants(const UObject* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
	{
		const TScriptInterface<IMounteaDialogueManagerInterface> Manager = GetDialogueManager(WorldContextObject);
		if (!Manager)
		{
			return false;
		}

		UMounteaDialogueContext* Context = Manager->GetDialogueContext();

		if (!Context)
		{
			return false;
		}

		return Context->AddDialogueParticipants(NewParticipants);		
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", CompactNodeTitle="Dialogue Manager", Keywords="manager, dialogue, master, initialize"))
	static bool RemoveParticipants(const UObject* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants)
	{
		const TScriptInterface<IMounteaDialogueManagerInterface> Manager = GetDialogueManager(WorldContextObject);
		if (!Manager)
		{
			return false;
		}

		UMounteaDialogueContext* Context = Manager->GetDialogueContext();

		if (!Context)
		{
			return false;
		}

		return Context->RemoveDialogueParticipants(NewParticipants);		
	}

	/**
	 * Returns first 'Mountea Dialogue Manager' Component from Player Controller.
	 * ❗ Might return Null❗
	 * 
	 * @param WorldContextObject	World Context Object 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", CompactNodeTitle="Dialogue Manager", Keywords="manager, dialogue, master, initialize"))
	static TScriptInterface<IMounteaDialogueManagerInterface> GetDialogueManager(const UObject* WorldContextObject)
	{
		if (!WorldContextObject) return nullptr;

		if (WorldContextObject->GetWorld() == nullptr) return nullptr;
			
		const APlayerController* PlayerController = WorldContextObject->GetWorld()->GetFirstPlayerController();

		if (!PlayerController) return nullptr;

		auto Components = PlayerController->GetComponentsByInterface(UMounteaDialogueManagerInterface::StaticClass());

		if (Components.Num() == 0) return nullptr;

		TScriptInterface<IMounteaDialogueManagerInterface> ReturnValue;
		ReturnValue.SetObject(Components[0]);
		ReturnValue.SetInterface(Cast<IMounteaDialogueManagerInterface>(Components[0]));

		return ReturnValue;
	}

	/**
	 * Find the best matching dialogue participant based on the active dialogue node and context.
	 *
	 * This static function searches for the best matching dialogue participant given the active dialogue node and context.
	 * It checks the compatibility of each participant's gameplay tag with the tags specified in the active dialogue node's dialogue row.
	 * The participant with a compatible tag is considered the best match and returned as a TScriptInterface<IMounteaDialogueParticipantInterface>.
	 *
	 * @param WorldContextObject The world context object from which to retrieve the dialogue participants.
	 * @param Context The Mountea dialogue context containing the active node and participants.
	 * @return The best matching dialogue participant, or nullptr if no match is found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", Keywords="get,find"))
	static TScriptInterface<IMounteaDialogueParticipantInterface> FindBestMatchingParticipant(const UObject* WorldContextObject, const UMounteaDialogueContext* Context)
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

		return nullptr;
	}

	/**
	 * Searches in Graph for Node by GUID.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="guid, node, find, search, get"))
	static UMounteaDialogueGraphNode* FindNodeByGUID(const UMounteaDialogueGraph* FromGraph, const FGuid ByGUID)
	{
		if (!FromGraph) return nullptr;
		if (!ByGUID.IsValid()) return nullptr;

		for (const auto& Itr : FromGraph->GetAllNodes())
		{
			if (Itr && Itr->GetNodeGUID() == ByGUID) return Itr;
		}

		return nullptr;
	}
	
	/**
	 * Tries to get Child Node from Dialogue Node at given Index. If none is found, returns null.
	 * ❗ Might return Null❗
	 * 
	 * @param Index			Index of the Node to be returned.
	 * @param ParentNode	Parent Node which should contain the Node to be returned.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="dialogue, child, node, index"))
	static UMounteaDialogueGraphNode* GetChildrenNodeFromIndex(const int32 Index, const UMounteaDialogueGraphNode* ParentNode)
	{
		if (ParentNode->GetChildrenNodes().IsValidIndex(Index))
		{
			return ParentNode->GetChildrenNodes()[Index];
		}

		return nullptr;
	}

	/**
	 * Tries to get first Dialogue Node from Children Nodes. If none is found, returns null.
	 * ❗ Might return Null❗
	 * 
	 * @param ParentNode	Node to read from.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="dialogue, child, node, first"))
	static UMounteaDialogueGraphNode* GetFirstChildNode(const UMounteaDialogueGraphNode* ParentNode)
	{
		if (ParentNode == nullptr) return nullptr;

		if (ParentNode->GetChildrenNodes().IsValidIndex(0))
		{
			return ParentNode->GetChildrenNodes()[0]->CanStartNode() ? ParentNode->GetChildrenNodes()[0] : nullptr;
		}

		return nullptr;
	}

	/**
	 * Returns all Allowed Child Nodes for given Parent Node
	 *❗Might return empty array❗
	 * 
	 * @param ParentNode	Node to get all Children From
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="diaogue, child, nodes"))
	static TArray<UMounteaDialogueGraphNode*> GetAllowedChildNodes(const UMounteaDialogueGraphNode* ParentNode)
	{
		TArray<UMounteaDialogueGraphNode*> ReturnNodes;

		if (!ParentNode) return ReturnNodes;

		if (ParentNode->GetChildrenNodes().Num() == 0) return ReturnNodes;

		for (UMounteaDialogueGraphNode* Itr : ParentNode->GetChildrenNodes())
		{
			if (Itr && Itr->CanStartNode())
			{
				ReturnNodes.Add(Itr);
			}
		}

		return ReturnNodes;
	}

	/**
	 * Returns whether Dialogue Row is valid or not.
	 * 
	 * @param Row	Row to be validated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Is Dialogue Row Valid", Keywords="dialogue, check, valid, null"))
	static bool IsDialogueRowValid(const FDialogueRow& Row)
	{
		FGuid InvalidGuid;
		InvalidGuid.Invalidate();
		
		return Row.RowGUID != InvalidGuid || Row.DialogueParticipant.IsEmpty() == false;
	}

	/**
	 * Returns whether Dialogue Row Data is valid or not.
	 * 
	 * @param Data	Data to be validated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Is Dialogue Data Valid", Keywords="dialogue, check, valid, null"))
	static bool IsDialogueRowDataValid(const FDialogueRowData& Data)
	{
		FGuid InvalidGuid;
		InvalidGuid.Invalidate();

		return Data.RowGUID != InvalidGuid || !Data.RowText.IsEmpty() || !Data.RowText.EqualTo(FText::FromString("Dialogue Example"));
	}
	
	/**
	 * Returns Dialogue Row for Given Node.
	 * ❗ Might return invalid Row❗
	 * ❗Only 'UMounteaDialogueGraphNode_DialogueNodeBase' classes have Dialogue data❗
	 * 
	 * @param Node	Node to get Data from.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Get Dialogue Row", Keywords="row, dialogue"))
	static FDialogueRow GetDialogueRow(const UMounteaDialogueGraphNode* Node)
	{
		const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		
		if (!DialogueNodeBase) return FDialogueRow();
		if (DialogueNodeBase->GetDataTable() == nullptr) return FDialogueRow();
		if (DialogueNodeBase->GetDataTable()->RowStruct->IsChildOf(FDialogueRow::StaticStruct()) == false) return FDialogueRow();

		const FDialogueRow* Row = DialogueNodeBase->GetDataTable()->FindRow<FDialogueRow>(DialogueNodeBase->GetRowName(), FString("") );
		if (!Row) return FDialogueRow();
		if (IsDialogueRowValid(*Row) == false) return FDialogueRow();

		return *Row;
	};

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Find Dialogue Row", Keywords="row, dialogue"))
	static FDialogueRow FindDialogueRow(const UDataTable* Table, const FName RowName)
	{
		if (Table == nullptr) return FDialogueRow();
		if (Table->RowStruct->IsChildOf(FDialogueRow::StaticStruct()) == false) return FDialogueRow();

		const FDialogueRow* Row = Table->FindRow<FDialogueRow>(RowName, FString("") );
		if (!Row) return FDialogueRow();
		if (IsDialogueRowValid(*Row) == false) return FDialogueRow();

		return *Row;
	};

	/**
	 * Returns Duration for each Dialogue Row.
	 * 
	 * @param Row	Row for the Duration to be calculated from.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Duration", Keywords="dialogue, duration, long, time"))
	static float GetRowDuration(const struct FDialogueRowData& Row)
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
					ReturnValue= ((Row.RowText.ToString().Len() * 8.f) / 100.f);
					break;
				}
		}

		ReturnValue = FMath::Max(1.f, ReturnValue);
		
		return ReturnValue;
	}

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(CompactNodeTitle="Subtitles Settings", Keywords="settings, subtitles, font"))
	static FSubtitlesSettings GetSubtitlesSettings(const UObject* WorldContextObject, const FUIRowID& OptionalFilterClass)
	{
		if (GetDialogueSystemSettings_Internal() == nullptr)
		{
			return FSubtitlesSettings();
		}

		return GetDialogueSystemSettings_Internal()->GetSubtitlesSettings(OptionalFilterClass);
	}

	static TArray<FMounteaDialogueDecorator> GetAllDialogueDecorators(const UMounteaDialogueGraph* FromGraph)
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
};
