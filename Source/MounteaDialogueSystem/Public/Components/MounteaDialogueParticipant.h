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
 * ❔ This Component allows any Actor to be Dialogue Participant.
 * ❗ Requires Dialogue Graph to work.
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
	 * ❔ Is being called in BeginPlay.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue")
	void InitializeParticipant();
	virtual void InitializeParticipant_Implementation();
	virtual void BeginPlay() override;

#pragma region Functions

protected:

	/**
	 * Finds an audio component using FindAudioComponentByName or FindAudioComponentByTag.
	 * ❗ Returns null if 'AudioComponentIdentification' is invalid!
	 * 
	 * @param Arg The Name to search for.
	 * @return The found audio component, or nullptr if not found.
 	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	UAudioComponent* FindAudioComponent() const;
	
	/**
	 * Finds an audio component by the specified Name.
	 * 
	 * @param Arg The Name to search for.
	 * @return The found audio component, or nullptr if not found.
 	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	UAudioComponent* FindAudioComponentByName(const FName& Arg) const;
	
	/**
	 * Finds an audio component by the specified tag.
	 * 
	 * @param Arg The tag to search for.
	 * @return The found audio component, or nullptr if not found.
 	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	UAudioComponent* FindAudioComponentByTag(const FName& Arg) const;

	/**
	 * Plays the given participant voice sound.
	 * @param ParticipantVoice The sound to play.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be played.
	 */ 
	virtual void PlayParticipantVoice(USoundBase* ParticipantVoice) override;
	/**
	 * Skips the given participant voice sound.
	 * @param ParticipantVoice The sound to skip.
	 * ❗ The sound should be a valid USoundBase object, otherwise nothing will be skipped.
	 */ 
	virtual  void SkipParticipantVoice(USoundBase* ParticipantVoice) override;

#pragma endregion 

#pragma region Variables

protected:

	/**
	 * Mountea Dialogue Graph.
	 * ❗ In order to start Dialogue, this value must be filled.
	 * ❔ Can be updated using SetDialogueGraph function.
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue", meta=(DisplayThumbnail=false, NoResetToDefault))
	UMounteaDialogueGraph* DialogueGraph = nullptr;

	/**
	 * Mountea Dialogue Participant Default State.
	 * ❔ Is used in BeginPlay to set ParticipantState.
	 * ❔ Is used as fallback value once Dialogue Ends.
	 */
	UPROPERTY(SaveGame, EditAnywhere, Category="Mountea|Dialogue", meta=(NoResetToDefault))
	EDialogueParticipantState DefaultParticipantState;

	/**
	* State of the Dialogue Participant.
	* ❗ In order to start Dialogue, this value must not be Disabled.
	* ❔ Can be updated using SetDialogueParticipantState function.
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
	 * Audio Component for Dialogue Participant Voice.
	 * ❗ Is populated by FindAudioComponent is called.
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

	/**
	 * Contains mapped list of Traversed Nodes by GUIDs.
	 * To update Performance, this Path is updated only once Dialogue has finished. Temporary Path is stored in Dialogue Context.
	 */
	UPROPERTY(SaveGame, VisibleAnywhere, Category="Mountea", AdvancedDisplay, meta=(NoResetToDefault))
	TMap<FGuid, int32> TraversedPath;

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
	 * Event called once Participant State has changed.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueParticipantStateChanged OnDialogueParticipantStateChanged;
	/**
	 * Event called once Audio Component has changed.
	 * ❗ Output AudioComponent could be Null
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue")
	FDialogueParticipantAudioComponentChanged OnAudioComponentChanged;
	/**
	 * Event called once Starting Node has saved.
	 */
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
	/**
	 * Returns the starting node of the Mountea Dialogue Graph saved during runtime.
	 * ❗ In order to have a saved starting node, the function 'SaveStartingNode' must have been called at least once.
	 * ❔ The saved starting node can be updated using 'SaveStartingNode' function.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueGraphNode* GetSavedStartingNode() const override
	{ return StartingNode; };
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode) override;

	/**
	 * Returns Dialogue Graph of this Participant.
	 * ❗ Might return Null❗
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UMounteaDialogueGraph* GetDialogueGraph() const override
	{ return DialogueGraph; };
	/**
	 * Overrides Dialogue Graph for this Participant.
	 * ❗ Accepts Null values❗
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetDialogueGraph(UMounteaDialogueGraph* NewDialogueGraph) override;
	/**
	 * Returns the current state of the Dialogue Participant.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual EDialogueParticipantState GetParticipantState() const override
	{ return  ParticipantState; };
	/**
	 * Sets the current state of the dialogue participant to the given state.
	 * @param NewState The new state to set the dialogue participant to.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetParticipantState(const EDialogueParticipantState NewState) override;
	/**
	 * Returns the Default state of the Dialogue Participant.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual EDialogueParticipantState GetDefaultParticipantState() const override
	{ return  DefaultParticipantState; };
	/**
	 * Sets the Default state of the dialogue participant to the given state.
	 * @param NewState The new state to set the dialogue participant to.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetDefaultParticipantState(const EDialogueParticipantState NewState) override;

	/**
	 * Returns the audio component used to play the participant voices.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UAudioComponent* GetAudioComponent() const override
	{ return AudioComponent; };
	/**
	 * Sets the audio component used to play dialogue audio.
	 *
	 * @param NewAudioComponent The new audio component to use for dialogue audio.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SetAudioComponent(UAudioComponent* NewAudioComponent) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual AActor* GetOwningActor_Implementation() const override;
	
	/**
	 * Returns the map of nodes traversed during the dialogue.
	 * 
	 * @return The map of nodes traversed during the dialogue.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual TMap<FGuid,int32> GetTraversedPath() const override
	{ return TraversedPath; };
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	virtual void SaveTraversedPath_Implementation(TMap<FGuid,int32>& InPath) override;
	

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