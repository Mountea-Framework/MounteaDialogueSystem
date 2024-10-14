// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphNode_Details.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorCommands/FMounteaDialogueGraphEditorCommands.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Helpers/MounteaDialogueGraphColors.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
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
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
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
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
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
	
	if (SavedLayoutBuilder) SavedLayoutBuilder->ForceRefreshDetails();
	
	if (UMounteaDialogueGraphNode_DialogueNodeBase* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(EditingNode) )
	{
		TArray<FText> PreviewTexts = EditingDialogueNode->GetPreviews();

		MakePreviewsScrollBox(PreviewTexts);
	}
}

void FMounteaDialogueGraphNode_Details::ResetPreviewingNode()
{
	if (!EditingNode) return;
	
	if (SavedLayoutBuilder) SavedLayoutBuilder->ForceRefreshDetails();
	
	if (EditingNode->IsA(UMounteaDialogueGraphNode_ReturnToNode::StaticClass()))
	{
		MakePreviewNode();
	}
}

FText FMounteaDialogueGraphNode_Details::GetPreviewingNodeTitle() const
{
	if (!EditingNode) return FText::FromString("INVALID");

	const UMounteaDialogueGraphNode_ReturnToNode* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);

	if (!EditingDialogueNode) return FText::FromString("INVALID");
	if (!EditingDialogueNode->SelectedNode) return FText::FromString("INVALID");
	
	return EditingDialogueNode->SelectedNode->GetNodeTitle();
}

FReply FMounteaDialogueGraphNode_Details::OnPreviewingNodeDoubleClicked(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	if (!EditingNode) return FReply::Unhandled();

	const UMounteaDialogueGraphNode_ReturnToNode* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	if (!EditingDialogueNode) return FReply::Unhandled();
	if (!EditingDialogueNode->SelectedNode) return FReply::Unhandled();
	
	const auto EdGraph = Cast<UEdGraph_MounteaDialogueGraph>(EditingNode->GetGraph()->EdGraph);

	if (!EdGraph->NodeMap.Contains(EditingDialogueNode->SelectedNode)) return FReply::Unhandled();

	return EdGraph->JumpToNode(EditingDialogueNode->SelectedNode) ? FReply::Handled() : FReply::Unhandled();
}

void FMounteaDialogueGraphNode_Details::OnPreviewingNodeMouseEnter(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	if (!PreviewNode.IsValid()) return;

	FSlateRenderTransform Hovered = FSlateRenderTransform
	(
		.95f, FVector2D(0.f, 0.f)
	);
	
	PreviewNode->SetRenderTransform( Hovered );
	PreviewNode->SetRenderTransformPivot(FVector2D(0.5f));
}

void FMounteaDialogueGraphNode_Details::OnPreviewingNodeMouseLeave(const FPointerEvent& PointerEvent)
{
	FSlateRenderTransform UnHovered = FSlateRenderTransform
	(
		1.0f, FVector2D(0.f, 0.f)
	);
	
	PreviewNode->SetRenderTransform( UnHovered );
	PreviewNode->SetRenderTransformPivot(FVector2D(0.5f));
}

FSlateColor FMounteaDialogueGraphNode_Details::GetPreviewingNodeBackgroundColor() const
{
	if (!EditingNode)  return FLinearColor::Red;

	const UMounteaDialogueGraphNode_ReturnToNode* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	if (!EditingDialogueNode) return FLinearColor::Red;
	if (!EditingDialogueNode->SelectedNode)  return FLinearColor::Red;
	
	return EditingDialogueNode->SelectedNode->GetBackgroundColor();
}

void FMounteaDialogueGraphNode_Details::MakePreviewNode()
{
	if (PreviewNode.IsValid())
	{
		PreviewNode.Reset();
	}
	
	if (!EditingNode)
	{
		MakeInvalidPreviewNode();
		return;
	}

	const UMounteaDialogueGraphNode_ReturnToNode* EditingReturnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	
	if (!EditingReturnNode)
	{
		MakeInvalidPreviewNode();
		return;
	}
	if (!EditingReturnNode->SelectedNode)
	{
		MakeInvalidPreviewNode();
		return;
	}

	const auto EdGraph = Cast<UEdGraph_MounteaDialogueGraph>(EditingNode->GetGraph()->EdGraph);
	if (!EdGraph) return;

	const UMounteaDialogueGraphNode_ReturnToNode* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	if (!EdGraph->NodeMap.Contains(EditingDialogueNode->SelectedNode))
	{
		MakeInvalidPreviewNode();
		return;
	}
	
	const FMargin NodePadding = FMargin(2.0f);
	const FSlateColor DefaultFontColor = MounteaDialogueGraphColors::TextColors::Normal;
	
	PreviewNode =  SNew(SBox)
	.Padding(FMargin(0.f, 5.f, 0.f, 5.f))
	.MinDesiredWidth(FOptionalSize(120.f))
	.MaxDesiredWidth(FOptionalSize(120.f))
	.HAlign(HAlign_Center)
	.MinDesiredHeight(FOptionalSize(75.f))
	.MaxDesiredHeight(FOptionalSize(90.f))
	.VAlign(VAlign_Fill)
	.ToolTipText(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreviewTooltip","This node is currently selected as Return Node.\nUpon execution of Return to Node, Dialogue will move to Selected Node.\n\nDouble click on this Preview to focus on this Node in Graph."))
	[
		// OUTER STYLE
		SNew(SBorder)
		.HAlign(HAlign_Fill)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.SoftEdges"))
		.Padding(3.0f)
		.BorderBackgroundColor(this, &FMounteaDialogueGraphNode_Details::GetPreviewingNodeBackgroundColor)
		.OnMouseDoubleClick(this, &FMounteaDialogueGraphNode_Details::OnPreviewingNodeDoubleClicked)
		[
			SNew(SOverlay)

			// Adding some colours so its not so boring
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				// INNER STYLE
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.SoftEdges"))
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.BorderBackgroundColor(MounteaDialogueGraphColors::Overlay::DarkTheme)
			]
			
			// Pins and node details
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSpacer)
					.Size(FVector2D(0.f, 10.f))
				]

				+ SVerticalBox::Slot()
				.Padding(FMargin(NodePadding.Left, 0.0f, NodePadding.Right, 0.0f))
				.VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges"))
						.BorderBackgroundColor(MounteaDialogueGraphColors::DecoratorsBody::Default)
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.Visibility(EVisibility::SelfHitTestInvisible)
						[
							SNew(SBox)
							.MinDesiredWidth(FOptionalSize(145.f))
							.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
							[
								SNew(STextBlock)
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.Text(this, &FMounteaDialogueGraphNode_Details::GetPreviewingNodeTitle)
								.Justification(ETextJustify::Center)
								.Visibility(EVisibility::Visible)
								.ColorAndOpacity(MounteaDialogueGraphColors::TextColors::Normal)
							]
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSpacer)
					.Size(FVector2D(0.f, 10.f))
				]
			]
		]
	];


	FSimpleNoReplyPointerEventHandler OnPreviewingNodeMouseLeaveEventHandle;
	OnPreviewingNodeMouseLeaveEventHandle.BindSP(this, &FMounteaDialogueGraphNode_Details::OnPreviewingNodeMouseLeave);

	FNoReplyPointerEventHandler OnPreviewingNodeMouseEnterEventHandle;
	OnPreviewingNodeMouseEnterEventHandle.BindSP(this, &FMounteaDialogueGraphNode_Details::OnPreviewingNodeMouseEnter);
	
	PreviewNode->SetOnMouseLeave(OnPreviewingNodeMouseLeaveEventHandle);
	PreviewNode->SetOnMouseEnter(OnPreviewingNodeMouseEnterEventHandle);
}

void FMounteaDialogueGraphNode_Details::MakeInvalidPreviewNode() 
{
	const FMargin NodePadding = FMargin(2.0f);

	const FSlateColor DefaultFontColor = MounteaDialogueGraphColors::TextColors::Normal;

	PreviewNode =  SNew(SBox)
	.Padding(FMargin(0.f, 5.f, 0.f, 5.f))
	.MinDesiredWidth(FOptionalSize(110.f))
	.MaxDesiredWidth(FOptionalSize(120.f))
	.HAlign(HAlign_Center)
	.MinDesiredHeight(FOptionalSize(75.f))
	.VAlign(VAlign_Fill)
	.ToolTipText(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreviewTooltip","There is currently no Node selected or is invalid!\n\nPlease fix this as this Dialogue Graph will be invalidated upon start!"))
	[
		// OUTER STYLE
		SNew(SBorder)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.SoftEdges"))
		.Padding(3.0f)
		.BorderBackgroundColor(FSlateColor(FLinearColor::Red))
		[
			SNew(SOverlay)

			// Adding some colours so its not so boring
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				// INNER STYLE
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.SoftEdges"))
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.BorderBackgroundColor(MounteaDialogueGraphColors::Overlay::DarkTheme)
			]
			
			// Pins and node details
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSpacer)
					.Size(FVector2D(0.f, 10.f))
				]

				+ SVerticalBox::Slot()
				.Padding(FMargin(NodePadding.Left, 0.0f, NodePadding.Right, 0.0f))
				.VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges"))
						.BorderBackgroundColor(MounteaDialogueGraphColors::DecoratorsBody::Default)
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.Visibility(EVisibility::SelfHitTestInvisible)
						[
							SNew(SBox)
							.MinDesiredWidth(FOptionalSize(145.f))
							.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
							[
								SNew(STextBlock)
								.Text(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreview_Invalid", "INVALID"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.Justification(ETextJustify::Center)
								.Visibility(EVisibility::Visible)
								.ColorAndOpacity(MounteaDialogueGraphColors::TextColors::Normal)
							]
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSpacer)
					.Size(FVector2D(0.f, 10.f))
				]
			]
		]
	];
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

	if (UMounteaDialogueGraphNode_ReturnToNode* EditingReturnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode))
	{
		EditingReturnNode->ReturnNodeUpdated.BindSP(this, &FMounteaDialogueGraphNode_Details::ResetPreviewingNode);

		IDetailCategoryBuilder& PreviewCategoryBuilder = DetailBuilder.EditCategory(TEXT("Return"), FText::GetEmpty(), ECategoryPriority::Uncommon);

		MakePreviewNode();
		PreviewCategoryBuilder.AddCustomRow(LOCTEXT("MounteaDialogueGraphNode_Details_Preview", "Node Dialogue Preview"), false)
		.WholeRowWidget
		[
			PreviewNode.ToSharedRef()
		];
		
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_ReturnToNode, SelectedNode));
	}

	if (UMounteaDialogueGraphNode_StartNode* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_StartNode>(EditingNode) )
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_StartNode, NodeDecorators));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_StartNode, bInheritGraphDecorators));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_StartNode, bAutoStarts));
	}

	// Hide those categories in Graph Editor
	DetailBuilder.HideCategory("Base");
	DetailBuilder.HideCategory("Private");
	DetailBuilder.HideCategory("Hidden");
	DetailBuilder.HideCategory("Hide");
	DetailBuilder.HideCategory("Editor");
}

#undef LOCTEXT_NAMESPACE