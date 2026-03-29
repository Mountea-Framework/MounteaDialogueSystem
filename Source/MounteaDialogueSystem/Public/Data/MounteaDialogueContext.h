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
#include "MounteaDialogueGraphDataTypes.h"
#include "Interfaces/Core/MounteaDialogueConditionContextInterface.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "UObject/Object.h"
#include "MounteaDialogueContext.generated.h"

struct FDialogueTraversePath;
class IMounteaDialogueParticipantInterface;
class UMounteaDialogueGraphNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueContextUpdated);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdatedFromBlueprint, UMounteaDialogueContext*, Context);

/**
 * Dialogue Context.
 * 
 * Contains information needed to successfully start Dialogue.
 * Also helps tracking Dialogue Specific data. Is recycled for whole Dialogue Graph.
 * 
 * In Dialogue Manager Component is used as Transient object, which is nullified once Dialogue ends and is never saved.
 */
UCLASS(Transient)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueContext : public UObject, public IMounteaDialogueConditionContextInterface
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FGuid SessionGUID;

	/**
	 * Active Dialogue Participant Interface reference.
	 * This is the Participant who is Active right now.
	 * ❗ Might be invalid
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TScriptInterface<IMounteaDialogueParticipantInterface> ActiveDialogueParticipant;

	/**
	 * All participants registered for this session.
	 * Use UMounteaDialogueSystemBFC::GetParticipantByType to find players, NPCs, etc.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> DialogueParticipants;
	
	/**
	 * Pointer to the Node which is currently active.
	 * ❗ Might be null
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraphNode> ActiveNode = nullptr;

	/**
	 * Keeps info about the Previous Active Node.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FGuid PreviousActiveNode = FGuid::NewGuid();
	
	/**
	 * List of Nodes that can be accessed from Active Node.
	 * Already filtered to contain only those that can be triggered.
	 * 
	 * ❔ Filter is done by edge conditions on each child node's incoming edge.
	 * ❗ Might be empty
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<TObjectPtr<UMounteaDialogueGraphNode>> AllowedChildNodes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FDataTableRowHandle ActiveDialogueTableHandle;
	
	/**
	 * Active Dialogue Row from Active Node. 
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FDialogueRow ActiveDialogueRow;
	
	/**
	 * Index of currently used Dialogue Row Data row.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	int32 ActiveDialogueRowDataIndex = 0;
	
	/**
	 * Contains mapped list of Traversed Nodes by GUIDs.
	 * Each time Dialogue is updated, like node is selected or starts itself, this Path is updated.
	 * Updates Participant once Dialogue is done.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue", meta=(NoResetToDefault))
	TArray<FDialogueTraversePath> TraversedPath;

	// Should be the last command provided on auth. side. Could be outdated on clients! Use with caution!
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FString LastWidgetCommand;

public:

	UPROPERTY(BlueprintAssignable, Category="Mountea|Dialogue|Context")
	FOnDialogueContextUpdated OnDialogueContextUpdated;

public:

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(CompactNodeTitle="To String"), meta=(CustomTag="MounteaK2Setter"))
	virtual FString ToString() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Context", meta=(CustomTag="MounteaK2Validate"))
	virtual bool IsValid() const;

	TScriptInterface<IMounteaDialogueParticipantInterface> GetActiveDialogueParticipant() const
	{ 
		return ActiveDialogueParticipant; 
	};
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> GetDialogueParticipants() const
	{ 
		return DialogueParticipants; 
	}
	
	/**
	 * Returns the Active Node object.
	 * ❗ Might be null
	 * 
	 * @return Active Node if any specified
	 */
	UMounteaDialogueGraphNode* GetActiveNode() const
	{ 
		return ActiveNode;
	 };
	
	/**
	 * Returns lsit of Children Nodes from Active Node.
	 * ❗ Might be empty
	 * 
	 * @return List of allowed Children Nodes 
	 */
	TArray<UMounteaDialogueGraphNode*> GetChildrenNodes() const
	{ 
		return AllowedChildNodes; 
	};

	FDataTableRowHandle GetActiveDataTable() const
	{ 
		return ActiveDialogueTableHandle; 
	};
	
	/**
	 * Returns Active Dialogue Row if any.
	 * ❗ Might return invalid
	 * 
	 * @return Active Dialogue Row if any 
	 */
	FDialogueRow GetActiveDialogueRow() const
	{ 
		return ActiveDialogueRow; 
	};
	
	/**
	 *Returns the Active Dialogue Row Data Index.
	 * 
	 * @return Active Row Index 
	 */
	int32 GetActiveDialogueRowDataIndex() const
	{ 
		return ActiveDialogueRowDataIndex; 
	};
	
	/**
	 * Returns the map of nodes traversed during this dialogue instance.
	 * 
	 * @return The map of nodes traversed during this dialogue instance.
	 */
	TArray<FDialogueTraversePath> GetTraversedPath() const
	{ 
		return TraversedPath; 
	};
	
	virtual void SetDialogueContext(UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes);
	virtual void UpdateActiveDialogueNode(UMounteaDialogueGraphNode* NewActiveNode);
	virtual void UpdateAllowedChildrenNodes(const TArray<UMounteaDialogueGraphNode*>& NewNodes);
	virtual void UpdateActiveDialogueTable(const FDataTableRowHandle& NewHandle);
	virtual void UpdateActiveDialogueRow(const FDialogueRow& NewActiveRow);
	virtual void UpdateActiveDialogueRowDataIndex(int32 NewIndex);
	virtual void SetActiveDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant);
	virtual void AddTraversedNode(const UMounteaDialogueGraphNode* TraversedNode);

	virtual bool AddDialogueParticipants(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants);
	virtual bool AddDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant);
	virtual bool RemoveDialogueParticipants(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants);
	virtual bool RemoveDialogueParticipant(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant);
	virtual void ClearDialogueParticipants();
		
	/**
	 * Sets the dialogue context.
	 *
	 * @param NewParticipant The new dialogue participant.
	 * @param NewActiveNode The new active dialogue node.
	 * @param NewAllowedChildNodes The new allowed child dialogue nodes.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="SetDialogueContext"), meta=(CustomTag="MounteaK2Setter"))
	virtual void SetDialogueContextBP(UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes);

	/**
	 * Updates Active Dialogue Node in Context.
	 * 
	 * @param NewActiveNode - New Active Dialogue Node to update to.
	 * ❗ Must not be Null
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="UpdateActiveDialogueNode"), meta=(CustomTag="MounteaK2Setter"))
	virtual void UpdateActiveDialogueNodeBP(UMounteaDialogueGraphNode* NewActiveNode);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="UpdateActiveDialogueRowDataIndex"), meta=(CustomTag="MounteaK2Setter"))
	virtual void UpdateActiveDialogueRowDataIndexBP(int32 NewIndex);
	
	/**
	 * Updates the active dialogue row.
	 * 
	 * @param NewActiveRow - The new active dialogue row.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="UpdateActiveDialogueRow"), meta=(CustomTag="MounteaK2Setter"))
	virtual void UpdateActiveDialogueRowBP(const FDialogueRow& NewActiveRow);
	
	/**
	 * Updates the active dialogue Table.
	 * 
	 * @param NewTable - New table to be used
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="UpdateActiveDialogueRowDataIndex"), meta=(CustomTag="MounteaK2Setter"))
	virtual void UpdateActiveDialogueTableBP(const FDataTableRowHandle& NewTable);
	
	/**
	 * Updates Dialogue Active Participant.
	 *
	 * @param NewParticipant - new Dialogue Active Participant.
	 * ❗ Must implement IMounteaDialogueParticipantInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="UpdateActiveDialogueParticipant"), meta=(CustomTag="MounteaK2Setter"))
	void UpdateActiveDialogueParticipantBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="AddDialogueParticipant"), meta=(CustomTag="MounteaK2Setter"))
	virtual bool AddDialogueParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant);
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="RemoveDialogueParticipant"), meta=(CustomTag="MounteaK2Setter"))
	virtual bool RemoveDialogueParticipantBP(const TScriptInterface<IMounteaDialogueParticipantInterface>& NewParticipant);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="AddDialogueParticipants"), meta=(CustomTag="MounteaK2Setter"))
	virtual bool AddDialogueParticipantsBP(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants);
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="RemoveDialogueParticipants"), meta=(CustomTag="MounteaK2Setter"))
	virtual bool RemoveDialogueParticipantsBP(const TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& NewParticipants);
	
	FDialogueContextUpdatedFromBlueprint DialogueContextUpdatedFromBlueprint;

public:

	virtual TArray<FDialogueTraversePath> GetConditionTraversedPath_Implementation() const override
	{ 
		return TraversedPath; 
	};
	virtual TScriptInterface<IMounteaDialogueParticipantInterface> GetConditionActiveParticipant_Implementation() const override
	{ 
		return ActiveDialogueParticipant; 
	};
	virtual FGuid GetConditionSessionGUID_Implementation() const override
	{ 
		return SessionGUID; 
	};
};
