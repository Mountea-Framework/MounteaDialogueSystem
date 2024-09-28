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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueGraphChanged,						UMounteaDialogueGraph*, NewGraph);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantStateChanged,			const EDialogueParticipantState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantAudioComponentChanged,	const UAudioComponent*, NewAudioComp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FParticipantStartingNodeSaved,				const UMounteaDialogueGraphNode*, NewSavedNode);

/**
 * 
 */
USTRUCT(BlueprintType)
struct FDialogueTraversePath
{
	GENERATED_BODY()

	FDialogueTraversePath() : NodeGuid(FGuid::NewGuid()), GraphGuid(FGuid::NewGuid()), TraverseCount(0) {}
	FDialogueTraversePath(const FGuid& Guid, const FGuid& InGraphGuid, const int32 AddCount) : NodeGuid(Guid), GraphGuid(InGraphGuid), TraverseCount(0)
	{
		TraverseCount += FMath::Max(0, AddCount);
	};
	FDialogueTraversePath(const FGuid& Guid, const FGuid& InGraphGuid, const int32& Count) : NodeGuid(Guid), GraphGuid(InGraphGuid), TraverseCount(Count) {};

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	FGuid NodeGuid = FGuid::NewGuid();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	FGuid GraphGuid = FGuid::NewGuid();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue|TraversePath")
	int32 TraverseCount;

public:

	bool operator==(const FDialogueTraversePath& Other) const
	{
		return Other.NodeGuid == NodeGuid && Other.GraphGuid == GraphGuid;
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
		return HashCombine(GetTypeHash(Path.NodeGuid), GetTypeHash(Path.GraphGuid));
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
	
	/*
	 * A way to determine whether the Dialogue can even start.
	 * It does come with Native C++ implementation, which can be overriden in child C++ classes.
	 * ❗ If you are using Blueprint implementation, don't forget to call Parent Node, which contains all parent implementations.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Validate"))
	bool CanStartDialogue() const;
	virtual bool CanStartDialogue_Implementation() const = 0;
	
	/**
	 * Returns the owning actor for this Dialogue Participant Component.
	 *
	 * @return The owning actor for this Dialogue Participant Component.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	AActor* GetOwningActor() const;
	virtual AActor* GetOwningActor_Implementation() const = 0;
	
	/**
	 * Saves the starting node for this Dialogue Participant Component.
	 *
	 * @param NewStartingNode The node to set as the starting node
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void SaveStartingNode(UMounteaDialogueGraphNode* NewStartingNode);
	virtual void SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode) = 0;
	
	/**
	 * Saves the traversed path for this Dialogue Participant Component.
	 * This function is called once Dialogue ends and is updated from Dialogue Context.
	 *
	 * @param InPath The traversed path of the dialogue graph to be saved.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void SaveTraversedPath(TArray<FDialogueTraversePath>& InPath);
	virtual void SaveTraversedPath_Implementation(TArray<FDialogueTraversePath>& InPath) = 0;
	
	/**
	 * Getter for Participant Gameplay Tag.
	 * @return Participant Gameplay Tag if any is associated.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	FGameplayTag GetParticipantTag() const;
	virtual FGameplayTag GetParticipantTag_Implementation() const = 0;
	
	/**
	 * Helps initialize Participant.
	 * ❔ Is being called in BeginPlay.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void InitializeParticipant();
	virtual void InitializeParticipant_Implementation() = 0;

	/**
	 * Plays the given participant voice sound.
	 * @param ParticipantVoice The sound to play.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be played.
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void PlayParticipantVoice(USoundBase* ParticipantVoice);
	virtual void PlayParticipantVoice_Implementation(USoundBase* ParticipantVoice) = 0;

	/**
	 * Skips the given participant voice sound and whole row. Will automatically start new Row if any is available.
	 * @param ParticipantVoice The sound to skip. Can be left empty.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be skipped.
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void SkipParticipantVoice(USoundBase* ParticipantVoice);
	virtual void SkipParticipantVoice_Implementation(USoundBase* ParticipantVoice) = 0;
		
	/**
	 * Gets the saved starting node for this Dialogue Participant.
	 * ❗ Could be null
	 * 
	 * @return The saved starting node, or nullptr if there is none
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	UMounteaDialogueGraphNode* GetSavedStartingNode() const;
	virtual UMounteaDialogueGraphNode* GetSavedStartingNode_Implementation() const = 0;
	
	/**
	 * Returns the dialogue graph assigned to this Participant.
	 * ❔ Could be updated using 'SetDialogueGraph', providing ability to switch Dialogue graphs on fly
	 * ❗ Could be null
	 *
	 * @return A pointer to the dialogue graph
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	UMounteaDialogueGraph* GetDialogueGraph() const;
	virtual UMounteaDialogueGraph* GetDialogueGraph_Implementation() const = 0;

	/**
	 * Sets new Dialogue graph for this Participant.
	 * ❗ Should not be null
	 *
	 * @param NewDialogueGraph	A pointer to the dialogue graph to be used
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph);
	virtual void SetDialogueGraph_Implementation(UMounteaDialogueGraph* NewDialogueGraph) = 0;

	/**
	 * Returns the current state of the Dialogue Participant.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	EDialogueParticipantState GetParticipantState() const;
	virtual EDialogueParticipantState GetParticipantState_Implementation() const = 0;
	
	/**
	 * Sets the state of the dialogue participant to the given state.
	 * 
	 * @param NewState The new state to set the dialogue participant to
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void SetParticipantState(const EDialogueParticipantState NewState);
	virtual void SetParticipantState_Implementation(const EDialogueParticipantState NewState) = 0;
	
	/**
	 * Returns the default state of the Dialogue Participant.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	EDialogueParticipantState GetDefaultParticipantState() const;
	virtual EDialogueParticipantState GetDefaultParticipantState_Implementation() const = 0;
	
	/**
	 * Sets the Default state of the dialogue participant to the given state.
	 * 
	 * @param NewState The new state to set the dialogue participant to
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void SetDefaultParticipantState(const EDialogueParticipantState NewState);
	virtual void SetDefaultParticipantState_Implementation(const EDialogueParticipantState NewState) = 0;

	/**
	 * Returns the audio component used to play the participant voices.
	 * ❗ Could be null
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	UAudioComponent* GetAudioComponent() const;
	virtual UAudioComponent* GetAudioComponent_Implementation() const = 0;
	
	/**
	 * Sets the audio component used to play dialogue audio.
	 *
	 * @param NewAudioComponent The new audio component to use for dialogue audio.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void SetAudioComponent(UAudioComponent* NewAudioComponent);
	virtual void SetAudioComponent_Implementation(UAudioComponent* NewAudioComponent) = 0;

	/**
	 * 
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Getter"))
	TArray<FDialogueTraversePath> GetTraversedPath() const;
	virtual TArray<FDialogueTraversePath> GetTraversedPath_Implementation() const = 0;

	/**
	 * 
	 * @param Command 
	 * @param Payload 
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	void ProcessDialogueCommand(const FString& Command, UObject* Payload);
	virtual void ProcessDialogueCommand_Implementation(const FString& Command, UObject* Payload) = 0;
	
#pragma endregion

#pragma region EventHandles

	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() = 0;
	virtual FDialogueParticipantStateChanged& GetDialogueParticipantStateChangedEventHandle() = 0;
	virtual FDialogueParticipantAudioComponentChanged& GetDialogueParticipantAudioComponentChangedEventHandle() = 0;
	virtual FParticipantStartingNodeSaved& GetParticipantStartingNodeSavedEventHandle() = 0;

#pragma endregion 
};
