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
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue")
	bool CanStartDialogueEvent() const;
	
#pragma endregion

protected:

#pragma region EventFunctions_Implementations

	bool CanStartDialogueEvent_Implementation() const
	{
		switch (GetParticipantState())
		{
			case EDialogueParticipantState::EDPS_Active:
			case EDialogueParticipantState::EDPS_Disabled:
				return false;
			case EDialogueParticipantState::EDPS_Enabled:
				return true;
		}

		return true;
	};

#pragma endregion 

public:

#pragma region Functions
	
	virtual bool CanStartDialogue() const = 0;

	virtual UMounteaDialogueGraph* GetDialogueGraph() const = 0;
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) = 0;

	virtual EDialogueParticipantState GetParticipantState() const = 0;
	virtual void SetParticipantState(const EDialogueParticipantState NewState) = 0;
	virtual EDialogueParticipantState GetDefaultParticipantState() const = 0;
	virtual void SetDefaultParticipantState(const EDialogueParticipantState NewState) = 0;
	
#pragma endregion

#pragma region EventHandles

	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() = 0;
	virtual FDialogueParticipantStateChanged& GetDialogueParticipantStateChangedEventHandle() = 0;

#pragma endregion 
};
