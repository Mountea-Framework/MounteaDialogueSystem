// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueSystemSettings.h"

#include "Data/MounteaDialogueGraphDataTypes.h"

#include "Interfaces/MounteaDialogueManagerInterface.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"

#include "GameFramework/Pawn.h"

#include "MounteaDialogueSystemBFC.generated.h"

struct FMounteaDialogueDecorator;

/**
 * Runtime helper functions library for Mountea Dialogue System.
 */
UCLASS(meta=(DisplayName="Mountea Dialogue Blueprint Function Library"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSystemBFC : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers")
	static bool IsEditor();
	
	static void CleanupGraph(const UObject* WorldContextObject, const UMounteaDialogueGraph* GraphToClean);

	static void SaveTraversePathToParticipant(TArray<FDialogueTraversePath>& InPath, const TScriptInterface<IMounteaDialogueParticipantInterface> Participant)
	{
		if (!Participant || !Participant.GetInterface()) return;

		Participant->Execute_SaveTraversedPath(Participant.GetObject(), InPath);
	}

	/**
	 * Returns whether selected Node for selected Participant has been already Traversed or not.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="node, traverse, open, active"), meta=(CustomTag="MounteaK2Validate"))
	static bool HasNodeBeenTraversed(const UMounteaDialogueGraphNode* Node, const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant);
	
	/**
	 * Returns whether selected Node has been already Traversed or not.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="node, traverse, open, active"), meta=(CustomTag="MounteaK2Validate"))
	static bool HasNodeBeenTraversedV2(const UMounteaDialogueGraphNode* Node, const UMounteaDialogueContext* Context);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="audio, tag, search"), meta=(CustomTag="MounteaK2Getter"))
	static UAudioComponent* FindAudioComponentByName(const AActor* ActorContext, const FName& Arg);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="audio, tag, search"), meta=(CustomTag="MounteaK2Getter"))
	static UAudioComponent* FindAudioComponentByTag(const AActor* ActorContext, const FName& Arg);

	/**
	 * Returns first 'Mountea Dialogue Particiapnt' Component from Player Pawn.
	 * ❗ Might return Null❗
	 * 
	 * @param WorldContextObject	World Context Object 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Player Participant", Keywords="participant, dialogue, player"), meta=(CustomTag="MounteaK2Getter"))
	static TScriptInterface<IMounteaDialogueParticipantInterface> GetPlayerDialogueParticipant(AActor* WorldContextObject);
	
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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="settings, setup, config"), meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueSystemSettings* GetDialogueSystemSettings()
	{
		return GetMutableDefault<UMounteaDialogueSystemSettings>();
	}
		
	/**
	 * Tries to get default Dialogue Widget from Project Settings.
	 * 
	 * ❗ Will return null if settings are not accessible❗
	 * ❗ Will return null if no Default Widget is selected❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="widget, UI"), meta=(CustomTag="MounteaK2Getter"))
	static TSubclassOf<UUserWidget>  GetDefaultDialogueWidget()
	{
		if (GetDialogueSystemSettings_Internal() == nullptr) return nullptr;
		
		const TSubclassOf<UUserWidget> DefaultClass = GetDialogueSystemSettings_Internal()->GetDefaultDialogueWidget().LoadSynchronous();
		return DefaultClass;
	}
	
	/**
	 * Tries to validate given Dialogue Context.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Is Dialogue Context Valid", Keywords="dialogue, null, validate, valid, check"), meta=(CustomTag="MounteaK2Validate"))
	static bool IsContextValid(const UMounteaDialogueContext* Context);

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
	static bool CloseDialogue(AActor* WorldContextObject, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant);

	/**
	 * Tries to initialize Dialogue.
	 * ❗ Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * 
	 * @param WorldContextObject	World Context Object
	 * @param Initiator							Player State
	 * @param MainParticipant			Main participant, the one who owns the Dialogue Graph
	 * @param DialogueParticipants	Other participants, could be NPCs or other Players
	 */
	static bool StartDialogue(const UObject* WorldContextObject, APlayerState* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants);
	
	/**
	 * Tries to initialize Dialogue.
	 * ❗ Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * 
	 * @param WorldContextObject	World Context Object to read World info from
	 * @param Initiator						Usually Player State or any Actor who implement `IMounteaDialogueManagerInterface`
	 * @param DialogueParticipant	Other person, could be NPC or other Player
	 */
	static bool InitializeDialogue(const UObject* WorldContextObject, AActor* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface>& DialogueParticipant);
	
	/**
	 * Tries to initialize Dialogue with given Context.
	 * ❗ Do not call from Actor's Begin Play, bindings on Manager might not be initialized yet❗
	 * ❗ Preferred way to Initialize Dialogue is to call 'InitializeDialogue' instead❗
	 * 
	 * @param WorldContextObject	World Context Object
	 * @param Initiator						Usually Player State or any Actor who implement `IMounteaDialogueManagerInterface`
	 * @param DialogueParticipant	Other person, could be NPC or other Player
	 * @param Context					Dialogue Context which is passed to Dialogue Manager
	 */
	static bool InitializeDialogueWithContext(const UObject* WorldContextObject, AActor* Initiator, const TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant, UMounteaDialogueContext* Context);

	/**
	 * Adds new participants to the Dialogue Manager.
	 * 
	 * @param WorldContextObject The context within which the world exists.
	 * @param NewParticipants A list of dialogue participants to be added.
	 * @return Returns true if the participants were successfully added, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Dialogue Manager", Keywords="manager, dialogue, master, initialize"), meta=(CustomTag="MounteaK2Setter"))
	static bool AddParticipants(AActor* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants);

	/**
	 * Removes participants from the Dialogue Manager.
	 * 
	 * @param WorldContextObject The context within which the world exists.
	 * @param NewParticipants A list of dialogue participants to be removed.
	 * @return Returns true if the participants were successfully removed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Dialogue Manager", Keywords="manager, dialogue, master, initialize"), meta=(CustomTag="MounteaK2Setter"))
	static bool RemoveParticipants(AActor* WorldContextObject, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants);

	/**
	 * Returns first 'Mountea Dialogue Manager' Component from Player State.
	 * ❗ Might return Null❗
	 * 
	 * @param WorldContextObject	World Context Object 
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Dialogue Manager", Keywords="manager, dialogue, master, initialize"), meta=(CustomTag="MounteaK2Getter"))
	static TScriptInterface<IMounteaDialogueManagerInterface> GetDialogueManager(UObject* WorldContextObject);

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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="get,find"), meta=(CustomTag="MounteaK2Getter"))
	static TScriptInterface<IMounteaDialogueParticipantInterface> FindBestMatchingParticipant(const UObject* WorldContextObject, const UMounteaDialogueContext* Context);

	/**
	 * Searches in Graph for Node by GUID.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="guid, node, find, search, get"), meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraphNode* FindNodeByGUID(const UMounteaDialogueGraph* FromGraph, const FGuid ByGUID);

	/**
	 * Searches in Graph for Node by GUID.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="guid, node, find, search, get"), meta=(CustomTag="MounteaK2Getter"))
	static TArray<UMounteaDialogueGraphNode*> FindNodesByGUID(const UMounteaDialogueGraph* FromGraph, const TArray<FGuid> Guids);

	/**
	 * Converts an array of dialogue nodes to their respective GUIDs.
	 * 
	 * @param Nodes The array of dialogue graph nodes to be converted.
	 * @return An array of GUIDs corresponding to the provided dialogue nodes.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="guid, node, convert, transform"), meta=(CustomTag="MounteaK2Getter"))
	static TArray<FGuid> NodesToGuids(TArray<UMounteaDialogueGraphNode*> Nodes);
	
	/**
	 * Tries to get Child Node from Dialogue Node at given Index. If none is found, returns null.
	 * ❗ Might return Null❗
	 * 
	 * @param Index			Index of the Node to be returned.
	 * @param ParentNode	Parent Node which should contain the Node to be returned.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="dialogue, child, node, index"), meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraphNode* GetChildrenNodeFromIndex(const int32 Index, const UMounteaDialogueGraphNode* ParentNode);

	/**
	 * Tries to get first Dialogue Node from Children Nodes. If none is found, returns null.
	 * ❗ Might return Null❗
	 * 
	 * @param ParentNode	Node to read from.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="dialogue, child, node, first"), meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraphNode* GetFirstChildNode(const UMounteaDialogueGraphNode* ParentNode);

	/**
	 * Returns all Allowed Child Nodes for given Parent Node
	 *❗Might return empty array❗
	 * 
	 * @param ParentNode	Node to get all Children From
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="diaogue, child, nodes"), meta=(CustomTag="MounteaK2Getter"))
	static TArray<UMounteaDialogueGraphNode*> GetAllowedChildNodes(const UMounteaDialogueGraphNode* ParentNode);
	
	/**
	 * Returns whether Dialogue Row is valid or not.
	 * 
	 * @param Row	Row to be validated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Is Dialogue Row Valid", Keywords="dialogue, check, valid, null"), meta=(CustomTag="MounteaK2Validate"))
	static bool IsDialogueRowValid(const FDialogueRow& Row)
	{
		FGuid InvalidGuid;
		InvalidGuid.Invalidate();
		
		return Row.RowGUID != InvalidGuid && Row.DialogueParticipant.IsEmpty() == false && Row.DialogueRowData.Num() > 0;
	}

	/**
	 * Returns whether Dialogue Row Data is valid or not.
	 * 
	 * @param Data	Data to be validated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Is Dialogue Data Valid", Keywords="dialogue, check, valid, null"), meta=(CustomTag="MounteaK2Validate"))
	static bool IsDialogueRowDataValid(const FDialogueRowData& Data)
	{
		FGuid InvalidGuid;
		InvalidGuid.Invalidate();

		return Data.RowGUID != InvalidGuid &&( !Data.RowText.IsEmpty() || !Data.RowText.EqualTo(FText::FromString("Dialogue Example")));
	}
	
	/**
	 * Returns Dialogue Row for Given Node.
	 * ❗ Might return invalid Row❗
	 * ❗Only 'UMounteaDialogueGraphNode_DialogueNodeBase' classes have Dialogue data❗
	 * 
	 * @param Node	Node to get Data from.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Get Dialogue Row", Keywords="row, dialogue"), meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow GetDialogueRow(const UMounteaDialogueGraphNode* Node);
	static FDialogueRow GetDialogueRow(const UDataTable* SourceTable, const FName& SourceName);
	
	/**
	 * Finds a specific dialogue row in a DataTable.
	 * 
	 * @param Table The DataTable where the dialogue row is stored.
	 * @param RowName The name of the row to find.
	 * @return The found dialogue row, or an empty dialogue row if not found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Find Dialogue Row", Keywords="row, dialogue"), meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow FindDialogueRow(const UDataTable* Table, const FName RowName)
	{
		if (Table == nullptr) return FDialogueRow::Invalid();
		if (Table->RowStruct->IsChildOf(FDialogueRow::StaticStruct()) == false) return FDialogueRow::Invalid();

		const FDialogueRow* Row = Table->FindRow<FDialogueRow>(RowName, FString("") );
		if (!Row) return FDialogueRow::Invalid();
		if (IsDialogueRowValid(*Row) == false) return FDialogueRow::Invalid();

		return *Row;
	};

	/**
	 * Returns Duration for each Dialogue Row.
	 * 
	 * @param Row	Row for the Duration to be calculated from.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Duration", Keywords="dialogue, duration, long, time"), meta=(CustomTag="MounteaK2Getter"))
	static float GetRowDuration(const struct FDialogueRowData& Row);

	/**
	 * Retrieves the subtitles settings for the dialogue system.
	 * 
	 * @param WorldContextObject The context within which the world exists.
	 * @param OptionalFilterClass An optional filter class used to refine the subtitles settings.
	 * @return Returns the settings related to dialogue subtitles, or default settings if none are found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(CompactNodeTitle="Subtitles Settings", Keywords="settings, subtitles, font"), meta=(CustomTag="MounteaK2Getter"))
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

	/**
	 * Function to retrieve Dialogue Participant Interface reference from any Actor.
	 * Actors can implement that interface themselves or contain a child component that implements the interface.
	 * If none is found null is returned and error is printed to console.
	 * 
	 * @param ParticipantActor	Actor who should implement the Participant interface
	 * @return								Mountea Dialogue Participant reference
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="get, find, retrieve"), meta=(CustomTag="MounteaK2Getter"))
	static TScriptInterface<IMounteaDialogueParticipantInterface> FindDialogueParticipantInterface(AActor* ParticipantActor, bool& bResult);

	static APawn* FindPlayerPawn(AActor* ForActor, int& SearchDepth);
	static APlayerController* FindPlayerController(AActor* ForActor, int& SearchDepth);

	static bool DoesNodeInvertSkipSettings(UMounteaDialogueGraphNode* Node);

	static bool DoesPreviousNodeSkipActiveNode(const UMounteaDialogueGraph* ParentGraph, const FGuid PreviousNode);

	/**
	 * Gets the execution mode of a specific row within the dialogue context.
	 * 
	 * @param DialogueContext The current dialogue context.
	 * @param RowIndex The index of the row to retrieve the execution mode for.
	 * @return The execution mode of the specified row.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="get, find, retrieve"), meta=(CustomTag="MounteaK2Getter"))
	static ERowExecutionMode GetActiveRowExecutionMode(UMounteaDialogueContext* DialogueContext, const int32 RowIndex);

	/**
	 * Retrieves the given object if it is of the specified class type.
	 *
	 * This function checks whether the provided `Object` is of the class type specified by `ClassFilter`.
	 * If the `Object` is of the specified class or a subclass thereof, it returns the `Object` of the specified class and sets `bResult` to `true`.
	 * Otherwise, it returns `nullptr` and sets `bResult` to `false`.
	 *
	 * @param Object		The object to check and potentially retrieve.
	 * @param ClassFilter	The class type to check against. Only objects of this class or subclasses will be returned.
	 * @param bResult		(Out) `true` if the object is of the specified class type; `false` otherwise.
	 * @return					The casted `Object` if it is of the specified class type; otherwise, `nullptr`.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta = (ClassFilter = "Object"), meta=(DeterminesOutputType = "ClassFilter"), meta=(CustomTag="MounteaK2Getter"))
	static UObject* GetObjectByClass(UObject* Object, const TSubclassOf<UObject> ClassFilter, bool& bResult);

	/**
	 * Retrieves a single component from an actor that implements a specified interface.
	 * 
	 * @param Target The actor from which to retrieve the component.
	 * @param InterfaceFilter The interface type used to filter components.
	 * @param bResult		(Out) `true` if the Component is of the specified Interface type; `false` otherwise.
	 * @return The actor component that implements the specified interface, or null if none found.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta = (ClassFilter = "Interface"), meta=(DeterminesOutputType = "InterfaceFilter"), meta=(CustomTag="MounteaK2Getter"))
	static UActorComponent* GetSingleComponentByInterface(const AActor* Target, TSubclassOf<UInterface> InterfaceFilter, bool& bResult);

	template <typename NodeType>
	static void SortNodes(TArray<NodeType*>& SortedNodes)
	{
		SortedNodes.Sort([](const NodeType& A, const NodeType& B)
		{
			return A.ExecutionOrder < B.ExecutionOrder;
		});
	}

	/**
	 * Sorts given array of Dialogue Nodes based on their Execution Order.
	 * 
	 * @param SortedNodes OUT Nodes array that will be sorted
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers", meta=(Keywords="sort,order,diaogue,child,nodes"), meta=(CustomTag="MounteaK2Getter"))
	static void SortNodes(TArray<UMounteaDialogueGraphNode*>& SortedNodes);

	static UMounteaDialogueGraphNode* GetStartingNode(const TScriptInterface<IMounteaDialogueParticipantInterface>& Participant, const UMounteaDialogueGraph* Graph);

	static UMounteaDialogueContext* CreateDialogueContext(UObject* NewOwner, const TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant, const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& DialogueParticipants);
	static UMounteaDialogueContext* CreateDialogueContext(UObject* NewOwner, const FMounteaDialogueContextReplicatedStruct& NewData);
	
	template<typename T>
	static FString GetEnumFriendlyName(const T EnumValue, const bool bShortName = false)
	{
		static_assert(TIsEnum<T>::Value, "Template parameter must be an enum type");
    
		const UEnum* EnumPtr = StaticEnum<T>();
		if (ensure(EnumPtr))
		{
			if (bShortName)
			{
				return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(EnumValue)).ToString();
			}
			else
			{
				return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(EnumValue)).ToString();
			}
		}
    
		return TEXT("Invalid");
	}

};
