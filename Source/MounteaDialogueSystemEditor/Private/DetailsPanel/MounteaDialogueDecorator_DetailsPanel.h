// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "IPropertyTypeCustomization.h"
#include "IDetailPropertyRow.h"

class FMounteaDialogueDecorator_CustomDetailsHelper;

class FMounteaDialogueDecorator_DetailsPanel : public IPropertyTypeCustomization
{
	typedef FMounteaDialogueDecorator_DetailsPanel Self;

public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<Self>(); }

	/**
	 * Called when the header of the property (the row in the details panel where the property is shown)
	 * If nothing is added to the row, the header is not displayed
	 *
	 * @param StructPropertyHandle		Handle to the property being customized
	 * @param HeaderRow					A row that widgets can be added to
	 * @param StructCustomizationUtils	Utilities for customization
	 */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	
	/**
	 * Called when the children of the property should be customized or extra rows added
	 *
	 * @param StructPropertyHandle		Handle to the property being customized
	 * @param StructBuilder				A builder for adding children
	 * @param StructCustomizationUtils	Utilities for customization
	 */
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	
	TSharedPtr<IPropertyUtilities> PropertyUtils;
	
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	IDetailPropertyRow* DecoratorPropertyRow = nullptr;
	TSharedPtr<FMounteaDialogueDecorator_CustomDetailsHelper> MounteaDialogueDecorator_CustomDetailsHelper;
};
