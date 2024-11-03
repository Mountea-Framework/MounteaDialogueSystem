// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphDataTypes.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "UObject/Object.h"
#include "MounteaDialogueContext.generated.h"

struct FDialogueTraversePath;
class IMounteaDialogueParticipantInterface;
class UMounteaDialogueGraphNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdatedFromBlueprint, UMounteaDialogueContext*, Context);

/**
 * Dialogue Context.
 * 
 * Contains information needed to successfully start Dialogue.
 * Also helps tracking Dialogue Specific data. Is recycled for whole Dialogue Graph.
 * 
 * In Dialogue Manager Component is used as Transient object, which is nullified once Dialogue ends and is never saved.
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueContext : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Active Dialogue Participant Interface reference.
	 * 
	 * This is the Participant who is Active right now.
	 * ❔ Lead Node sets this value to Dialogue Participant.
	 * ❔ Answer Node sets this value to Player Participant.
	 * ❗ Might be invalid
	 */
	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TScriptInterface<IMounteaDialogueParticipantInterface> ActiveDialogueParticipant;
	
	/**
	 * Player Dialogue Participant Interface reference.
	 * 
	 * This is the Participant who represent the Player.
	 * ❗ Might be invalid
	 */
	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TScriptInterface<IMounteaDialogueParticipantInterface> PlayerDialogueParticipant;

	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant;

	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> DialogueParticipants;
	
	/**
	 * Pointer to the Node which is currently active.
	 * ❗ Might be null
	 */
	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TObjectPtr<UMounteaDialogueGraphNode> ActiveNode = nullptr;

	/**
	 * Keeps info about the Previous Active Node.
	 */
	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FGuid PreviousActiveNode = FGuid::NewGuid();
	
	/**
	 * List of Nodes that can be accessed from Active Node.
	 * Already filtered to contain only those that can be triggered.
	 * 
	 * ❔ Filter is done by 'CanStartNode', which can have its own logic and can be driven by Decorators as well.
	 * ❗ Might be empty
	 */
	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<TObjectPtr<UMounteaDialogueGraphNode>> AllowedChildNodes;

	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FDataTableRowHandle ActiveDialogueTableHandle;
	
	/**
	 * Active Dialogue Row from Active Node. 
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	FDialogueRow ActiveDialogueRow;
	
	/**
	 * Index of currently used Dialogue Row Data row.
	 */
	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	int32 ActiveDialogueRowDataIndex = 0;
	
	/**
	 * Contains mapped list of Traversed Nodes by GUIDs.
	 * Each time Dialogue is updated, like node is selected or starts itself, this Path is updated.
	 * Updates Participant once Dialogue is done.
	 */
	UPROPERTY(/*Replicated, */VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue", meta=(NoResetToDefault))
	TArray<FDialogueTraversePath> TraversedPath;

public:

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(CompactNodeTitle="To String"), meta=(CustomTag="MounteaK2Setter"))
	virtual FString ToString() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Context", meta=(CustomTag="MounteaK2Validate"))
	virtual bool IsValid() const;

	TScriptInterface<IMounteaDialogueParticipantInterface> GetActiveDialogueParticipant() const
	{ return ActiveDialogueParticipant; };
	TScriptInterface<IMounteaDialogueParticipantInterface> GetDialoguePlayerParticipant() const
	{ return PlayerDialogueParticipant; };
	TScriptInterface<IMounteaDialogueParticipantInterface> GetDialogueParticipant() const
	{ return DialogueParticipant; };
	TArray<TScriptInterface<IMounteaDialogueParticipantInterface>> GetDialogueParticipants() const
	{ return DialogueParticipants; }
	
	/**
	 * Returns the Active Node object.
	 * ❗ Might be null
	 * 
	 * @return Active Node if any specified
	 */
	UMounteaDialogueGraphNode* GetActiveNode() const
	{ return ActiveNode; };
	
	/**
	 * Returns lsit of Children Nodes from Active Node.
	 * ❗ Might be empty
	 * 
	 * @return List of allowed Children Nodes 
	 */
	TArray<UMounteaDialogueGraphNode*> GetChildrenNodes() const
	{ return AllowedChildNodes; };

	FDataTableRowHandle GetActiveDataTable() const
	{ return ActiveDialogueTableHandle; };
	
	/**
	 * Returns Active Dialogue Row if any.
	 * ❗ Might return invalid
	 * 
	 * @return Active Dialogue Row if any 
	 */
	FDialogueRow GetActiveDialogueRow() const
	{ return ActiveDialogueRow; };
	
	/**
	 *Returns the Active Dialogue Row Data Index.
	 * 
	 * @return Active Row Index 
	 */
	int32 GetActiveDialogueRowDataIndex() const
	{ return ActiveDialogueRowDataIndex; };
	
	/**
	 * Returns the map of nodes traversed during this dialogue instance.
	 * 
	 * @return The map of nodes traversed during this dialogue instance.
	 */
	TArray<FDialogueTraversePath> GetTraversedPath() const
	{ return TraversedPath; };
	
	virtual void SetDialogueContext(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes);
	virtual void UpdateDialogueParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	virtual void UpdateActiveDialogueNode(UMounteaDialogueGraphNode* NewActiveNode);
	virtual void UpdateAllowedChildrenNodes(const TArray<UMounteaDialogueGraphNode*>& NewNodes);
	virtual void UpdateActiveDialogueTable(const FDataTableRowHandle& NewHandle);
	virtual void UpdateActiveDialogueRow(const FDialogueRow& NewActiveRow);
	virtual void UpdateActiveDialogueRowDataIndex(int32 NewIndex);
	void UpdateDialoguePlayerParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	void UpdateActiveDialogueParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	void AddTraversedNode(const UMounteaDialogueGraphNode* TraversedNode);

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
	virtual void SetDialogueContextBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes);

	/**
	 * Updates Dialogue Participant.
	 *
	 * @param NewParticipant - new Dialogue Participant.
	 * ❗ Must implement IMounteaDialogueParticipantInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="UpdateDialogueParticipant"), meta=(CustomTag="MounteaK2Setter"))
	virtual void UpdateDialogueParticipantBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	
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
	 * Updates Dialogue Player Participant.
	 *
	 * @param NewParticipant - new Dialogue Player Participant.
	 * ❗ Must implement IMounteaDialogueParticipantInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context", meta=(DisplayName="UpdateDialoguePlayerParticipant"), meta=(CustomTag="MounteaK2Setter"))
	void UpdateDialoguePlayerParticipantBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	
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

private:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override {return true;};

public:

	UMounteaDialogueContext* operator += (const UMounteaDialogueContext* Other);
	UMounteaDialogueContext* operator+=(const TObjectPtr<UMounteaDialogueContext>& Other)
	{
		if (!Other) return this;
		return (*this) += Other.Get();
	}
	UMounteaDialogueContext* operator += (const FMounteaDialogueContextReplicatedStruct& Other);
};