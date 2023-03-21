// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

enum class EDecoratorsInfoStyle : uint8;
class UEdNode_MounteaDialogueGraphNode;

class SEdNode_MounteaDialogueGraphNode : public SGraphNode
{
	
public:
	SLATE_BEGIN_ARGS(SEdNode_MounteaDialogueGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphNode* InNode);

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	const FSlateBrush* GetIndexBrush() const;
	
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual bool IsNameReadOnly() const override;

	void OnNameTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);
	
	virtual const FSlateBrush* GetNodeTypeBrush () const;
	virtual const FSlateBrush* GetTextNodeTypeBrush () const;
	virtual FSlateColor GetBorderBackgroundColor() const;
	virtual FSlateColor GetBorderFrontColor() const;
	virtual FSlateColor GetNodeTitleBackgroundColor() const;
	virtual FSlateColor GetDecoratorsBackgroundColor() const;

	virtual FSlateColor GetPinsDockColor() const;
	
	virtual EVisibility GetDragOverMarkerVisibility() const;

	virtual const FSlateBrush* GetNameIcon() const;

	virtual const FSlateBrush* GetInheritsImageBrush() const;
	virtual FSlateColor GetInheritsImageTint() const;

	const FSlateBrush* GetBulletPointImageBrush() const;

	virtual FText GetIndexOverlayTooltipText() const;
	virtual FText GetIndexText() const;
	EVisibility GetIndexSlotVisibility() const;
	FVector2D GetIndexSlotOffset() const;
	FVector2D GetIndexSlotSize() const;
	
	virtual void OnIndexHoverStateChanged(bool bArg) const;
	virtual FSlateColor GetOverlayWidgetBackgroundColor(bool bArg) const;

	bool HasGraphDecorators() const;
	bool HasNodeDecorators() const;
	
	virtual FText GetDecoratorsText() const;
	virtual FText GetNumberOfDecorators() const;
	virtual FText GetDecoratorsInheritanceText() const;
	
	EVisibility ShowImplementsOnlySlot_Unified() const;
	EVisibility ShowInheritsDecoratorsSlot_Unified() const;
	EVisibility ShowImplementsOnlySlot_Stack() const;
	EVisibility ShowInheritsDecoratorsSlot_Stack() const;
	EVisibility ShowAllDecorators() const;
	EVisibility ShowDecoratorsBottomPadding() const;

	FSlateColor GetImplementsRowColor() const;
	FSlateColor GetBulletPointsImagePointColor() const;
	
	virtual EDecoratorsInfoStyle GetDecoratorsStyle() const;
	EVisibility GetStackVisibility() const;
	EVisibility GetUnifiedVisibility() const;

	FText GetTooltipText() const;

protected:
	TSharedPtr<SBorder> NodeBody;
	TSharedPtr<SHorizontalBox> OutputPinBox;

	class UMounteaDialogueGraphEditorSettings* GraphEditorSettings = nullptr;

	FLinearColor NodeInnerColor;
	FLinearColor PinsDockColor;
};
