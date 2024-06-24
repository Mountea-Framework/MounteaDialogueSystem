// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
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

struct FDialogueRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueGraphChanged,										UMounteaDialogueGraph*, NewGraph);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantStateChanged,						const EDialogueParticipantState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantAudioComponentChanged,	const UAudioComponent*, NewAudioComp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FParticipantStartingNodeSaved,							const UMounteaDialogueGraphNode*, NewSavedNode);

/**
 * 
 */
USTRUCT(BlueprintType)
struct FDialogueTraversePath
{
	GENERATED_BODY()

	FDialogueTraversePath() : NodeGuid(FGuid::NewGuid()), TraverseCount(0) {}
	FDialogueTraversePath(const FGuid& Guid, const int32 AddCount) : NodeGuid(Guid), TraverseCount(0)
	{
		TraverseCount += FMath::Max(0, AddCount);
	};
	FDialogueTraversePath(const FGuid& Guid, const int32& Count) : NodeGuid(Guid), TraverseCount(Count) {};

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	FGuid NodeGuid = FGuid::NewGuid();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	int32 TraverseCount;

public:

	bool operator==(const FDialogueTraversePath& Other) const
	{
		return Other.NodeGuid == NodeGuid;
	}

	bool operator==(const FGuid& Other) const
	{
		return Other == NodeGuid;
	}

	bool operator!=(const FDialogueTraversePath& Other) const
	{
		return !(*this == Other);
	}

	friend uint32 GetTypeHash(const FDialogueTraversePath& Path)
	{
		return GetTypeHash(Path.NodeGuid);
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

#pragma region EventFunctions
	
	/*
	 * A way to determine whether the Dialogue can even start.
	 * It does come with Native C++ implementation, which can be overriden in child C++ classes.
	 * ❗ If you are using Blueprint implementation, don't forget to call Parent Node, which contains all parent implementations.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	bool CanStartDialogueEvent() const;

	/**
	 * Returns the owning actor for this Dialogue Participant Component.
	 *
	 * @return The owning actor for this Dialogue Participant Component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	AActor* GetOwningActor() const;

	/**
	 * Saves the starting node for this Dialogue Participant Component.
	 *
	 * @param NewStartingNode The node to set as the starting node
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	void SaveStartingNode(UMounteaDialogueGraphNode* NewStartingNode);
	
	/**
	 * Saves the traversed path for this Dialogue Participant Component.
	 * This function is called once Dialogue ends and is updated from Dialogue Context.
	 *
	 * @param InPath The traversed path of the dialogue graph to be saved.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	void SaveTraversedPath(TArray<FDialogueTraversePath>& InPath);

	/**
	 * Interface call.
	 * Retrieves current Dialogue Participant State.
	 * State defines whether Participant can start/close dialogue or not.
	 * 
	 * @return ParticipantState	Participant state value
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	EDialogueParticipantState GetState() const;

	/**
	 * Getter for Participant Gameplay Tag.
	 * @return Participant Gameplay Tag if any is associated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	FGameplayTag GetTag() const;
	
	/**
	 * Helps initialize Participant.
	 * ❔ Is being called in BeginPlay.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue")
	void InitializeParticipant();

	/**
	 * Plays the given participant voice sound.
	 * @param ParticipantVoice The sound to play.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be played.
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	void PlayParticipantVoice(USoundBase* ParticipantVoice);

	/**
	 * Skips the given participant voice sound and whole row. Will automatically start new Row if any is available.
	 * @param ParticipantVoice The sound to skip. Can be left empty.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be skipped.
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	void SkipParticipantVoice(USoundBase* ParticipantVoice);
	
#pragma endregion

public:

#pragma region EventFunctions_Implementations

	bool CanStartDialogueEvent_Implementation() const
	{
		return CanStartDialogue();
	};

	virtual void SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode) = 0;

	virtual AActor* GetOwningActor_Implementation() const = 0;

	virtual void SaveTraversedPath_Implementation(TArray<FDialogueTraversePath>& InPath) = 0;

	EDialogueParticipantState GetState_Implementation() const
	{ return GetParticipantState(); };

	FGameplayTag GetTag_Implementation() const
	{ return GetParticipantTag(); }

#pragma endregion 

public:

#pragma region Functions
	
	/**
	 * Checks if the Participant can be start Dialogue.
	 * ❔ To enhance this, you can implement 'CanStartDialogueEvent' and add custom checks to that function.
	 *
	 * @return Whether the dialogue can be started
	 */
	virtual bool CanStartDialogue() const = 0;

	/**
	 * Gets the saved starting node for this Dialogue Participant.
	 * ❗ Could be null
	 * 
	 * @return The saved starting node, or nullptr if there is none
	 */
	virtual UMounteaDialogueGraphNode* GetSavedStartingNode() const = 0;

	/**
	 * Tries to play the specified sound as the voice of this dialogue participant.
	 *
	 * @param ParticipantVoice The sound to play as the voice of this dialogue participant
	 */
	virtual void PlayParticipantVoice_Implementation(USoundBase* ParticipantVoice) = 0;
	/**
	 * Tries to skip the specified sound this participant is playing as voice.
	 *
	 * @param ParticipantVoice The sound to skip this participant is playing as voice.
	 */
	virtual void SkipParticipantVoice_Implementation(USoundBase* ParticipantVoice) = 0;
	
	/**
	 * Returns the dialogue graph assigned to this Participant.
	 * ❔ Could be updated using 'SetDialogueGraph', providing ability to swith Dialogue graphs on fly
	 * ❗ Could be null
	 *
	 * @return A pointer to the dialogue graph
	 */
	virtual UMounteaDialogueGraph* GetDialogueGraph() const = 0;
	/**
	 * Sets new Dialogue graph for this Participant.
	 * ❗ Should not be null
	 *
	 * @param NewDialogueGraph	A pointer to the dialogue graph to be used
	 */
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) = 0;

	/**
	 * Returns the current state of the Dialogue Participant.
	 */
	virtual EDialogueParticipantState GetParticipantState() const = 0;
	/**
	 * Sets the state of the dialogue participant to the given state.
	 * 
	 * @param NewState The new state to set the dialogue participant to
	 */
	virtual void SetParticipantState(const EDialogueParticipantState NewState) = 0;
	/**
	 * Returns the default state of the Dialogue Participant.
	 */
	virtual EDialogueParticipantState GetDefaultParticipantState() const = 0;
	/**
	 * Sets the Default state of the dialogue participant to the given state.
	 * 
	 * @param NewState The new state to set the dialogue participant to
	 */
	virtual void SetDefaultParticipantState(const EDialogueParticipantState NewState) = 0;

	/**
	 * Returns the audio component used to play the participant voices.
	 * ❗ Could be null
	 */
	virtual UAudioComponent* GetAudioComponent() const = 0;
	/**
	 * Sets the audio component used to play dialogue audio.
	 *
	 * @param NewAudioComponent The new audio component to use for dialogue audio.
	 */
	virtual void SetAudioComponent(UAudioComponent* NewAudioComponent) = 0;

	/**
	 * Returns the map of nodes traversed during the dialogue.
	 * 
	 * @return The map of nodes traversed during the dialogue.
	 */
	virtual TArray<FDialogueTraversePath> GetTraversedPath() const = 0;

	virtual FGameplayTag GetParticipantTag() const = 0;

#pragma endregion

#pragma region EventHandles

	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() = 0;
	virtual FDialogueParticipantStateChanged& GetDialogueParticipantStateChangedEventHandle() = 0;
	virtual FDialogueParticipantAudioComponentChanged& GetDialogueParticipantAudioComponentChangedEventHandle() = 0;
	virtual FParticipantStartingNodeSaved& GetParticipantStartingNodeSavedEventHandle() = 0;

#pragma endregion 
};
