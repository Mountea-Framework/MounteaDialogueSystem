// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphDataTypes.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "UObject/Object.h"
#include "MounteaDialogueContext.generated.h"

class IMounteaDialogueParticipantInterface;
class UMounteaDialogueGraphNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdatedFromBlueprint, UMounteaDialogueContext*, Context);

/**
 * Dialogue Context.
 * 
 * Contains information needed to successfully start Dialogue.
 * Also helps tracking Dialogue Specific data. Is recycled for whole Dialogue Graph.
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueContext : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TScriptInterface<IMounteaDialogueParticipantInterface> ActiveDialogueParticipant;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TScriptInterface<IMounteaDialogueParticipantInterface> PlayerDialogueParticipant;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TScriptInterface<IMounteaDialogueParticipantInterface> DialogueParticipant;
	/**
	 * Pointer to the Node which is currently active.
	 * ❗Might be null❗
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	UMounteaDialogueGraphNode* ActiveNode = nullptr;
	/**
	 * List of Nodes that can be accessed from Active Node.
	 * Already filtered to contain only those that can be triggered.
	 * 
	 * ❗Might be empty❗ 
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> AllowedChildNodes;
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
	TMap<FGuid, int32> TraversedPath;

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Debug")
	virtual bool IsValid() const;

	TScriptInterface<IMounteaDialogueParticipantInterface> GetActiveDialogueParticipant() const
	{ return PlayerDialogueParticipant; };
	TScriptInterface<IMounteaDialogueParticipantInterface> GetDialoguePlayerParticipant() const
	{ return PlayerDialogueParticipant; };
	TScriptInterface<IMounteaDialogueParticipantInterface> GetDialogueParticipant() const
	{ return DialogueParticipant; };
	/**
	 * Returns the Active Node object.
	 * ❗Might be null❗
	 * 
	 * @return Active Node if any specified
	 */
	UMounteaDialogueGraphNode* GetActiveNode() const
	{ return ActiveNode; };
	/**
	 * Returns lsit of Children Nodes from Active Node.
	 * ❗Might be empty❗
	 * 
	 * @return List of allowed Children Nodes 
	 */
	TArray<UMounteaDialogueGraphNode*> GetChildrenNodes() const
	{ return AllowedChildNodes; };
	/**
	 * Returns Active Dialogue Row if any.
	 * ❗Might return invalid❗ 
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
	TMap<FGuid, int32> GetTraversedPath() const
	{ return TraversedPath; };
	
	virtual void SetDialogueContext(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes);
	virtual void UpdateDialogueParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	virtual void UpdateActiveDialogueNode(UMounteaDialogueGraphNode* NewActiveNode);
	virtual void UpdateAllowedChildrenNodes(const TArray<UMounteaDialogueGraphNode*>& NewNodes);
	virtual void UpdateActiveDialogueRow(const FDialogueRow& NewActiveRow);
	virtual void UpdateActiveDialogueRowDataIndex(int32 NewIndex);
	void UpdateDialoguePlayerParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	void UpdateActiveDialogueParticipant(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	void AddTraversedNode(const UMounteaDialogueGraphNode* TraversedNode);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context")
	virtual void SetDialogueContextBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant, UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes);
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context")
	virtual void UpdateDialogueParticipantBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context")
	virtual void UpdateActiveDialogueNodeBP(UMounteaDialogueGraphNode* NewActiveNode);
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context")
	virtual void UpdateActiveDialogueRowBP(const FDialogueRow& NewActiveRow);
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context")
	virtual void UpdateActiveDialogueRowDataIndexBP(int32 NewIndex);
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context")
	void UpdateDialoguePlayerParticipantBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Context")
	void UpdateActiveDialogueParticipantBP(TScriptInterface<IMounteaDialogueParticipantInterface> NewParticipant);

	FDialogueContextUpdatedFromBlueprint DialogueContextUpdatedFromBlueprint;
};
