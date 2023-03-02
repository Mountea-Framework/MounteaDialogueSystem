// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "IDetailCustomization.h"
#include "IDetailPropertyRow.h"
#include "Helpers/MounteaDialogueBase_CustomRowHelper.h"
#include "Nodes/MounteaDialogueGraphNode.h"

class FMounteaDialogueGraphNode_Details : public IDetailCustomization
{
	typedef FMounteaDialogueGraphNode_Details Self;

public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<Self>(); }

	// IDetailCustomization interface
	/** Called when details should be customized */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Hold the reference to the Graph Node this represents */
	UMounteaDialogueGraphNode* GraphNode = nullptr;
	
	// Property Handles
	TSharedPtr<IPropertyHandle> TextPropertyHandle;

	// Property rows
	IDetailPropertyRow* NodeDataPropertyRow = nullptr;
	TSharedPtr<FMounteaDialogueBase_CustomRowHelper> NodeDataPropertyRow_CustomDisplay;
	IDetailPropertyRow* GenericDataPropertyRow = nullptr;
	IDetailPropertyRow* ChildrenPropertyRow = nullptr;

	/** The details panel layout builder reference. */
	IDetailLayoutBuilder* DetailLayoutBuilder = nullptr;
};
