// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "Helpers/FMounteaDialogueK2NodesHelpers.h"
#include "K2Node_MounteaDialogueCallFunction.generated.h"

/*
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMDEVELOPER_API UK2Node_MounteaDialogueCallFunction : public UK2Node_CallFunction
{
	GENERATED_BODY()
	
public:
	
	EFunctionCallType GetFunctionType() const;
	EFunctionRole GetFunctionRole() const;

	void Initialize(const UFunction* relevantFunction, UClass* relevantClass);
	
	// UK2Node
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& actionRegistrar) const override;

	// UK2Node_CallFunction
	virtual FText GetToolTipHeading() const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FName GetCornerIcon() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& outColor) const override;

};
