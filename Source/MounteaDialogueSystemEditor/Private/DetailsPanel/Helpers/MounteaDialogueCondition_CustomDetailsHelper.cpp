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

#include "MounteaDialogueCondition_CustomDetailsHelper.h"

#include "Conditions/MounteaDialogueConditionBase.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueCondition_CustomDetailsHelper"

FReply FMounteaDialogueCondition_CustomDetailsHelper::OnDocumentationClicked() const
{
	UMounteaDialogueConditionBase* condition = Cast<UMounteaDialogueConditionBase>(GetObject());

	if (condition)
	{
		FPlatformProcess::LaunchURL(*condition->GetConditionDocumentationLink(), nullptr, nullptr);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FText FMounteaDialogueCondition_CustomDetailsHelper::GetOpenDocumentationText() const
{
	return LOCTEXT("OpenDocumentationText", "Open Documentation page for Dialogue Condition");
}

#undef LOCTEXT_NAMESPACE
