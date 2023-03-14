// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphNode_Details.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"

#define LOCTEXT_NAMESPACE ""

FReply FMounteaDialogueGraphNode_Details::OnDocumentationClicked() const
{
	return FReply::Handled();
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

void FMounteaDialogueGraphNode_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// Only support one object being customized
	if (ObjectsBeingCustomized.Num() != 1) return;

	const TWeakObjectPtr<UMounteaDialogueGraphNode> WeakGraphNode = Cast<UMounteaDialogueGraphNode>(ObjectsBeingCustomized[0].Get());
	if (!WeakGraphNode.IsValid()) return;

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
			SAssignNew(DocumentationButton, SButton)
			.HAlign(HAlign_Fill)
			.Text(LOCTEXT("MounteaDialogueGraphNode_Details_Documentation_Text", "Documentation"))
			.ToolTipText(LOCTEXT("MounteaDialogueGraphNode_Details_Documentation_Tooltip", "Opens a documentation page for selected Node type.\nIf this Node type has no documentation, link to Mountea Dialogue Wiki will opened instead."))
			.OnClicked(this, &FMounteaDialogueGraphNode_Details::OnDocumentationClicked)
			.OnHovered(this, &FMounteaDialogueGraphNode_Details::OnDocumentationHovered)
			.OnUnhovered(this, &FMounteaDialogueGraphNode_Details::OnDocumentationHovered)
			.ButtonStyle(&DocumentationButtonStyle)
			[
				SNew(SHorizontalBox)
					
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SAssignNew(DocumentationImage, SImage)
					.Image(this, &FMounteaDialogueGraphNode_Details::GetDocumentationBrush)
				]
			]
		]
	];

	// Hide those categories in Graph Editor
	DetailBuilder.HideCategory("Base");
	DetailBuilder.HideCategory("Private");
	DetailBuilder.HideCategory("Hidden");
	DetailBuilder.HideCategory("Editor");
}

#undef LOCTEXT_NAMESPACE