#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SEdNode_MounteaDialogueGraphNodeIndex : public SCompoundWidget
{
	typedef SEdNode_MounteaDialogueGraphNodeIndex Self;
	
public:

public:
	/** Delegate event fired when the hover state of this widget changes */
	DECLARE_DELEGATE_OneParam(FOnHoverStateChanged, bool /* bHovered */);

	/** Delegate used to receive the background color of the node, depending on hover state and state of other siblings */
	DECLARE_DELEGATE_RetVal_OneParam(FSlateColor, FOnGetBackgroundColor, bool /* bHovered */);

	SLATE_BEGIN_ARGS(SEdNode_MounteaDialogueGraphNodeIndex) {}
	SLATE_ATTRIBUTE(TSharedPtr<SWidget>, OverlayBody)

	// Events
	SLATE_EVENT(FOnHoverStateChanged, OnHoverStateChanged)
	SLATE_EVENT(FOnGetBackgroundColor, OnGetBackgroundColor)
	SLATE_END_ARGS()

void Construct(const FArguments& InArgs);

	void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	}

	void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		SCompoundWidget::OnMouseLeave(MouseEvent);
	}

	/** Get the color we use to display the rounded border */
	FSlateColor GetBackgroundColor() const
	{
		return FSlateColor::UseForeground();
	}

private:

	/** The OverlayBody used for this widget*/
	TSharedPtr<SWidget> OverlayBody;
	
};
