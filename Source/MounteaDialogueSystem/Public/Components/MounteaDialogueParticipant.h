// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"
#include "Interfaces/Core/MounteaDialogueTickableObject.h"
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
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipant : public UActorComponent, public IMounteaDialogueParticipantInterface, public IMounteaDialogueTickableObject
{
	GENERATED_BODY()

public:

	UMounteaDialogueParticipant();

protected:
		
	virtual void BeginPlay() override;	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#pragma region Functions

public:
	
	virtual void InitializeParticipant_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;

	/**
	 * Finds an audio component using FindAudioComponentByName or FindAudioComponentByTag.
	 * ❗ Returns null if 'AudioComponentIdentification' is invalid!
	 * 
	 * @return The found audio component, or nullptr if not found.
 	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	UAudioComponent* FindAudioComponent() const;
	
	/**
	 * Finds an audio component by the specified Name.
	 * 
	 * @param Arg The Name to search for.
	 * @return The found audio component, or nullptr if not found.
 	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	UAudioComponent* FindAudioComponentByName(const FName& Arg) const;
	
	/**
	 * Finds an audio component by the specified tag.
	 * 
	 * @param Arg The tag to search for.
	 * @return The found audio component, or nullptr if not found.
 	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	UAudioComponent* FindAudioComponentByTag(const FName& Arg) const;
	
	virtual void PlayParticipantVoice_Implementation(USoundBase* ParticipantVoice) override;
	
	virtual  void SkipParticipantVoice_Implementation(USoundBase* ParticipantVoice) override;

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion 

#pragma region Variables

protected:

	/**
	 * Mountea Dialogue Graph.
	 * ❗ In order to start Dialogue, this value must be filled.
	 * ❔ Can be updated using SetDialogueGraph function.
	 *
	 * Set Graph is allowed only outside active Dialogue.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_DialogueGraph, SaveGame, EditAnywhere, Category="Mountea|Dialogue", meta=(NoResetToDefault))
	TObjectPtr<UMounteaDialogueGraph> DialogueGraph = nullptr;

	/**
	 * Mountea Dialogue Participant Default State.
	 * ❔ Is used in BeginPlay to set ParticipantState.
	 * ❔ Is used as fallback value once Dialogue Ends.
	 */
	UPROPERTY(Replicated, SaveGame, EditAnywhere, Category="Mountea|Dialogue|Participant", meta=(NoResetToDefault))
	EDialogueParticipantState DefaultParticipantState;

	/**
	* State of the Dialogue Participant.
	* ❗ In order to start Dialogue, this value must not be Disabled.
	* ❔ Can be updated using SetDialogueParticipantState function.
	*/
	UPROPERTY(ReplicatedUsing=OnRep_ParticipantState, Transient, VisibleAnywhere,  Category="Mountea|Dialogue|Participant",  meta=(NoResetToDefault))
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
	UPROPERTY(EditAnywhere, Category="Mountea|Dialogue|Participant", meta=(NoResetToDefault))
	FName AudioComponentIdentification;
	
	/**
	 * Audio Component for Dialogue Participant Voice.
	 * ❗ Is populated by FindAudioComponent is called.
	 */
	UPROPERTY(SaveGame, VisibleAnywhere, Category="Mountea|Dialogue|Participant", AdvancedDisplay, meta=(DisplayThumbnail=false, NoResetToDefault))
	TObjectPtr<UAudioComponent> AudioComponent = nullptr;
	
	/**
	 * Optional Starting Node.
	 *
	 * If this value is selected, this Participant's Dialogue will start from Selected Node, if valid!
	 * Otherwise it will start from Start Node of the Graph.
	 */
	UPROPERTY(Replicated, SaveGame, VisibleAnywhere, Category="Mountea|Dialogue|Participant", AdvancedDisplay, meta=(DisplayThumbnail=false, NoResetToDefault))
	TObjectPtr<UMounteaDialogueGraphNode> StartingNode = nullptr;

	/**
	 * Contains mapped list of Traversed Nodes by GUIDs.
	 * To update Performance, this Path is updated only once Dialogue has finished. Temporary Path is stored in Dialogue Context.
	 */
	UPROPERTY(Replicated, SaveGame, VisibleAnywhere, Category="Mountea|Dialogue|Participant", AdvancedDisplay, meta=(NoResetToDefault))
	TArray<FDialogueTraversePath> TraversedPath;

	/**
	 * Gameplay tag identifying this Participant.
	 * Servers a purpose of being unique ID for Dialogues with multiple Participants.
	 */
	UPROPERTY(Replicated, SaveGame, EditAnywhere, Category="Mountea|Dialogue|Participant", meta=(NoResetToDefault))
	FGameplayTag ParticipantTag;

private:

	UPROPERTY(Transient, BlueprintReadOnly, Category="Mountea|Dialogue|Participant", meta=(AllowPrivateAccess))
	TScriptInterface<IMounteaDialogueManagerInterface> DialogueManager;

#pragma endregion

#pragma region EventVariables
	
protected:
	
	/**
	 * Event called once Dialogue Graph has changed.
	 * Other events can be assigned to this one.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Participant")
	FDialogueGraphChanged OnDialogueGraphChanged;
	/**
	 * Event called once Participant State has changed.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Participant")
	FDialogueParticipantStateChanged OnDialogueParticipantStateChanged;
	/**
	 * Event called once Audio Component has changed.
	 * ❗ Output AudioComponent could be Null
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Participant")
	FDialogueParticipantAudioComponentChanged OnAudioComponentChanged;
	/**
	 * Event called once Starting Node has saved.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Participant")
	FParticipantStartingNodeSaved OnStartingNodeSaved;
	/**
	 * Generic event that will usually be called from Decorator.
	 * Serves purpose of passing Command and Payload from Decorator to Participant (and its owning Actor).
	 *
	 * Useful to retrieve data like Animations etc. from Decorators.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Participant")
	FParticipantCommandRequested ParticipantCommandRequested;
	/**
	 * Event called once Dialogue updates.
	 * Manager calls to Every participant. This serves as notification rather than passing any data.
	 */
	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Participant")
	FDialogueUpdated OnDialogueUpdated;
	
#pragma endregion 

#pragma region EventFunctions
	
protected:
	
	/**
	 * Event called once Dialogue Graph has changed. 
	 * @param NewDialogueGraph	Value of the new Dialogue Graph. Can be null!
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Delegate"))
	void OnDialogueGraphChangedEvent(UMounteaDialogueGraph* NewDialogueGraph);

#pragma endregion 

#pragma region IMounteaDialogueInterface
	
public:
	
	virtual bool CanStartDialogue_Implementation() const override;

	virtual bool CanParticipateInDialogue_Implementation() const override;

	virtual UMounteaDialogueGraphNode* GetSavedStartingNode_Implementation() const override
	{ return StartingNode; };
	
	virtual void SaveStartingNode_Implementation(UMounteaDialogueGraphNode* NewStartingNode) override;

	virtual UMounteaDialogueGraph* GetDialogueGraph_Implementation() const override
	{ return DialogueGraph; };

	virtual void SetDialogueGraph_Implementation(UMounteaDialogueGraph* NewDialogueGraph) override;

	virtual EDialogueParticipantState GetParticipantState_Implementation() const override;

	virtual void SetParticipantState_Implementation(const EDialogueParticipantState NewState) override;
	
	virtual EDialogueParticipantState GetDefaultParticipantState_Implementation() const override
	{ return  DefaultParticipantState; };
	
	virtual void SetDefaultParticipantState_Implementation(const EDialogueParticipantState NewState) override;
	
	virtual UAudioComponent* GetAudioComponent_Implementation() const override
	{ return AudioComponent; };
	
	virtual void SetAudioComponent_Implementation(UAudioComponent* NewAudioComponent) override;
	
	virtual AActor* GetOwningActor_Implementation() const override
	{ return GetOwner();	};
	
	virtual TArray<FDialogueTraversePath> GetTraversedPath_Implementation() const override
	{ return TraversedPath; };
	
	virtual void SaveTraversedPath_Implementation(TArray<FDialogueTraversePath>& InPath) override;

	virtual FGameplayTag GetParticipantTag_Implementation() const override
	{ return ParticipantTag; };
	
	virtual void ProcessDialogueCommand_Implementation(const FString& Command, UObject* Payload) override
	{ ParticipantCommandRequested.Broadcast(Command, Payload); };

	virtual TScriptInterface<IMounteaDialogueManagerInterface> GetDialogueManager() const override
	{ return DialogueManager; };
	
#pragma region EventHandleGetters
	
	virtual FDialogueGraphChanged& GetDialogueGraphChangedEventHandle() override
	{ return OnDialogueGraphChanged; };
	virtual FDialogueParticipantStateChanged& GetDialogueParticipantStateChangedEventHandle() override
	{ return OnDialogueParticipantStateChanged; };
	virtual FDialogueParticipantAudioComponentChanged& GetDialogueParticipantAudioComponentChangedEventHandle() override
	{ return OnAudioComponentChanged; };
	virtual FParticipantStartingNodeSaved& GetParticipantStartingNodeSavedEventHandle() override
	{ return OnStartingNodeSaved; };
	virtual FParticipantCommandRequested& GetParticipantCommandRequestedEventHandle() override
	{return ParticipantCommandRequested; };
	virtual FDialogueUpdated& GetDialogueUpdatedEventHandle() override
	{ return OnDialogueUpdated; };
	
#pragma endregion 

#pragma endregion

#pragma region TickableInterface
	
public:
	virtual void RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable) override;
	virtual void UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable) override;
	virtual void TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick, float DeltaTime) override;
	virtual FMounteaDialogueTick& GetMounteaDialogueTickHandle() override {return ParticipantTickEvent; };

	UPROPERTY(BlueprintAssignable, BlueprintReadOnly, Category="Mountea|Dialogue|Participant")
	FMounteaDialogueTick ParticipantTickEvent;
	
#pragma endregion

#pragma region Functions

protected:

	virtual void UpdateParticipantTick();
	
	UFUNCTION()
	void OnRep_DialogueGraph();
	UFUNCTION()
	void OnRep_ParticipantState();
	UFUNCTION(Server, Reliable)
	void SetParticipantState_Server(const EDialogueParticipantState NewState);
	UFUNCTION(Server, Reliable)
	void SetDefaultParticipantState_Server(const EDialogueParticipantState NewState);
	UFUNCTION(Server, Reliable)
	void SetAudioComponent_Server(UAudioComponent* NewAudioComponent);
	UFUNCTION(Server, Reliable)
	void SetDialogueGraph_Server(UMounteaDialogueGraph* NewGraph);

#pragma endregion

#if WITH_EDITORONLY_DATA

	virtual void RegisterWithPIEInstance();
	virtual void UnregisterFromPIEInstance();
	virtual int32 GetCurrentPIEInstanceID() const;
	
#endif
	
};