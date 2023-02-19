// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MounteaDialogueContext.generated.h"

class UMounteaDialogueGraphNode;

/**
 * Dialogue Context.
 * Contains information needed to successfully start Dialogue.
 * Also helps tracking Dialogue Specific data. Is recycled for whole Dialogue Graph.
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueContext : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	UMounteaDialogueGraphNode* ActiveNode = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mountea|Dialogue")
	TArray<UMounteaDialogueGraphNode*> AllowedChildNodes;

public:

	virtual bool IsValid() const;
	
	virtual void SetDialogueContext(UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes);
};
