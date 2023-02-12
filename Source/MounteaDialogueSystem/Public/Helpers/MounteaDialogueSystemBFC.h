// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Nodes/MounteaDialogueGraphNode.h"
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
	
};
