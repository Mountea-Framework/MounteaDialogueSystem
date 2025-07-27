// All rights reserved Dominik Morse 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueGraphNodeInterface.generated.h"

class UMounteaDialogueGraphNode;

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueGraphNodeInterface : public UInterface
{
	GENERATED_BODY()
};

class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueGraphNodeInterface
{
	GENERATED_BODY()
public:
	
	/**
	 * Retrieves an array of allowed input classes for a Dialogue Graph Node.
	 * Allowed Input Classes are defined using two ways:
	 * - By the Node itself, which defines which input classes are allowed to connect to it.
	 * - In the Config File, which defines the default allowed input classes for all nodes of a specific type.
	 *
	 * This function is designed to be overridden in derived classes to specify which input node classes
	 * are permitted to connect to the current node in the dialogue graph structure.
	 *
	 * @return An array of TSubclassOf<UMounteaDialogueGraphNode> representing the allowed input classes.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Node")
	TArray<TSubclassOf<UMounteaDialogueGraphNode>> GetAllowedInputClasses() const;
	virtual TArray<TSubclassOf<UMounteaDialogueGraphNode>> GetAllowedInputClasses_Implementation() const = 0;
};
