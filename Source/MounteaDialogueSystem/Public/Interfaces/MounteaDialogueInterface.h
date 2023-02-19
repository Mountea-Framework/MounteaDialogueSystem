// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "MounteaDialogueInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UMounteaDialogueParticipantInterface : public UInterface
{
	GENERATED_BODY()
};

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;

struct FDialogueRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueGraphChanged, UMounteaDialogueGraph*, NewGraph);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueStarted, UMounteaDialogueGraphNode*, StartingNode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueFinished, UMounteaDialogueGraphNode*, ActiveNode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueActiveNodeChanged, UMounteaDialogueGraphNode*, NewActiveNode);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeExecuted, UMounteaDialogueGraphNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDialogueRowExecuted, UMounteaDialogueGraphNode*, Node, const FDialogueRow&, Row, int32, RowIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeFinished, UMounteaDialogueGraphNode*, Node);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueExitRequested, UMounteaDialogueGraphNode*, ActiveNode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueSkipStarted, UMounteaDialogueGraphNode*, NodeToSkip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueSkipFinished, UMounteaDialogueGraphNode*, NodeSkipped);

/**
 * Mountea Dialogue Interface.
 * Interface connecting Mountea Dialogue Graph with Mountea Dialogue Component.
 * Mountea Dialogue Components are implementing this Interface.
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

	/*
	 * Trigger function to Start dialogue.
	 * Does only start if Dialogue Graph is valid!
	 * Internal C++ functions calls CanStartDialogueEvent to validate this call.
	 * ⚠ If you are using Blueprint implementation, don't forget to call Parent Node, which contains all parent implementations.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue")
	bool  StartDialogueEvent();

	/**
	 * Executes given Node.
	 * Nodes have abstract function "ExecuteNode" which is triggered.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void ExecuteDialogueNodeEvent();

#pragma endregion

protected:

#pragma region EventFunctions_Implementations

	bool CanStartDialogueEvent_Implementation() const
	{
		return true;
	};

	bool StartDialogueEvent_Implementation()
	{
		if (!CanStartDialogue())
		{
			return false;
		}

		return true;
	};

#pragma endregion 

public:

#pragma region Functions
	
	virtual bool CanStartDialogue() const = 0;
	virtual bool StartDialogue() = 0;

	virtual void ExecuteDialogueNode(UMounteaDialogueGraphNode* Node) = 0;

	virtual UMounteaDialogueGraph* GetDialogueGraph() const = 0;
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) = 0;

	virtual UMounteaDialogueGraphNode* GetStartNode() const = 0;
	
	virtual UMounteaDialogueGraphNode* GetActiveNode() const = 0 ;
	virtual void SetActiveNode(UMounteaDialogueGraphNode* NewActiveNode) = 0;

#pragma endregion

#pragma region EventHandles

	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() = 0;
	virtual FDialogueStarted& GetDialogueStartedEventHandle() = 0;
	virtual FDialogueFinished& GetDialogueFinishedEventHandle() = 0;
	virtual FDialogueActiveNodeChanged& GetDialogueActiveNodeChangedEventHandle() = 0;
	virtual FDialogueNodeExecuted& GetDialogueNodeExecutedEventHandle() = 0;
	virtual FDialogueRowExecuted& GetDialogueRowExecutedEventHandle() = 0;
	virtual FDialogueNodeFinished& GetDialogueNodeFinishedEventHandle() = 0;
	virtual FDialogueExitRequested& GetDialogueExitRequestedEventHandle() = 0;
	virtual FDialogueSkipStarted& GetDialogueSkipStartedEventHandle() = 0;
	virtual FDialogueSkipFinished& GetDialogueSkipFinishedEventHandle() = 0;

#pragma endregion 
};
