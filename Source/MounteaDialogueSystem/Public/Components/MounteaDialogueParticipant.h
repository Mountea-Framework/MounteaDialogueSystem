// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"
#include "MounteaDialogueParticipant.generated.h"

class UMounteaDialogueGraphNode_CompleteNode;
class UMounteaDialogueGraphNode_DialogueNodeBase;

/**
 * Mountea Dialogue Participant Component.
 *
 * ❔This Component allows any Actor to be Dialogue Participant.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, hideCategories=(Collision, AssetUserData, Cooking, ComponentTick, Activation, Rendering, Sockets), meta=(BlueprintSpawnableComponent, DisplayName = "Mountea Dialogue Participant"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipant : public UActorComponent, public IMounteaDialogueParticipantInterface
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

#pragma endregion

#pragma region EventVariables
	
protected:
	
	/**
	 * Event called once Dialogue Graph has changed.
	 * Other events can be assigned to this one.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueGraphChanged OnDialogueGraphChanged;

#pragma endregion 

#pragma region EventFunctions
	
protected:
	
	/**
	 * Event called once Dialogue Graph has changed. 
	 * @param NewDialogueGraph	Value of the new Dialogue Graph. Can be null!
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue")
	void OnDialogueGraphChangedEvent(UMounteaDialogueGraph* NewDialogueGraph);

#pragma endregion 

#pragma region IMounteaDialogueInterface
	
public:

	/**
	 * Returns whether Dialogue Can start or not.
	 * Returns CanStartDialogueEvent.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool CanStartDialogue() const override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueGraph* GetDialogueGraph() const override
	{ return DialogueGraph; };
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) override;

#pragma region EventHandleGetters
	
	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() override
	{ return OnDialogueGraphChanged; };
	
#pragma endregion 

#pragma endregion
	
};
