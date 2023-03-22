// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "UObject/Interface.h"
#include "MounteaDialogueParticipantInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType, Blueprintable)
class UMounteaDialogueParticipantInterface : public UInterface
{
	GENERATED_BODY()
};

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;

struct FDialogueRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueGraphChanged, UMounteaDialogueGraph*, NewGraph);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantStateChanged, const EDialogueParticipantState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueParticipantAudioComponentChanged, const UAudioComponent*, NewAudioComp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FParticipantStartingNodeSaved, const UMounteaDialogueGraphNode*, NewSavedNode);

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
	 * ⚠ If you are using Blueprint implementation, don't forget to call Parent Node, which contains all parent implementations.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	bool CanStartDialogueEvent() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	AActor* GetOwningActor() const;
		
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	void SaveStartingNode(UMounteaDialogueGraphNode* NewStartingNode);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Mountea|Dialogue")
	void SaveTraversedPath(TMap<FGuid,int32>& InPath);

#pragma endregion

protected:

#pragma region EventFunctions_Implementations

	bool CanStartDialogueEvent_Implementation() const
	{
		return CanStartDialogue();
	};

	virtual void SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode)
	{
		//Implement logic in child Blueprints
	}

	virtual AActor* GetOwningActor_Implementation() const
	{
		return nullptr;
	};

	virtual void SaveTraversedPath_Implementation(TMap<FGuid,int32>& InPath)
	{
		// Implement logic in children
	};

#pragma endregion 

public:

#pragma region Functions
	
	virtual bool CanStartDialogue() const = 0;

	virtual UMounteaDialogueGraphNode* GetSavedStartingNode() const = 0;

	virtual void PlayParticipantVoice(USoundBase* ParticipantVoice) = 0;
	virtual void SkipParticipantVoice(USoundBase* ParticipantVoice) = 0;

	virtual UMounteaDialogueGraph* GetDialogueGraph() const = 0;
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) = 0;

	virtual EDialogueParticipantState GetParticipantState() const = 0;
	virtual void SetParticipantState(const EDialogueParticipantState NewState) = 0;
	virtual EDialogueParticipantState GetDefaultParticipantState() const = 0;
	virtual void SetDefaultParticipantState(const EDialogueParticipantState NewState) = 0;

	virtual UAudioComponent* GetAudioComponent() const = 0;
	virtual void SetAudioComponent(UAudioComponent* NewAudioComponent) = 0;

	virtual TMap<FGuid,int32> GetTraversedPath() const = 0;

#pragma endregion

#pragma region EventHandles

	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() = 0;
	virtual FDialogueParticipantStateChanged& GetDialogueParticipantStateChangedEventHandle() = 0;
	virtual FDialogueParticipantAudioComponentChanged& GetDialogueParticipantAudioComponentChangedEventHandle() = 0;
	virtual FParticipantStartingNodeSaved& GetParticipantStartingNodeSavedEventHandle() = 0;

#pragma endregion 
};
