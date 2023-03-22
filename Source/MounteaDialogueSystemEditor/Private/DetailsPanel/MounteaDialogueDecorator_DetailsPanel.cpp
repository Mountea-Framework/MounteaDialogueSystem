// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueDecorator_DetailsPanel.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"

#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "DetailsPanel/Helpers/MounteaDialogueDecorator_CustomDetailsHelper.h"

#define LOCTEXT_NAMESPACE "FMounteaDialogueDecorator_DetailsPanel"

void FMounteaDialogueDecorator_DetailsPanel::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;
	PropertyUtils = StructCustomizationUtils.GetPropertyUtilities();
}

void FMounteaDialogueDecorator_DetailsPanel::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Make Custom elements
	{
		DecoratorPropertyRow = &StructBuilder.AddProperty
		(
			StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMounteaDialogueDecorator, DecoratorType)).ToSharedRef()
		);
		DecoratorPropertyRow->Visibility(EVisibility::Visible);

		// Add Custom buttons
		MounteaDialogueDecorator_CustomDetailsHelper = MakeShared<FMounteaDialogueDecorator_CustomDetailsHelper>(DecoratorPropertyRow);
		MounteaDialogueDecorator_CustomDetailsHelper->SetPropertyUtils(PropertyUtils);
		MounteaDialogueDecorator_CustomDetailsHelper->Update();
		MounteaDialogueDecorator_CustomDetailsHelper->SetFunctionNameToOpen
		(
			EMounteaDialogueBlueprintOpenType::Function,
			GET_FUNCTION_NAME_CHECKED(UMounteaDialogueDecoratorBase, EvaluateDecorator)
		);
	}
}

#undef  LOCTEXT_NAMESPACE