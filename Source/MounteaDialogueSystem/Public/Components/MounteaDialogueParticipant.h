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
 * ❗Requires Dialogue Graph to work.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, hideCategories=(Collision, AssetUserData, Cooking, Activation, Rendering, Sockets), AutoExpandCategories=("Mountea", "Dialogue", "Mountea|Dialogue"), meta=(BlueprintSpawnableComponent, DisplayName = "Mountea Dialogue Participant"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipant : public UActorComponent, public IMounteaDialogueParticipantInterface
{
	GENERATED_BODY()

public:

	UMounteaDialogueParticipant();

protected:

	/**
	 * Helps initialize Participant.
	 * Is being called in BeginPlay.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue")
	void InitializeParticipant();
	virtual void InitializeParticipant_Implementation();
	virtual void BeginPlay() override;

#pragma region Functions

protected:

	UAudioComponent* FindAudioComponent() const;
	UAudioComponent* FindAudioComponentByName(const FName& Arg) const;
	UAudioComponent* FindAudioComponentByTag(const FName& Arg) const;

	virtual void PlayParticipantVoice(USoundBase* ParticipantVoice) override;
	virtual  void SkipParticipantVoice(USoundBase* ParticipantVoice) override;

#pragma endregion 

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
	 * 
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue", meta=(NoResetToDefault))
	EDialogueParticipantState DefaultParticipantState;
	/**
	 * 
	 */
	UPROPERTY(Transient, VisibleAnywhere,  Category="Mountea", AdvancedDisplay,  meta=(NoResetToDefault))
	EDialogueParticipantState ParticipantState;

	/**
	 * Provides simple way to pass down Audio Component.
	 * Consumes:
	 * * Actor Tag
	 * * Name
	 *
	 * Tries to find 'UAudioComponent' by both methods in the Owner (Parent) Actor.
	 * If any found, it will be set as AudioComponent.
	 *
	 * This is user friendly way to avoid node 'SetAudioComponent'.
	 */
	UPROPERTY(EditAnywhere, Category="Mountea|Dialogue", meta=(NoResetToDefault))
	FName AudioComponentIdentification;
	/**
	 * 
	 */
	UPROPERTY(SaveGame, VisibleAnywhere, Category="Mountea", AdvancedDisplay, meta=(DisplayThumbnail=false, NoResetToDefault))
	UAudioComponent* AudioComponent = nullptr;
	/**
	 * Optional Starting Node.
	 *
	 * If this value is selected, this Participant's Dialogue will start from Selected Node, if valid!
	 * Otherwise it will start from Start Node of the Graph.
	 */
	UPROPERTY(SaveGame, VisibleAnywhere, Category="Mountea", AdvancedDisplay, meta=(DisplayThumbnail=false, NoResetToDefault))
	UMounteaDialogueGraphNode* StartingNode = nullptr;

#pragma endregion

#pragma region EventVariables
	
protected:
	
	/**
	 * Event called once Dialogue Graph has changed.
	 * Other events can be assigned to this one.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueGraphChanged OnDialogueGraphChanged;

	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueParticipantStateChanged OnDialogueParticipantStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueParticipantAudioComponentChanged OnAudioComponentChanged;

	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FParticipantStartingNodeSaved OnStartingNodeSaved;

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
	virtual UMounteaDialogueGraphNode* GetSavedStartingNode() const override
	{ return StartingNode; };
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode) override;

	/**
	 * Returns Dialogue Graph of this Participant.
	 * ❗Might return Null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueGraph* GetDialogueGraph() const override
	{ return DialogueGraph; };
	/**
	 * Overrides Dialogue Graph for this Participant.
	 * ❗Accepts Null values❗
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual EDialogueParticipantState GetParticipantState() const override
	{ return  ParticipantState; };
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetParticipantState(const EDialogueParticipantState NewState) override;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual EDialogueParticipantState GetDefaultParticipantState() const override
	{ return  DefaultParticipantState; };
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetDefaultParticipantState(const EDialogueParticipantState NewState) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UAudioComponent* GetAudioComponent() const override
	{ return AudioComponent; };
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetAudioComponent(UAudioComponent* NewAudioComponent) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual AActor* GetOwningActor_Implementation() const override;

#pragma region EventHandleGetters
	
	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() override
	{ return OnDialogueGraphChanged; };
	virtual FDialogueParticipantStateChanged& GetDialogueParticipantStateChangedEventHandle() override
	{ return OnDialogueParticipantStateChanged; };
	virtual FDialogueParticipantAudioComponentChanged& GetDialogueParticipantAudioComponentChangedEventHandle() override
	{ return OnAudioComponentChanged; };
	virtual FParticipantStartingNodeSaved& GetParticipantStartingNodeSavedEventHandle() override
	{ return OnStartingNodeSaved; };
	
#pragma endregion 

#pragma endregion
	
};
