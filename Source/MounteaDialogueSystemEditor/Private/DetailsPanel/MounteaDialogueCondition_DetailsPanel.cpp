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

#include "MounteaDialogueCondition_DetailsPanel.h"

#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"

#include "Conditions/MounteaDialogueConditionBase.h"
#include "DetailsPanel/Helpers/MounteaDialogueCondition_CustomDetailsHelper.h"

#define LOCTEXT_NAMESPACE "FMounteaDialogueCondition_DetailsPanel"

void FMounteaDialogueCondition_DetailsPanel::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;
	PropertyUtils = StructCustomizationUtils.GetPropertyUtilities();
}

void FMounteaDialogueCondition_DetailsPanel::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// ConditionClass — custom widget with delete/browse/open/docs buttons
	{
		ConditionPropertyRow = &StructBuilder.AddProperty(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMounteaDialogueCondition, ConditionClass)).ToSharedRef()
		);
		ConditionPropertyRow->Visibility(EVisibility::Visible);

		ConditionCustomDetailsHelper = MakeShared<FMounteaDialogueCondition_CustomDetailsHelper>(ConditionPropertyRow);
		ConditionCustomDetailsHelper->SetPropertyUtils(PropertyUtils);
		ConditionCustomDetailsHelper->Update();
		ConditionCustomDetailsHelper->SetFunctionNameToOpen(
			EMounteaDialogueBlueprintOpenType::Function,
			GET_FUNCTION_NAME_CHECKED(UMounteaDialogueConditionBase, EvaluateCondition)
		);
	}

	// bNegate — standard property row
	StructBuilder.AddProperty(
		StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMounteaDialogueCondition, bNegate)).ToSharedRef()
	);
}

#undef LOCTEXT_NAMESPACE
