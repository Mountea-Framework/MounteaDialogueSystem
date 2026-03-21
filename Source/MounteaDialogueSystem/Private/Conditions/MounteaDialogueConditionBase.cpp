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

#include "Conditions/MounteaDialogueConditionBase.h"

#include "Helpers/MounteaDialogueSystemConsts.h"

UMounteaDialogueConditionBase::UMounteaDialogueConditionBase()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
		ConditionGUID = FGuid::NewGuid();
}

bool UMounteaDialogueConditionBase::EvaluateCondition_Implementation(UObject* Context) const
{
	return true;
}

FString UMounteaDialogueConditionBase::GetConditionName_Implementation() const
{
	return ConditionName.ToString();
}

FString UMounteaDialogueConditionBase::GetConditionDocumentationLink_Implementation() const
{
	return MounteaDialogueSystemConsts::DialogueDocumentationLink;
}

FGuid UMounteaDialogueConditionBase::GetConditionGUID() const
{
	return ConditionGUID;
}

void UMounteaDialogueConditionBase::SetConditionGUID(const FGuid& NewGUID)
{
	if (ConditionGUID != NewGUID)
		ConditionGUID = NewGUID;
}
