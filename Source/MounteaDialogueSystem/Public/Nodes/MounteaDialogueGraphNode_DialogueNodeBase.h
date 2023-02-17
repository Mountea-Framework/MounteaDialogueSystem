// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Engine/DataTable.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_DialogueNodeBase.generated.h"

/**
 * 
 */
UCLASS(Abstract, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories="Mountea", AutoExpandCategories=(DataTableRowHandle, Mountea, Dialogue))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_DialogueNodeBase : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_DialogueNodeBase();

	virtual FText GetDescription_Implementation() const override;

	//TODO: Implement custom Handle with this logic: UPROPERTY(meta=(GetOptions ="abc")) https://benui.ca/unreal/uproperty/#getoptions
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual FDataTableRowHandle GetDialogueGraphHandle() const;

protected:

	UPROPERTY(EditDefaultsOnly, Category="Mountea|Dialogue", meta=(RowType="DialogueRow", ShowOnlyInnerProperties))
	FDataTableRowHandle DialogueRowHandle;

	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<TSubclassOf<UMounteaDialogueGraphNode>> AllowedInputClasses;

	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue")
	FGuid NodeGUID;

#if WITH_EDITOR
	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat) override;
	virtual bool CanCreateConnection(UMounteaDialogueGraphNode* Other, EEdGraphPinDirection Direction, FText& ErrorMessage) override;
#endif
	
};
