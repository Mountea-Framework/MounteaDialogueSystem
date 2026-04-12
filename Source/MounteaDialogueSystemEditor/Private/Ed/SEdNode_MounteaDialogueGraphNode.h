// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UEdNode_MounteaDialogueGraphNode;
class UMounteaDialogueGraphEditorSettings;
class SGraphPin;

class SEdNode_MounteaDialogueGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SEdNode_MounteaDialogueGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphNode* InNode);

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty) override;

	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual bool IsNameReadOnly() const override;

	void OnNameTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

	FText GetNodeTitle() const;
	FText GetPreviewText() const;
	FText GetRowsMetaText() const;
	TArray<FText> GetParticipantChipTexts() const;
	TArray<FText> GetDecoratorChipTexts() const;
	FText GetTooltipText() const;
	int32 GetDialogueRowCount() const;

	EVisibility GetDragOverMarkerVisibility() const;
	const FSlateBrush* GetNameIcon() const;
	const FSlateBrush* GetParticipantIcon() const;
	const FSlateBrush* GetDecoratorIcon() const;

	FSlateColor GetHeaderTextColor() const;
	FSlateColor GetHeaderIconColor() const;
	FSlateColor GetSelectionRingColor() const;
	FSlateColor GetCardBorderColor() const;
	FSlateColor GetCardBackgroundColor() const;
	FSlateColor GetParticipantChipBackgroundColor() const;
	FSlateColor GetParticipantChipForegroundColor() const;
	FSlateColor GetDecoratorChipBackgroundColor() const;
	FSlateColor GetDecoratorChipForegroundColor() const;
	FSlateColor GetSeparatorColor() const;

	bool HasParticipantChip() const;
	bool HasDecoratorChips() const;
	bool ShouldUpdate() const;
	bool IsNodeActive() const;

private:
	TSharedRef<SWidget> CreateNameSlotWidget();
	TSharedRef<SWidget> CreateHeaderWidget();
	TSharedRef<SWidget> CreateBodyWidget();
	TSharedRef<SWidget> CreateParticipantChipsWidget();
	TSharedRef<SWidget> CreateDecoratorChipsWidget();
	TSharedRef<SWidget> CreateTagChipsWidget(const TArray<FText>& ChipTexts, const FSlateBrush* ChipIcon, const FSlateColor& ChipForegroundColor, const FSlateColor& ChipBackgroundColor) const;
	TSharedRef<SWidget> CreateTagChip(const FText& ChipText, const FSlateBrush* ChipIcon, const FSlateColor& ChipForegroundColor, const FSlateColor& ChipBackgroundColor) const;
	FLinearColor GetAccentColor() const;
	FVector2D GetNodeSizeHint() const;

private:
	TSharedPtr<SBorder> NodeBody;
	TSharedPtr<SVerticalBox> InputPinBox;
	TSharedPtr<SVerticalBox> OutputPinBox;
	TObjectPtr<UMounteaDialogueGraphEditorSettings> GraphEditorSettings = nullptr;
};
