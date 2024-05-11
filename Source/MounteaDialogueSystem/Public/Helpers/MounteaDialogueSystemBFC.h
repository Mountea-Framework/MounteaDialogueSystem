// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueSystemSettings.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"



#include "Interfaces/MounteaDialogueManagerInterface.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"

#include "MounteaDialogueSystemBFC.generated.h"


/**
 * Runtime helper functions library for Mountea Dialogue System.
 */
UCLASS(meta=(DisplayName="Mountea Dialogue Blueprint Function Library"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSystemBFC : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	static void CleanupGraph(const UObject* WorldContextObject, const UMounteaDialogueGraph* GraphToClean);

	static void InitializeDecorators(UWorld* WorldContext, const TScriptInterface<IMounteaDialogueParticipantInterface> Participant);

	static void SaveTraversePathToParticipant(TArray<FDialogueTraversePath>& InPath, const TScriptInterface<IMounteaDialogueParticipantInterface> Participant)
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
	static UAudioComponent* FindAudioComponentByName(const AActor* ActorContext, const FName& Arg);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="audio, tag, search"))
	static UAudioComponent* FindAudioComponentByTag(const AActor* ActorContext, const FName& Arg);

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
	static bool ExecuteDecorators(const UObject* WorldContextObject, const UMounteaDialogueContext* DialogueContext);
	
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
		if (!GetDialogueManager(WorldContextObject))
		{
			LOG_ERROR(TEXT("[CloseDialogue] Cannot find Dialogue Manager. Cannot close dialogue."));
			return false;
		}
		
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
	 * @param Initiator							Player Controller
	 * @param MainParticipant			Main participant, the one who owns the Dialogue Graph
	 * @param DialogueParticipants	Other participants, could be NPCs or other Players
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", Keywords="start, initialize, dialogue"))
	static bool StartDialogue(const UObject* WorldContextObject, APlayerState* Initiator, TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant, TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants);
	
	/**
	 * Tries to initialize Dialogue.
	 * ❗ Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * 
	 * @param WorldContextObject	World Context Object
	 * @param Initiator						Usually Player Controller
	 * @param DialogueParticipant	Other person, could be NPC or other Player
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue", meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", Keywords="start, initialize, dialogue"))
	static bool InitializeDialogue(const UObject* WorldContextObject, UObject* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface>& DialogueParticipant);
	
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
		if (DialogueParticipant == nullptr)
		{
			LOG_ERROR(TEXT("[InitializeDialogueWithContext] Missing DialogueParticipant. Cannot Initialize dialogue."));
			return false;
		}
		if (Context == nullptr)
		{
			LOG_ERROR(TEXT("[InitializeDialogueWithContext] Missing Dialogue Context. Cannot Initialize dialogue."));
			return false;
		}
		if (IsContextValid(Context) == false)
		{
			LOG_ERROR(TEXT("[InitializeDialogueWithContext] Dialogue Context is Invalid. Cannot Initialize dialogue."));
			return false;
		}

		const auto DialogueManager = GetDialogueManager(WorldContextObject);
		if (DialogueManager == nullptr)
		{
			LOG_ERROR(TEXT("[InitializeDialogueWithContext] Dialogue Manager is Invalid. Cannot Initialize dialogue."));
			return false;
		}

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

		// Make sure we check Context first
		if (const AActor* ActorContext = Cast<AActor>(WorldContextObject))
		{
			if (UActorComponent* ManagerComponent = ActorContext->FindComponentByInterface(UMounteaDialogueManagerInterface::StaticClass()))
			{
				return ManagerComponent;
			}
		}

		// TODO: change!
		const APlayerController* PlayerController = WorldContextObject->GetWorld()->GetFirstPlayerController();

		if (!PlayerController) return nullptr;

		if (UActorComponent* ManagerComponent = PlayerController->FindComponentByInterface(UMounteaDialogueManagerInterface::StaticClass()))
		{
			return ManagerComponent;
		}
		
		LOG_ERROR(TEXT("[GetDialogueManager] Unable to find Dialogue Manager."));
		return nullptr;
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
	static TScriptInterface<IMounteaDialogueParticipantInterface> FindBestMatchingParticipant(const UObject* WorldContextObject, const UMounteaDialogueContext* Context);

	/**
	 * Searches in Graph for Node by GUID.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue", meta=(Keywords="guid, node, find, search, get"))
	static UMounteaDialogueGraphNode* FindNodeByGUID(const UMounteaDialogueGraph* FromGraph, const FGuid ByGUID);
	
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
	static FDialogueRow GetDialogueRow(const UMounteaDialogueGraphNode* Node);

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
	static float GetRowDuration(const struct FDialogueRowData& Row);

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

	static TArray<FMounteaDialogueDecorator> GetAllDialogueDecorators(const UMounteaDialogueGraph* FromGraph);

	static bool CanExecuteCosmeticEvents(const UWorld* WorldContext);
};
