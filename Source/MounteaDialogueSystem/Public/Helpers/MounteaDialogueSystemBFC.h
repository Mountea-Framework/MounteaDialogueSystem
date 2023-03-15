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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="audio, tag, search"))
	static UAudioComponent* FindAudioComponentByName(const AActor* ActorContext, const FName& Arg)
	{
		if (ActorContext == nullptr) return nullptr;

		TArray<UAudioComponent*> OwnerComponents;
		ActorContext->GetComponents<UAudioComponent>(OwnerComponents);
	
		if (OwnerComponents.Num() == 0) return nullptr;

		for (const auto Itr : OwnerComponents)
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
		
		for (const auto Itr : OwnerComponents)
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
	 * ❗Might return Null❗
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
	 * ❗Might return null❗
	 */
	static const UMounteaDialogueSystemSettings* GetDialogueSystemSettings_Internal()
	{
		return GetDefault<UMounteaDialogueSystemSettings>();
	}
	
	/**
	 * Returns Dialogue System Settings.
	 * ❗Might return null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="settings, setup, config"))
	static UMounteaDialogueSystemSettings* GetDialogueSystemSettings()
	{
		return GetMutableDefault<UMounteaDialogueSystemSettings>();
	};
	
	/**
	 * Tries to get default Dialogue Widget from Project Settings.
	 * 
	 * ❗Will return null if settings are not accessible❗
	 * ❗Will return null if no Default Widget is selected❗
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
	static bool IsContextValid(UMounteaDialogueContext* Context)
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

		if (DialogueContext->GetActiveNode() == nullptr)
		{
			return false;
		}
		
		TArray<FMounteaDialogueDecorator> AllDecorators;
		AllDecorators.Append(DialogueContext->DialogueParticipant->GetDialogueGraph()->GetGraphDecorators());
		AllDecorators.Append(DialogueContext->GetActiveNode()->GetNodeDecorators());

		for (auto Itr : AllDecorators)
		{
			Itr.ExecuteDecorator();
		}

		return true;
	}
	
	/**
	 * Tries to close Dialogue.
	 * 
	 * ❗Returns false if Dialogue Manager is not accessible❗
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
	 * ❗Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * 
	 * @param WorldContextObject	World Context Object
	 * @param Initiator						Usually Player Controller
	 * @param DialogueParticipant	Other person, could be NPC or other Player
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", Keywords="start, initialize, dialogue"))
	static bool InitializeDialogue(const UObject* WorldContextObject, UObject* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant)
	{
		if (Initiator == nullptr || DialogueParticipant.GetInterface() == nullptr) return false;

		if (GetDialogueManager(WorldContextObject) == nullptr) return false;

		if (DialogueParticipant->CanStartDialogue() == false) return false;

		const UMounteaDialogueGraph* Graph = DialogueParticipant->GetDialogueGraph();

		if (Graph == nullptr) return false;

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

		TArray<UMounteaDialogueGraphNode*> StartNode_Children = GetAllowedChildNodes(NodeToStart);
		
		UMounteaDialogueContext* Context = NewObject<UMounteaDialogueContext>();
		Context->SetDialogueContext(DialogueParticipant, NodeToStart, StartNode_Children);
		Context->UpdateDialoguePlayerParticipant(GetPlayerDialogueParticipant(WorldContextObject));
		
		return  InitializeDialogueWithContext(WorldContextObject, Initiator, DialogueParticipant, Context);
	}
	
	/**
	 * Tries to initialize Dialogue with given Context.
	 * ❗Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * ❗Preferred way to Initialize Dialogue is to call 'InitializeDialogue' instead❗
	 * 
	 * @param WorldContextObject	World Context Object
	 * @param Initiator						Usually Player Controller
	 * @param DialogueParticipant	Other person, could be NPC or other Player
	 * @param Context					Dialogue Context which is passed to Dialogue Manager
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", Keywords="start, initialize, dialogue"))
	static bool InitializeDialogueWithContext(const UObject* WorldContextObject, UObject* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant, UMounteaDialogueContext* Context)
	{
		if (DialogueParticipant == nullptr) return false;
		if (Context == nullptr) return false;
		if (IsContextValid(Context) == false) return false;

		GetDialogueManager(WorldContextObject)->GetDialogueInitializedEventHandle().Broadcast(Context);
		return true;
	}

	/**
	 * Returns first 'Mountea Dialogue Manager' Component from Player Controller.
	 * ❗Might return Null❗
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
	 * Tries to get Child Node from Dialogue Node at given Index. If none is found, returns null.
	 * ❗Might return Null❗
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
	 * ❗Might return Null❗
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
	 * ❗Might return invalid Row❗
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
					ReturnValue = Row.RowDuration + Row.RowDurationOverride;
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

		for (const auto Itr : FromGraph->GetAllNodes())
		{
			if (Itr)
			{
				Decorators.Append(Itr->GetNodeDecorators());
			}
		}
		
		return Decorators;
	}
};
