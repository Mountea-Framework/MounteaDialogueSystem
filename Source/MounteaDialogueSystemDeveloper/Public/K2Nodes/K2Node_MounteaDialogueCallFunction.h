// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "Helpers/FMounteaDialogueK2NodesHelpers.h"
#include "K2Node_MounteaDialogueCallFunction.generated.h"


/**
 * This node allows for styling the functions from Mountea Dialogue System. It works by creating a node for each
 * function in the library (these are set to BlueprintInternalUseOnly) and then customizing the node to display the
 * desired information and styling.
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMDEVELOPER_API UK2Node_MounteaDialogueCallFunction : public UK2Node_CallFunction
{
	GENERATED_BODY()
	EFunctionCallType GetFunctionType() const;
	EFunctionRole GetFunctionRole() const;

	/**
	 * Initializes the node with the given function. This is called by the BlueprintNodeSpawner, and is used
	 * to initialize each node with the correct function pointer.
	 */
	void Initialize(const UFunction* func, UClass* cls);
	
	// UK2Node
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

public:

	// UK2Node_CallFunction
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FName GetCornerIcon() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& outColor) const override;

};
