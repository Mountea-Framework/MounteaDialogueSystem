// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/MounteaDialogueInterface.h"
#include "MounteaDialogueParticipant.generated.h"

class UMounteaDialogueGraphNode_CompleteNode;
class UMounteaDialogueGraphNode_DialogueNodeBase;

/**
 * Mountea Dialogue Participant Component.
 *
 * ❔This Component allows any Actor to be 
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, hideCategories=(Collision, AssetUserData, Cooking, ComponentTick, Activation, Rendering, Sockets), meta=(BlueprintSpawnableComponent, DisplayName = "Mountea Dialogue Participant"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipant : public UActorComponent, public IMounteaDialogueInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueParticipant();

protected:

	virtual void BeginPlay() override;

#pragma region Variables

protected:

	/**
	 * Mountea Dialogue Graph.
	 * ❗In order to start Dialogue, this value must be filled.
	 * ❔Can be updated using SetDialogueGraph function.
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue", meta=(DisplayThumbnail=false, NoResetToDefault))
	UMounteaDialogueGraph* DialogueGraph = nullptr;

	/**
	 * Currently Active Mountea Dialogue Node.
	 * ❗This node is not supposed to be changed manually.
	 */
	UPROPERTY(SaveGame, VisibleAnywhere, Category="Mountea|Dialogue", meta=(DisplayThumbnail=false))
	UMounteaDialogueGraphNode* ActiveNode = nullptr;

	FTimerHandle TimerHandle_RowTimer;

#pragma endregion

#pragma region EventVariables
	
protected:
	
	/**
	 * Event called once Dialogue Graph has changed.
	 * Other events can be assigned to this one.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueGraphChanged OnDialogueGraphChanged;
	/**
	 * Event called once Dialogue Starts.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueStarted OnDialogueStarted;
	/**
	 * Event called once Dialogue is Finished.
	 * ❕Finished doesn't mean Completed. This event is called every time you Exit Dialogue.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueFinished OnDialogueFinished;
	/**
	 * Event called every time Active Node has changed.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueActiveNodeChanged OnDialogueActiveNodeChanged;
	/**
	 * 
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueNodeExecuted OnDialogueNodeExecuted;
	/**
	 * 
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueRowExecuted OnDialogueRowExecuted;
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueNodeFinished OnDialogueNodeFinished;
	/**
	 * Event called when player wants to exit Dialogue.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueExitRequested OnDialogueExitRequested;
	/**
	 * Even called when Skip is requested. This indicates that skip should be performed, if allowed.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueSkipStarted OnDialogueSkipRequested;
	/**
	 * Even called when Skip has finished. This only happens if Skip is allowed.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueSkipFinished OnDialogueSkipFinished;

#pragma endregion 

#pragma region EventFunctions
	
protected:
	
	/**
	 * Event called once Dialogue Graph has changed. 
	 * @param NewDialogueGraph	Value of the new Dialogue Graph. Can be null!
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueGraphChangedEvent(UMounteaDialogueGraph* NewDialogueGraph);

	/**
	 * Event called once Dialogue Starts.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueStartedEvent(UMounteaDialogueGraphNode* StartingNode);
	/**
	 * Event called once Dialogue is Finished.
	 * ❕Finished doesn't mean Completed. This event is called every time you Exit Dialogue.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueFinishedEvent(UMounteaDialogueGraphNode* LastActiveNode);
	/**
	 * Event called every time Active Node has changed.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueActiveNodeChangedEvent(UMounteaDialogueGraphNode* NewActiveNode);
	/**
	 * 
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueNodeExecutedEvent(UMounteaDialogueGraphNode* Node);
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueRowExecutedEvent(UMounteaDialogueGraphNode* Node, const FDialogueRow& Row, int32 Index);
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueNodeFinishedEvent(UMounteaDialogueGraphNode* Node);
	/**
	 * Event called when player wants to exit Dialogue.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueExitRequestedEvent(UMounteaDialogueGraphNode* LastActiveNode);
	/**
	 * Even called when Skip is requested. This indicates that skip should be performed, if allowed.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueSkipRequestedEvent(UMounteaDialogueGraphNode* NodeToSkip);
	/**
	 * Even called when Skip has finished. This only happens if Skip is allowed.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueSkipFinishedEvent(UMounteaDialogueGraphNode* NodeSkipped);

#pragma endregion 

#pragma region IMounteaDialogueInterface
	
public:

	/**
	 * Returns whether Dialogue Can start or not.
	 * Returns CanStartDialogueEvent.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool CanStartDialogue() const override;

	/**
	 * Tries to start Dialogue Event.
	 * Call CanStartDialogue function to validate this Dialogue can even start.
	 * If so, Dialogue starts.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual bool StartDialogue() override;

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void ExecuteDialogueNode(UMounteaDialogueGraphNode* Node) override;

#pragma region GettersSetters
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueGraph* GetDialogueGraph() const override
	{ return DialogueGraph; };
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueGraphNode* GetStartNode() const override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueGraphNode* GetActiveNode() const override;
	virtual void SetActiveNode(UMounteaDialogueGraphNode* NewActiveNode) override;

#pragma endregion 

#pragma region EventHandleGetters
	
	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() override
	{ return OnDialogueGraphChanged; };
	virtual FDialogueStarted& GetDialogueStartedEventHandle() override
	{ return OnDialogueStarted; };
	virtual FDialogueFinished& GetDialogueFinishedEventHandle() override
	{ return OnDialogueFinished; };
	virtual FDialogueActiveNodeChanged& GetDialogueActiveNodeChangedEventHandle() override
	{ return OnDialogueActiveNodeChanged; };
	virtual FDialogueNodeExecuted& GetDialogueNodeExecutedEventHandle() override
	{ return OnDialogueNodeExecuted; };
	virtual FDialogueRowExecuted& GetDialogueRowExecutedEventHandle() override
	{ return OnDialogueRowExecuted; };
	virtual FDialogueNodeFinished& GetDialogueNodeFinishedEventHandle() override
	{ return OnDialogueNodeFinished; };
	virtual FDialogueExitRequested& GetDialogueExitRequestedEventHandle() override
	{ return OnDialogueExitRequested; };
	virtual FDialogueSkipStarted& GetDialogueSkipStartedEventHandle() override
	{return OnDialogueSkipRequested; };
	virtual FDialogueSkipFinished& GetDialogueSkipFinishedEventHandle() override
	{return OnDialogueSkipFinished; };

#pragma endregion 

#pragma endregion

#pragma region Functions
public:

	virtual void ExecuteNode_DialogueNode(UMounteaDialogueGraphNode_DialogueNodeBase* Node);
	virtual void ExecuteNode_CompleteNode(UMounteaDialogueGraphNode_CompleteNode* Node);

	UFUNCTION() virtual void StartExecuteDialogueRow(const FDialogueRow& DialogueRow, int32 Index);
	UFUNCTION() virtual void FinishedExecuteDialogueRow(const FDialogueRow& DialogueRow, int32 Index);
#pragma endregion
	
};
