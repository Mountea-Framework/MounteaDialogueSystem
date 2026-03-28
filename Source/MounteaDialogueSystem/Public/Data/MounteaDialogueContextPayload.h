// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"

#include "MounteaDialogueContextPayload.generated.h"

/**
 * FMounteaDialogueContextPayload is the authoritative, network-replicated state of an active
 * dialogue session. It lives as a UPROPERTY on UMounteaDialogueSession (attached to AGameState),
 * giving it a stable replication path to all clients.
 *
 * The server is the sole mutator. Every mutation increments ContextVersion.
 * Clients receive the full struct via OnRep and rebuild their local read-only
 * UMounteaDialogueContext view from it.
 *
 * @see UMounteaDialogueSession
 * @see UMounteaDialogueContext
 */
USTRUCT(BlueprintType)
struct MOUNTEADIALOGUESYSTEM_API FMounteaDialogueContextPayload
{
	GENERATED_BODY()

public:

	/**
	 * Unique identifier for the owning dialogue session.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	FGuid SessionGUID;

	/**
	 * GUID of the node currently being processed.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	FGuid ActiveNodeGUID;

	/**
	 * GUID of the node that was active immediately before the current node.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	FGuid PreviousNodeGUID;

	/**
	 * GUIDs of child nodes that pass all edge conditions from the current active node.
	 * Used by the client UI to display selectable options.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	TArray<FGuid> AllowedChildNodeGUIDs;

	/**
	 * GUID of the graph currently being traversed.
	 * Changes when OpenChildGraph transitions into a sub-graph.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	FGuid ActiveGraphGUID;

	/**
	 * Stack of parent graph states saved when entering sub-graphs via OpenChildGraph.
	 * Popped when a sub-graph completes to restore the parent context.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	TArray<FDialogueGraphStackEntry> GraphStack;

	/**
	 * The participant that is currently speaking or acting.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	TScriptInterface<IMounteaDialogueParticipantInterface> ActiveDialogueParticipant;

	/**
	 * The participant representing the player in this session.
	 * May be null for NPC-NPC or monologue sessions.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	TScriptInterface<IMounteaDialogueParticipantInterface> PlayerDialogueParticipant;

	/**
	 * The primary NPC participant.
	 * May be null for Player-Player sessions.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant;

	/**
	 * All participants registered for this session, including player and NPCs.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> DialogueParticipants;

	/**
	 * The dialogue row for the current active node, resolved via IMounteaDialogueSpeechDataInterface.
	 * Default (empty) for non-speech nodes (Delay, OpenChildGraph, StartNode, ReturnToNode).
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	FDialogueRow ActiveDialogueRow;

	/**
	 * Index into ActiveDialogueRow.DialogueRowData for the line currently being played.
	 * Advances as each line of the row completes.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	int32 ActiveDialogueRowDataIndex = 0;

	/**
	 * Last widget command string broadcast to dialogue UI.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	FString LastWidgetCommand;

	/**
	 * Monotonically increasing version counter. Incremented on every server-side mutation.
	 * Clients use this to detect skipped updates.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Context")
	int32 ContextVersion = 0;

public:

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	bool IsValid() const
	{
		return SessionGUID.IsValid();
	}

	void Reset()
	{
		SessionGUID.Invalidate();
		ActiveNodeGUID.Invalidate();
		PreviousNodeGUID.Invalidate();
		AllowedChildNodeGUIDs.Empty();
		ActiveGraphGUID.Invalidate();
		GraphStack.Empty();
		ActiveDialogueParticipant = nullptr;
		PlayerDialogueParticipant = nullptr;
		DialogueParticipant = nullptr;
		DialogueParticipants.Empty();
		ActiveDialogueRow = FDialogueRow();
		ActiveDialogueRowDataIndex = 0;
		LastWidgetCommand.Empty();
		ContextVersion = 0;
	}
};

template<>
struct TStructOpsTypeTraits<FMounteaDialogueContextPayload> : public TStructOpsTypeTraitsBase2<FMounteaDialogueContextPayload>
{
	enum
	{
		WithNetSerializer = true
	};
};
