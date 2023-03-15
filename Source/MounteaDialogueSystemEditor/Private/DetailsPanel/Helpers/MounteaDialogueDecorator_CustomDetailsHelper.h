// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "IDetailPropertyRow.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"

// Custom row for Objects that most likely are Blueprints?
// This only works with the IDetailPropertyRow::CustomWidget
// If we don't use this the children of EditInlineNew won't be displayed
// Custom row for things that can Handle Objects that are most likely Blueprints or Native Classes
// And this helps us to open them in the Blueprint Editor or open the Native Class inside the IDE
class FMounteaDialogueDecorator_CustomDetailsHelper : public TSharedFromThis<FMounteaDialogueDecorator_CustomDetailsHelper>
{

	typedef FMounteaDialogueDecorator_CustomDetailsHelper Self;
	
public:
	
	FMounteaDialogueDecorator_CustomDetailsHelper(IDetailPropertyRow* InPropertyRow);
	virtual ~FMounteaDialogueDecorator_CustomDetailsHelper() {}
	// Update the full property row.
	void Update();

	// Mutually exclusive with
	Self& SetFunctionNameToOpen(EMounteaDialogueBlueprintOpenType InOpenType, FName Name)
	{
		OpenType = InOpenType;
		FunctionNameToOpen = Name;
		return *this;
	}

	Self& SetPropertyUtils(const TSharedPtr<IPropertyUtilities>& Utils)
	{
		PropertyUtils = Utils;
		return *this;
	}

protected:
	
	// Reset to default
	virtual FReply OnBrowseClicked();
	virtual FReply OnOpenClicked();
	FReply OnDocumentationClicked() const;

	virtual UObject* GetObject() const;
	UBlueprint* GetBlueprint() const;
	bool IsObjectABlueprint() const;

	virtual FText GetBrowseObjectText() const;
	virtual FText GetJumpToObjectText() const;
	virtual FText GetOpenDocumentationText() const;
	virtual float GetRowMinimumDesiredWidth() const { return 250.f; }

	EVisibility GetOpenButtonVisibility() const;
	EVisibility GetBrowseButtonVisibility() const;
	EVisibility GetOpenDocumentationButtonVisibility() const;
	virtual bool CanBeVisible() const { return true; }

	void RequestDeleteItem();
	void OnDeleteItem();

protected:

	TSharedPtr<IPropertyUtilities> PropertyUtils;
	
	// The Property handle of what this row represents
	IDetailPropertyRow* PropertyRow = nullptr;

	// Blueprint Editor
	bool bForceFullEditor = true;
	FName FunctionNameToOpen = NAME_None;
	bool bAddBlueprintFunctionIfItDoesNotExist = true;
	EMounteaDialogueBlueprintOpenType OpenType = EMounteaDialogueBlueprintOpenType::None;
};
