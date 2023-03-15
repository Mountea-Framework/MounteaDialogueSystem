// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphNode_Details.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Helpers/MounteaDialogueGraphColors.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "FMounteaDialogueGraphNode_Details"

FReply FMounteaDialogueGraphNode_Details::OnDocumentationClicked() const
{
	if (EditingNode)
	{
		FPlatformProcess::LaunchURL(*EditingNode->GetNodeDocumentationLink(), nullptr, nullptr);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

const FSlateBrush* FMounteaDialogueGraphNode_Details::GetDocumentationBrush() const
{
	return FMounteaDialogueGraphEditorStyle::GetBrush( "MDSStyleSet.Buttons.Documentation" );
}

FSlateColor FMounteaDialogueGraphNode_Details::GetDocumentationColorOpacity() const
{
	return FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
}

const FSlateBrush* FMounteaDialogueGraphNode_Details::GetBorderImage() const
{
	return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
}

void FMounteaDialogueGraphNode_Details::OnDocumentationHovered()
{
	if (!DocumentationButton.IsValid()) return;
	if (!DocumentationImage.IsValid()) return;

	FSlateRenderTransform Unhovered;

	FSlateRenderTransform Hovered = FSlateRenderTransform
	(
		.99f, FVector2D(0.f, 0.f)
	);
	
	DocumentationImage->SetRenderTransform( DocumentationButton->IsHovered() ? Hovered : Unhovered );
}

const FSlateBrush* FMounteaDialogueGraphNode_Details::GetPreviewsBrush() const
{
	return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
}

FSlateColor FMounteaDialogueGraphNode_Details::GetPreviewsBackgroundColor() const
{
	return MounteaDialogueGraphColors::Previews::Normal;
}

FSlateColor FMounteaDialogueGraphNode_Details::GetPreviewsTextColor() const
{
	return MounteaDialogueGraphColors::TextColors::Normal;
}

void FMounteaDialogueGraphNode_Details::MakePreviewsScrollBox(TArray<FText>& FromTexts)
{
	PreviewRows->ClearChildren();

	if (FromTexts.Num() == 0)
	{
		PreviewRows->AddSlot()
		[
			SNew(SBorder)
			.Padding(FMargin(1.f))
			.BorderImage(this, &FMounteaDialogueGraphNode_Details::GetPreviewsBrush)
			.BorderBackgroundColor(MounteaDialogueGraphColors::Previews::Invalid)
			[
				SNew(SBox)
				.Padding(FMargin(2.5f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FMounteaDialogueGraphNode_Details_Previews_Invalid", "invalid data selected"))
					.TextStyle(FEditorStyle::Get(), "Profiler.TooltipBold")
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
					.ColorAndOpacity(this, &FMounteaDialogueGraphNode_Details::GetPreviewsTextColor)
				]
			]
		];

		return;
	}
	for (auto Itr : FromTexts)
	{
		PreviewRows->AddSlot()
		[
			SNew(SBox)
			.Padding(FMargin(4.f))
			[
				SNew(SBorder)
				.Padding(FMargin(2.f))
				.BorderImage(this, &FMounteaDialogueGraphNode_Details::GetPreviewsBrush)
				.BorderBackgroundColor(this, &FMounteaDialogueGraphNode_Details::GetPreviewsBackgroundColor)
				[
					SNew(SBox)
					.Padding(FMargin(2.5f))
					[
						SNew(STextBlock)
						.Text(Itr)
						.TextStyle(FEditorStyle::Get(), "NormalText")
						.AutoWrapText(true)
						.Justification(ETextJustify::Left)
						.AutoWrapText(true)
						.ColorAndOpacity(this, &FMounteaDialogueGraphNode_Details::GetPreviewsTextColor)
					]
				]
			]
		];
	}
}

void FMounteaDialogueGraphNode_Details::ResetTexts()
{
	if (!EditingNode) return;
	if (UMounteaDialogueGraphNode_DialogueNodeBase* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(EditingNode) )
	{
		TArray<FText> PreviewTexts = EditingDialogueNode->GetPreviews();

		MakePreviewsScrollBox(PreviewTexts);
	}
}

void FMounteaDialogueGraphNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// Only support one object being customized
	if (ObjectsBeingCustomized.Num() != 1) return;

	const TWeakObjectPtr<UMounteaDialogueGraphNode> WeakGraphNode = Cast<UMounteaDialogueGraphNode>(ObjectsBeingCustomized[0].Get());
	if (!WeakGraphNode.IsValid()) return;

	EditingNode = WeakGraphNode.Get();
	if (!EditingNode) return;

	// Only edit if editing Graph Editor
	if (DetailBuilder.GetBaseClass()->IsChildOf(UEdNode_MounteaDialogueGraphNode::StaticClass()) == false)
	{ return; };
	
	SavedLayoutBuilder = &DetailBuilder;

	DocumentationButtonStyle = FMounteaDialogueGraphEditorStyle::GetWidgetStyle<FButtonStyle>(TEXT("MDSStyleSet.Buttons.Style"));

	IDetailCategoryBuilder& ItrCategoryBuild = DetailBuilder.EditCategory(TEXT("Documentation"), FText::GetEmpty(), ECategoryPriority::Important);
	ItrCategoryBuild.AddCustomRow(LOCTEXT("MounteaDialogueGraphNode_Details_Documentation", "Node Documentation"), false)
	.WholeRowWidget
	[
		SNew(SBox)
		.HAlign(HAlign_Fill)
		[
			SNew(SScaleBox)
			.HAlign(EHorizontalAlignment::HAlign_Fill)
			.Stretch(EStretch::ScaleToFit)
			[
				SAssignNew(DocumentationButton, SButton)
				.HAlign(HAlign_Fill)
				.Text(LOCTEXT("MounteaDialogueGraphNode_Details_Documentation_Text", "Documentation"))
				.ToolTipText(LOCTEXT("MounteaDialogueGraphNode_Details_Documentation_Tooltip", "Opens a documentation page for selected Node type.\nIf this Node type has no documentation, link to Mountea Dialogue Wiki will opened instead."))
				.OnClicked(this, &FMounteaDialogueGraphNode_Details::OnDocumentationClicked)
				.OnHovered(this, &FMounteaDialogueGraphNode_Details::OnDocumentationHovered)
				.OnUnhovered(this, &FMounteaDialogueGraphNode_Details::OnDocumentationHovered)
				.ButtonStyle(&DocumentationButtonStyle)
				[
					SNew(SScaleBox)
					.HAlign(EHorizontalAlignment::HAlign_Left)
					.Stretch(EStretch::ScaleToFit)
					[
						SAssignNew(DocumentationImage, SImage)
					.Image(this, &FMounteaDialogueGraphNode_Details::GetDocumentationBrush)
					]
				]
			]
		]
	];

	if (UMounteaDialogueGraphNode_DialogueNodeBase* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(EditingNode) )
	{
		EditingDialogueNode->PreviewsUpdated.BindSP(this, &FMounteaDialogueGraphNode_Details::ResetTexts);

		PreviewRows = SNew(SScrollBox).ToolTipText(LOCTEXT("FMounteaDialogueGraphNode_Details_PreviewsTootlip", "Localized preview of selected Node's Dialogue Data."));
		TArray<FText> PreviewTexts = EditingDialogueNode->GetPreviews();
		
		MakePreviewsScrollBox(PreviewTexts);
	
		IDetailCategoryBuilder& PreviewCategoryBuilder = DetailBuilder.EditCategory(TEXT("Previews"), FText::GetEmpty(), ECategoryPriority::Uncommon);
		PreviewCategoryBuilder.AddCustomRow(LOCTEXT("MounteaDialogueGraphNode_Details_Preview", "Node Dialogue Preview"), false)
		.WholeRowWidget
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.MinDesiredWidth(250.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					PreviewRows.ToSharedRef()
				]
			]
		];
		
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_DialogueNodeBase, Preview));
	}
	

	// Hide those categories in Graph Editor
	DetailBuilder.HideCategory("Base");
	DetailBuilder.HideCategory("Private");
	DetailBuilder.HideCategory("Hidden");
	DetailBuilder.HideCategory("Hide");
	DetailBuilder.HideCategory("Editor");
}

#undef LOCTEXT_NAMESPACE