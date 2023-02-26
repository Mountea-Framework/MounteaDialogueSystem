#include "SEdNode_MounteaDialogueGraphNodeIndex.h"

#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"

void SEdNode_MounteaDialogueGraphNodeIndex::Construct(const FArguments& InArgs)
{
	OnHoverStateChangedEvent = InArgs._OnHoverStateChanged;
	OnGetBackgroundColorEvent = InArgs._OnGetBackgroundColor;
	OverlayBody = InArgs._OverlayBody.Get();

	const FSlateBrush* CircleBrush = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.IndexCircle"));
	ChildSlot
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			// Add a dummy box here to make sure the widget doesnt get smaller than the brush
			SNew(SBox)
			.WidthOverride(CircleBrush->ImageSize.X)
			.HeightOverride(CircleBrush->ImageSize.Y)
		]

		+SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(CircleBrush)
			.BorderBackgroundColor(this, &Self::GetBackgroundColor)
			.Padding(FMargin(4.0f))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				OverlayBody.ToSharedRef()
			]
		]
	];
}
