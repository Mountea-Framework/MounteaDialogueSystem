// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "Conditions/MounteaDialogueConditionBase.h"
#include "MounteaDialogueCondition_OnlyFirstTime.generated.h"

/**
 * Edge condition that passes only when the target node has not been traversed yet.
 *
 * This condition is intended as the edge-condition replacement for legacy
 * "Only First Time" decorator-based traversal stoppers.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew,
	ClassGroup=("Mountea|Dialogue"),
	AutoExpandCategories=("Mountea", "Dialogue"),
	HideCategories=("Private"),
	meta=(DisplayName="Only First Time"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueCondition_OnlyFirstTime : public UMounteaDialogueConditionBase
{
	GENERATED_BODY()

public:

	UMounteaDialogueCondition_OnlyFirstTime();

	virtual bool EvaluateCondition_Implementation(const TScriptInterface<IMounteaDialogueConditionContextInterface>& Context) const override;

	virtual FString GetConditionDocumentationLink_Implementation() const override
	{
		return TEXT("https://mountea.tools/docs/DialogueSystem/DialogueConditions/OnlyFirstTime");
	}
};
