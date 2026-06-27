// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
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

#include "MounteaDialogueDecorator_CustomDetailsHelper.h"

/**
 * Details helper for FMounteaDialogueCondition.
 * Extends the decorator helper, overriding only the documentation-specific behaviour.
 */
class FMounteaDialogueCondition_CustomDetailsHelper : public FMounteaDialogueDecorator_CustomDetailsHelper
{
public:

	FMounteaDialogueCondition_CustomDetailsHelper(IDetailPropertyRow* InPropertyRow)
		: FMounteaDialogueDecorator_CustomDetailsHelper(InPropertyRow) {}

protected:

	virtual FReply OnDocumentationClicked() const override;
	virtual FText GetOpenDocumentationText() const override;
};
