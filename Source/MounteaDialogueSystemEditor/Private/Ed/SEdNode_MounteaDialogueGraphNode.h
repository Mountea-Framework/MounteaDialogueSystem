// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UEdNode_MounteaDialogueGraphNode;

class SEdNode_MounteaDialogueGraphNode : public SGraphNode
{
	
public:
	SLATE_BEGIN_ARGS(SEdNode_MounteaDialogueGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphNode* InNode);

	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual bool IsNameReadOnly() const override;

	void OnNameTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);
	
	virtual const FSlateBrush* GetNodeTypeBrush () const;
	virtual const FSlateBrush* GetTextNodeTypeBrush () const;
	virtual FSlateColor GetBorderBackgroundColor() const;
	virtual FSlateColor GetBorderFrontColor() const;
	virtual FSlateColor GetBackgroundColor() const;

	virtual FSlateColor GetPinsDockColor() const;
	
	virtual EVisibility GetDragOverMarkerVisibility() const;

	virtual const FSlateBrush* GetNameIcon() const;

protected:
	TSharedPtr<SBorder> NodeBody;
	TSharedPtr<SHorizontalBox> OutputPinBox;

	class UMounteaDialogueGraphEditorSettings* GraphEditorSettings = nullptr;

	FLinearColor NodeInnerColor;
	FLinearColor PinsDockColor;

private:

	// TODO: Example, will be expanded further
	virtual TSharedRef<SVerticalBox>  MakeTextBox(const FText& InText);
};
