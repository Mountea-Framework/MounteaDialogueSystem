// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphHelpers.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueParticipantStatics.generated.h"

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class UAudioComponent;
enum class EDialogueParticipantState : uint8;

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipantStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// --- Template functions ------------------------------
	
	template<typename ReturnType, typename Func, typename... Args>
	static ReturnType ExecuteIfImplements(UObject* Target, const TCHAR* FunctionName, Func Function, Args&&... args)
	{
		if (!IsValid(Target))
		{
			LOG_ERROR(TEXT("[%s] Invalid Target provided!"), FunctionName);
			if constexpr (!std::is_void_v<ReturnType>)
				return ReturnType{};
			else return;
		}

		if (Target->Implements<UMounteaDialogueParticipantInterface>())
		{
			if constexpr (std::is_void_v<ReturnType>)
			{
				Function(Target, Forward<Args>(args)...);
				return;
			}
			return Function(Target, Forward<Args>(args)...);
		}

		LOG_ERROR(TEXT("[%s] Target does not implement 'MounteaDialogueParticipantInterface'!"), FunctionName);
		if constexpr (!std::is_void_v<ReturnType>)
			return ReturnType{};
		else return;
	}

public:

	// --- Actor functions ------------------------------

	/**
	 * A way to determine whether the Dialogue can even start.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return True if dialogue can start, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Validate"))
	static bool CanStartDialogue(UObject* Target);

	/**
	 * A way to determine whether the Dialogue can even start.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return True if dialogue can start, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Can Start Dialogue (Interface)", meta=(CustomTag="MounteaK2Validate"))
	static bool CanStartDialogueV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	/**
	 * Returns the owning actor for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return The owning actor for this Dialogue Participant
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static AActor* GetOwningActor(UObject* Target);

	/**
	 * Returns the owning actor for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return The owning actor for this Dialogue Participant
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Owning Actor (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static AActor* GetOwningActorV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	/**
	 * Gets the participant's gameplay tag.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return The participant's gameplay tag
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static FGameplayTag GetParticipantTag(UObject* Target);

	/**
	 * Gets the participant's gameplay tag.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return The participant's gameplay tag
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Participant Tag (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static FGameplayTag GetParticipantTagV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	/**
	 * Initializes the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void InitializeParticipant(UObject* Target, const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);

	/**
	 * Initializes the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Initialize Participant (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void InitializeParticipantV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const TScriptInterface<IMounteaDialogueManagerInterface>& Manager);

	/**
	 * Gets the current state of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return The current participant state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueParticipantState GetParticipantState(UObject* Target);

	/**
	 * Gets the current state of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return The current participant state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Participant State (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueParticipantState GetParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	/**
	 * Sets the state of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param NewState  The state to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void SetParticipantState(UObject* Target, const EDialogueParticipantState NewState);

	/**
	 * Sets the state of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @param NewState  The state to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Set Participant State (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SetParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const EDialogueParticipantState NewState);

	/**
	 * Gets the default state of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return The default participant state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueParticipantState GetDefaultParticipantState(UObject* Target);

	/**
	* Gets the default state of the Dialogue Participant.
	*
	* @param Target	Dialogue Participant interface.
	* @return The default participant state
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Default Participant State (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueParticipantState GetDefaultParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	/**
	 * Sets the default state of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param NewState  The default state to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void SetDefaultParticipantState(UObject* Target, const EDialogueParticipantState NewState);

	/**
	 * Sets the default state of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @param NewState  The default state to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Set Default Participant State (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SetDefaultParticipantStateV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const EDialogueParticipantState NewState);
	
	// --- Node functions ------------------------------
	
	/**
	 * Saves the starting node for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param NewStartingNode   The node to set as the starting node
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void SaveStartingNode(UObject* Target, UMounteaDialogueGraphNode* NewStartingNode);

	/**
	 * Saves the starting node for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant interface.
	 * @param NewStartingNode   The node to set as the starting node
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Save Starting Node (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SaveStartingNodeV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UMounteaDialogueGraphNode* NewStartingNode);

	/**
	 * Saves the traversed path for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param InPath	The traversed path to save
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void SaveTraversedPath(UObject* Target, UPARAM(ref) TArray<FDialogueTraversePath>& InPath);

	/**
	 * Saves the traversed path for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @param InPath	The traversed path to save
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Save Traversed Path (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SaveTraversedPathV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UPARAM(ref) TArray<FDialogueTraversePath>& InPath);

	/**
	 * Gets the saved starting node for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return The saved starting node
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraphNode* GetSavedStartingNode(UObject* Target);

	/**
	 * Gets the saved starting node for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return The saved starting node
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Saved Starting Node (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraphNode* GetSavedStartingNodeV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	// --- Audio functions ------------------------------
	
	/**
	 * Plays voice sound for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param ParticipantVoice  The sound to play
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void PlayParticipantVoice(UObject* Target, USoundBase* ParticipantVoice);

	/**
	 * Plays voice sound for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant interface.
	 * @param ParticipantVoice  The sound to play
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Play Participant Voice (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void PlayParticipantVoiceV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, USoundBase* ParticipantVoice);

	/**
	 * Skips the current voice sound for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param ParticipantVoice  The sound to skip
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void SkipParticipantVoice(UObject* Target, USoundBase* ParticipantVoice);

	/**
	 * Skips the current voice sound for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant interface.
	 * @param ParticipantVoice  The sound to skip
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Skip Participant Voice (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SkipParticipantVoiceV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, USoundBase* ParticipantVoice);

	/**
	 * Gets the audio component of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return The audio component used for dialogue
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static UAudioComponent* GetAudioComponent(UObject* Target);

	/**
	 * Gets the audio component of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return The audio component used for dialogue
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Audio Component (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static UAudioComponent* GetAudioComponentV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	/**
	 * Sets the audio component for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param NewAudioComponent The audio component to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void SetAudioComponent(UObject* Target, UAudioComponent* NewAudioComponent);

	/**
	 * Sets the audio component for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant interface.
	 * @param NewAudioComponent The audio component to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Set Audio Component (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SetAudioComponentV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UAudioComponent* NewAudioComponent);

	// --- Graph functions ------------------------------
	
	/**
	 * Gets the dialogue graph for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return The dialogue graph
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraph* GetDialogueGraph(UObject* Target);

	/**
	 * Gets the dialogue graph for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return The dialogue graph
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Dialogue Graph (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraph* GetDialogueGraphV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	/**
	 * Sets the dialogue graph for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param NewDialogueGraph  The dialogue graph to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueGraph(UObject* Target, UMounteaDialogueGraph* NewDialogueGraph);

	/**
	 * Sets the dialogue graph for the Dialogue Participant.
	 *
	 * @param Target			Dialogue Participant interface.
	 * @param NewDialogueGraph  The dialogue graph to set
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Set Dialogue Graph (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueGraphV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, UMounteaDialogueGraph* NewDialogueGraph);
	
	/**
	 * Gets the traversed path of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @return Array of traversed dialogue paths
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Getter"))
	static TArray<FDialogueTraversePath> GetTraversedPath(UObject* Target);

	/**
	 * Gets the traversed path of the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @return Array of traversed dialogue paths
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Participant", DisplayName="Get Traversed Path (Interface)", meta=(CustomTag="MounteaK2Getter"))
	static TArray<FDialogueTraversePath> GetTraversedPathV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target);

	// --- Commands functions ------------------------------
	
	/**
	 * Processes a dialogue command for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant object. Must implement `IMounteaDialogueParticipant` interface.
	 * @param Command   The command to process
	 * @param Payload   Optional payload object for the command
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessDialogueCommand(UObject* Target, const FString& Command, UObject* Payload);

	/**
	 * Processes a dialogue command for the Dialogue Participant.
	 *
	 * @param Target	Dialogue Participant interface.
	 * @param Command   The command to process
	 * @param Payload   Optional payload object for the command
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant", DisplayName="Process Dialogue Command (Interface)", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessDialogueCommandV2(const TScriptInterface<IMounteaDialogueParticipantInterface>& Target, const FString& Command, UObject* Payload);
};
