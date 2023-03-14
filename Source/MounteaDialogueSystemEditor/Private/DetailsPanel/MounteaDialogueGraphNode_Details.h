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
	
	FReply OnDocumentationClicked() const;

	const FSlateBrush* GetDocumentationBrush() const;
	FSlateColor GetDocumentationColorOpacity() const;
	const FSlateBrush* GetBorderImage() const;
	void OnDocumentationHovered();
	
	// IDetailCustomization interface
	/** Called when details should be customized */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TSharedPtr<SButton> DocumentationButton;
	TSharedPtr<SImage> DocumentationImage;
	
	FButtonStyle DocumentationButtonStyle;
	IDetailLayoutBuilder* SavedLayoutBuilder = nullptr;
};
