// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "MounteaDialogueSystemBFC.generated.h"

/**
 * Helper functions library for Mountea Dialogue System.
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueSystemBFC : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Tries to get Dialogue Node  from Children Nodes at given Index. If none is found, returns null.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	static UMounteaDialogueGraphNode* GetChildrenNodeFromIndex(const int32 Index, const UMounteaDialogueGraphNode* ParentNode)
	{
		if (ParentNode->GetChildrenNodes().IsValidIndex(Index))
		{
			return ParentNode->GetChildrenNodes()[Index];
		}

		return nullptr;
	}

	/**
	 * Tries to get first Dialogue Node from Children Nodes. If none is found, returns null.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	static UMounteaDialogueGraphNode* GetFirstChildNode(const UMounteaDialogueGraphNode* ParentNode)
	{
		if (ParentNode->GetChildrenNodes().IsValidIndex(0))
		{
			return ParentNode->GetChildrenNodes()[0];
		}

		return nullptr;
	}

	/**
	 *Returns all Allowed Child Nodes for given Parent.*/ 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	static TArray<UMounteaDialogueGraphNode*> GetAllowedChildNodes(const UMounteaDialogueGraphNode* ParentNode)
	{
		TArray<UMounteaDialogueGraphNode*> ReturnNodes;

		if (!ParentNode) return ReturnNodes;

		if (ParentNode->GetChildrenNodes().Num() == 0) return ReturnNodes;

		for (UMounteaDialogueGraphNode* Itr : ParentNode->GetChildrenNodes())
		{
			if (Itr && Itr->CanStartNode())
			{
				ReturnNodes.Add(Itr);
			}
		}

		return ReturnNodes;
	}

	/**
	 * Returns whether Dialogue Row is valid or not.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	static bool IsDialogueRowValid(const FDialogueRow& Row)
	{
		FGuid InvalidGuid;
		InvalidGuid.Invalidate();
		
		return Row.RowGUID != InvalidGuid || Row.DialogueParticipant.IsEmpty() == false;
	}

	/**
	 * Returns Dialogue Row for Given Node.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	static FDialogueRow GetDialogueRow(UMounteaDialogueGraphNode* Node)
	{
		UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		
		if (!DialogueNodeBase) FDialogueRow();
		if (DialogueNodeBase->GetDataTable() == nullptr) return FDialogueRow();
		if (DialogueNodeBase->GetDataTable()->RowStruct->IsChildOf(FDialogueRow::StaticStruct()) == false) return FDialogueRow();

		const FDialogueRow* Row = DialogueNodeBase->GetDataTable()->FindRow<FDialogueRow>(DialogueNodeBase->GetRowName(), FString("") );
		if (!Row) return FDialogueRow();
		if (IsDialogueRowValid(*Row) == false) return FDialogueRow();

		return *Row;
	};

	/**
	 * Returns Duration for each Dialogue Row.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	static float GetRowDuration(const struct FDialogueRowData& Row)
	{
		float ReturnValue = 1.f;
		switch (Row.RowDurationMode)
		{
			case ERowDurationMode::ERDM_Duration:
				{
					if (Row.RowSound)
					{
						ReturnValue = Row.RowSound->Duration;
						break;
					}
					
					ReturnValue =  Row.RowDuration;
				}
				break;
			case ERowDurationMode::EDRM_Override:
				{
					ReturnValue = Row.RowDurationOverride;
				}
				break;
			case ERowDurationMode::EDRM_Add:
				{
					ReturnValue = Row.RowDuration + Row.RowDurationOverride;
				}
				break;
			case ERowDurationMode::ERDM_AutoCalculate:
				{
					//TODO: Make 8:100 ratio editable in Settings!
					ReturnValue= ((Row.RowText.ToString().Len() * 8.f) / 100.f);
					break;
				}
		}

		ReturnValue = FMath::Max(1.f, ReturnValue);
		
		return ReturnValue;
	}
};
