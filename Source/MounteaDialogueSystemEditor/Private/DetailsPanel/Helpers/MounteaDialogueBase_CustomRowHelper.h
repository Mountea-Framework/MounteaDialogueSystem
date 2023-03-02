// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "DetailWidgetRow.h"

class MOUNTEADIALOGUESYSTEMEDITOR_API FMounteaDialogueBase_CustomRowHelper
{
	typedef FMounteaDialogueBase_CustomRowHelper Self;
	
public:
	FMounteaDialogueBase_CustomRowHelper(FDetailWidgetRow* InDetailWidgetRow, const TSharedPtr<IPropertyHandle>& InPropertyHandle);
	virtual ~FMounteaDialogueBase_CustomRowHelper() {}

	// Sets the localized display name of the property.
	Self& SetDisplayName(const FText& InDisplayName)
	{
		DisplayName = InDisplayName;
		return *this;
	}

	// Sets the localized tooltip of the property.
	Self& SetToolTip(const FText& InToolTip)
	{
		ToolTip = InToolTip;
		return *this;
	}

	// Sets the visibility of this property.
	Self& SetVisibility(const TAttribute<EVisibility>& InVisibility)
	{
		DetailWidgetRow->Visibility(InVisibility);
		return *this;
	}

	// Set the optional utils
	Self& SetPropertyUtils(const TSharedPtr<IPropertyUtilities>& Utils)
	{
		PropertyUtils = Utils;
		return *this;
	}

	// Update the full property row.
	void Update();

protected:
	// Internal update method that must be implemented. Called after Update finishes.
	virtual void UpdateInternal() = 0;

protected:
	// The custom widget row this represents
	FDetailWidgetRow* DetailWidgetRow = nullptr;

	// Helpers class
	TSharedPtr<IPropertyUtilities> PropertyUtils;

	// The Property handle of what this row represents
	TSharedPtr<IPropertyHandle> PropertyHandle;

	// The NameContent Widget.
	TSharedPtr<SWidget> NameContentWidget;

	// Texts used for this property row.
	FText DisplayName;
	FText ToolTip;	
};
