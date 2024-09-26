// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "MounteaK2Node_StartDialogue.generated.h"

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMDEVELOPER_API UMounteaK2Node_StartDialogue : public UK2Node_CallFunction
{
	GENERATED_BODY()

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface

	// Begin UK2Node interface
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FLinearColor GetNodeBodyTintColor() const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; };
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	// End UK2Node interface

private:
	void SetFunction();

private:
	
	TObjectPtr<UFunction> Function = nullptr;
};
