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

#include "IPropertyTypeCustomization.h"
#include "IDetailPropertyRow.h"

class FMounteaDialogueCondition_CustomDetailsHelper;

class FMounteaDialogueCondition_DetailsPanel : public IPropertyTypeCustomization
{
	typedef FMounteaDialogueCondition_DetailsPanel Self;

public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<Self>(); }

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:

	TSharedPtr<IPropertyUtilities> PropertyUtils;
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	IDetailPropertyRow* ConditionPropertyRow = nullptr;
	TSharedPtr<FMounteaDialogueCondition_CustomDetailsHelper> ConditionCustomDetailsHelper;
};
