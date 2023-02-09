// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode.h"
#include "Engine/DataTable.h"
#include "MounteaDialogueGraphNode_DialogueNodeBase.generated.h"

/**
 * 
 */
UCLASS(Abstract, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories="Mountea", AutoExpandCategories="DataTableRowHandle")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_DialogueNodeBase : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_DialogueNodeBase();

	virtual FText GetDescription_Implementation() const override;

protected:

	UPROPERTY(EditDefaultsOnly, Category="Mountea|Dialogue", meta=(RowType="DialogueRow"))
	FDataTableRowHandle DialogueRowHandle;
	
	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue")
	FGuid NodeGUID;

#if WITH_EDITORONLY_DATA
	
	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<TSubclassOf<UMounteaDialogueGraphNode>> AllowedInputClasses;

#endif
	

protected:

#if WITH_EDITOR
	virtual bool CanCreateConnection(UMounteaDialogueGraphNode* Other, EEdGraphPinDirection Direction, FText& ErrorMessage) override;
#endif
	
};
