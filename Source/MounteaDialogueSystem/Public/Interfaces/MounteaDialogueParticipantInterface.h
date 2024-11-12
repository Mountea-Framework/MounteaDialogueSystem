// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Containers/Map.h"
#include "UObject/Interface.h"
#include "MounteaDialogueParticipantInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueParticipantInterface : public UInterface
{
	GENERATED_BODY()
};

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class IMounteaDialogueManagerInterface;

struct FDialogueRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueGraphChanged, UMounteaDialogueGraph*, NewGraph);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantStateChanged, const EDialogueParticipantState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantAudioComponentChanged,	const UAudioComponent*, NewAudioComp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FParticipantStartingNodeSaved, const UMounteaDialogueGraphNode*, NewSavedNode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FParticipantCommandRequested, const FString&, Command, UObject*, OptionalPayload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueUpdated, const TScriptInterface<IMounteaDialogueManagerInterface>&, OwningManager);

/**
 * 
 */
USTRUCT(BlueprintType)
struct FDialogueTraversePath
{
	GENERATED_BODY()

public:
	
	FDialogueTraversePath()
		: NodeGuid(FGuid::NewGuid())
		, GraphGuid(FGuid::NewGuid())
		, TraverseCount(0)
	{}

	FDialogueTraversePath(const FGuid& InNodeGuid, const FGuid& InGraphGuid, int32 InTraverseCount = 1)
		: NodeGuid(InNodeGuid)
		, GraphGuid(InGraphGuid)
		, TraverseCount(FMath::Max(0, InTraverseCount))
	{}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	FGuid NodeGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	FGuid GraphGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	int32 TraverseCount;

	bool operator==(const FDialogueTraversePath& Other) const
	{
		return NodeGuid == Other.NodeGuid && GraphGuid == Other.GraphGuid;
	}

	bool operator!=(const FDialogueTraversePath& Other) const
	{
		return !(*this == Other);
	}

	FDialogueTraversePath& operator+=(const FDialogueTraversePath& Other)
	{
		if (NodeGuid == Other.NodeGuid && GraphGuid == Other.GraphGuid)
		{
			TraverseCount += Other.TraverseCount;
		}
		return *this;
	}

	friend uint32 GetTypeHash(const FDialogueTraversePath& Path)
	{
		return HashCombine(GetTypeHash(Path.NodeGuid), GetTypeHash(Path.GraphGuid));
	}

	void IncrementCount(int32 IncrementBy = 1)
	{
		TraverseCount += FMath::Max(0, IncrementBy);
	}

	TPair<FGuid, FGuid> GetGuidPair() const
	{
		return TPair<FGuid, FGuid>(NodeGuid, GraphGuid);
	}
};

/**
 * Mountea Dialogue Participant Interface.
 * Interface connecting Mountea Dialogue Graph with Mountea Dialogue Manager Component.
 * Mountea Dialogue Participant Component is implementing this Interface.
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueParticipantInterface
{
	GENERATED_BODY()

public:

#pragma region Functions

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	bool CanParticipateInDialogue() const;
	virtual bool CanParticipateInDialogue_Implementation() const = 0;
	
	/*
	 * A way to determine whether the Dialogue can even start.
	 * It does come with Native C++ implementation, which can be overriden in child C++ classes.
	 * ❗ If you are using Blueprint implementation, don't forget to call Parent Node, which contains all parent implementations.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	bool CanStartDialogue() const;
	virtual bool CanStartDialogue_Implementation() const = 0;
	
	/**
	 * Returns the owning actor for this Dialogue Participant Component.
	 *
	 * @return The owning actor for this Dialogue Participant Component.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	AActor* GetOwningActor() const;
	virtual AActor* GetOwningActor_Implementation() const = 0;
	
	/**
	 * Saves the starting node for this Dialogue Participant Component.
	 *
	 * @param NewStartingNode The node to set as the starting node
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void SaveStartingNode(UMounteaDialogueGraphNode* NewStartingNode);
	virtual void SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode) = 0;
	
	/**
	 * Saves the traversed path for this Dialogue Participant Component.
	 * This function is called once Dialogue ends and is updated from Dialogue Context.
	 *
	 * @param InPath The traversed path of the dialogue graph to be saved.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void SaveTraversedPath(TArray<FDialogueTraversePath>& InPath);
	virtual void SaveTraversedPath_Implementation(TArray<FDialogueTraversePath>& InPath) = 0;
	
	/**
	 * Getter for Participant Gameplay Tag.
	 * @return Participant Gameplay Tag if any is associated.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	FGameplayTag GetParticipantTag() const;
	virtual FGameplayTag GetParticipantTag_Implementation() const = 0;
	
	/**
	 * Helps initialize Participant.
	 * ❔ Is being called in BeginPlay.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void InitializeParticipant(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);
	virtual void InitializeParticipant_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) = 0;

	/**
	 * Plays the given participant voice sound.
	 * @param ParticipantVoice The sound to play.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be played.
	 */ 
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void PlayParticipantVoice(USoundBase* ParticipantVoice);
	virtual void PlayParticipantVoice_Implementation(USoundBase* ParticipantVoice) = 0;

	/**
	 * Skips the given participant voice sound and whole row. Will automatically start new Row if any is available.
	 * @param ParticipantVoice The sound to skip. Can be left empty.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be skipped.
	 */ 
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void SkipParticipantVoice(USoundBase* ParticipantVoice);
	virtual void SkipParticipantVoice_Implementation(USoundBase* ParticipantVoice) = 0;
		
	/**
	 * Gets the saved starting node for this Dialogue Participant.
	 * ❗ Could be null
	 * 
	 * @return The saved starting node, or nullptr if there is none
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	UMounteaDialogueGraphNode* GetSavedStartingNode() const;
	virtual UMounteaDialogueGraphNode* GetSavedStartingNode_Implementation() const = 0;
	
	/**
	 * Returns the dialogue graph assigned to this Participant.
	 * ❔ Could be updated using 'SetDialogueGraph', providing ability to switch Dialogue graphs on fly
	 * ❗ Could be null
	 *
	 * @return A pointer to the dialogue graph
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	UMounteaDialogueGraph* GetDialogueGraph() const;
	virtual UMounteaDialogueGraph* GetDialogueGraph_Implementation() const = 0;

	/**
	 * Sets new Dialogue graph for this Participant.
	 * ❗ Should not be null
	 *
	 * @param NewDialogueGraph	A pointer to the dialogue graph to be used
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph);
	virtual void SetDialogueGraph_Implementation(UMounteaDialogueGraph* NewDialogueGraph) = 0;

	/**
	 * Returns the current state of the Dialogue Participant.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	EDialogueParticipantState GetParticipantState() const;
	virtual EDialogueParticipantState GetParticipantState_Implementation() const = 0;
	
	/**
	 * Sets the state of the dialogue participant to the given state.
	 * 
	 * @param NewState The new state to set the dialogue participant to
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void SetParticipantState(const EDialogueParticipantState NewState);
	virtual void SetParticipantState_Implementation(const EDialogueParticipantState NewState) = 0;
	
	/**
	 * Returns the default state of the Dialogue Participant.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	EDialogueParticipantState GetDefaultParticipantState() const;
	virtual EDialogueParticipantState GetDefaultParticipantState_Implementation() const = 0;
	
	/**
	 * Sets the Default state of the dialogue participant to the given state.
	 * 
	 * @param NewState The new state to set the dialogue participant to
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void SetDefaultParticipantState(const EDialogueParticipantState NewState);
	virtual void SetDefaultParticipantState_Implementation(const EDialogueParticipantState NewState) = 0;

	/**
	 * Returns the audio component used to play the participant voices.
	 * ❗ Could be null
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	UAudioComponent* GetAudioComponent() const;
	virtual UAudioComponent* GetAudioComponent_Implementation() const = 0;
	
	/**
	 * Sets the audio component used to play dialogue audio.
	 *
	 * @param NewAudioComponent The new audio component to use for dialogue audio.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void SetAudioComponent(UAudioComponent* NewAudioComponent);
	virtual void SetAudioComponent_Implementation(UAudioComponent* NewAudioComponent) = 0;

	/**
	 * Gets the traversed path of the Dialogue Participant.
	 *
	 * @return Array of traversed dialogue paths
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	TArray<FDialogueTraversePath> GetTraversedPath() const;
	virtual TArray<FDialogueTraversePath> GetTraversedPath_Implementation() const = 0;

	/**
	 * Processes a dialogue command for the Dialogue Participant.
	 *
	 * @param Command   The command to process
	 * @param Payload   Optional payload object for the command
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Participant")
	void ProcessDialogueCommand(const FString& Command, UObject* Payload);
	virtual void ProcessDialogueCommand_Implementation(const FString& Command, UObject* Payload) = 0;

	virtual TScriptInterface<IMounteaDialogueManagerInterface> GetDialogueManager() const = 0;
	
#pragma endregion

#pragma region EventHandles

	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() = 0;
	virtual FDialogueParticipantStateChanged& GetDialogueParticipantStateChangedEventHandle() = 0;
	virtual FDialogueParticipantAudioComponentChanged& GetDialogueParticipantAudioComponentChangedEventHandle() = 0;
	virtual FParticipantStartingNodeSaved& GetParticipantStartingNodeSavedEventHandle() = 0;
	virtual FParticipantCommandRequested& GetParticipantCommandRequestedEventHandle() = 0;
	virtual FDialogueUpdated& GetDialogueUpdatedEventHandle() = 0;

#pragma endregion 
};
