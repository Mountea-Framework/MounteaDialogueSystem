// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "IDetailCustomization.h"
#include "Helpers/MounteaDialogueBase_CustomRowHelper.h"
#include "Nodes/MounteaDialogueGraphNode.h"

class SScrollBox;

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

#pragma region DialoguePreviews
	const FSlateBrush* GetPreviewsBrush() const;
	FSlateColor GetPreviewsBackgroundColor() const;
	FSlateColor GetPreviewsTextColor() const;
	
	void MakePreviewsScrollBox(TArray<FText>& FromTexts);
	void ResetTexts();
#pragma endregion 

#pragma region ReturnNodePreview
	void ResetPreviewingNode();
	
	FSlateColor GetPreviewingNodeBackgroundColor() const;
	FText GetPreviewingNodeTitle() const;
	FReply OnPreviewingNodeDoubleClicked(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	void OnPreviewingNodeMouseEnter(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	void OnPreviewingNodeMouseLeave(const FPointerEvent& PointerEvent);
	void MakePreviewNode();
	void MakeInvalidPreviewNode();
#pragma endregion 
	
	// IDetailCustomization interface
	/** Called when details should be customized */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TSharedPtr<SBox> PreviewNode;
	TSharedPtr<SScrollBox> PreviewRows;
	TSharedPtr<SButton> DocumentationButton;
	TSharedPtr<SImage> DocumentationImage;
	
	FButtonStyle DocumentationButtonStyle;
	IDetailLayoutBuilder* SavedLayoutBuilder = nullptr;

	UMounteaDialogueGraphNode* EditingNode = nullptr;
};

