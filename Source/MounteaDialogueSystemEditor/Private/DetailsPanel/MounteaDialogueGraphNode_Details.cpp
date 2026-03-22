// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueGraphNode_Details.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorCommands/FMounteaDialogueGraphEditorCommands.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Helpers/MounteaDialogueGraphColors.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "ScopedTransaction.h"
#include "Graph/MounteaDialogueGraph.h"

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

	const FLinearColor previewAccent = GetPreviewingNodeBackgroundColor().GetSpecifiedColor();
	const FLinearColor headerForeground = FMounteaDialogueGraphVisualTokens::GetNodeHeaderForeground(EditingDialogueNode->SelectedNode);
	const FLinearColor bodyForeground = FMounteaDialogueGraphVisualTokens::GetCardForeground();
	const FLinearColor bodyBackground = FMounteaDialogueGraphVisualTokens::GetCardBackground();

	PreviewNode =  SNew(SBox)
	.Padding(FMargin(0.f, 4.f, 0.f, 4.f))
	.MinDesiredWidth(FOptionalSize(240.f))
	.MaxDesiredWidth(FOptionalSize(300.f))
	.HAlign(HAlign_Center)
	.MinDesiredHeight(FOptionalSize(88.f))
	.VAlign(VAlign_Fill)
	.ToolTipText(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreviewTooltip","This node is currently selected as Return Node.\nUpon execution of Return to Node, Dialogue will move to Selected Node.\n\nDouble click on this Preview to focus on this Node in Graph."))
	[
		SNew(SBorder)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
		.Padding(1.0f)
		.BorderBackgroundColor(previewAccent)
		.OnMouseDoubleClick(this, &FMounteaDialogueGraphNode_Details::OnPreviewingNodeDoubleClicked)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
				.Visibility(EVisibility::SelfHitTestInvisible)
				.BorderBackgroundColor(bodyBackground)
			]

			+ SOverlay::Slot()
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundTop"))
				.Padding(FMargin(10.f, 6.f, 10.f, 6.f))
				.BorderBackgroundColor(previewAccent)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Icon.Return"))
						.ColorAndOpacity(headerForeground)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(this, &FMounteaDialogueGraphNode_Details::GetPreviewingNodeTitle)
						.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
						.ColorAndOpacity(headerForeground)
					]
				]
			]

			+ SOverlay::Slot()
			.Padding(FMargin(10.f, 36.f, 10.f, 10.f))
			.VAlign(VAlign_Fill)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreview_Body", "Double-click to focus this target node in graph."))
				.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTag")
				.ColorAndOpacity(bodyForeground.CopyWithNewOpacity(0.85f))
				.AutoWrapText(true)
			]
		]
	];


	FSimpleNoReplyPointerEventHandler onPreviewingNodeMouseLeaveEventHandle;
	onPreviewingNodeMouseLeaveEventHandle.BindSP(this, &FMounteaDialogueGraphNode_Details::OnPreviewingNodeMouseLeave);

	FNoReplyPointerEventHandler onPreviewingNodeMouseEnterEventHandle;
	onPreviewingNodeMouseEnterEventHandle.BindSP(this, &FMounteaDialogueGraphNode_Details::OnPreviewingNodeMouseEnter);

	PreviewNode->SetOnMouseLeave(onPreviewingNodeMouseLeaveEventHandle);
	PreviewNode->SetOnMouseEnter(onPreviewingNodeMouseEnterEventHandle);
}

void FMounteaDialogueGraphNode_Details::MakeInvalidPreviewNode() 
{
	const FLinearColor previewAccent = FLinearColor(0.85f, 0.15f, 0.15f, 1.0f);
	const FLinearColor bodyForeground = FMounteaDialogueGraphVisualTokens::GetCardForeground();
	const FLinearColor bodyBackground = FMounteaDialogueGraphVisualTokens::GetCardBackground();

	PreviewNode =  SNew(SBox)
	.Padding(FMargin(0.f, 4.f, 0.f, 4.f))
	.MinDesiredWidth(FOptionalSize(240.f))
	.MaxDesiredWidth(FOptionalSize(300.f))
	.HAlign(HAlign_Center)
	.MinDesiredHeight(FOptionalSize(88.f))
	.VAlign(VAlign_Fill)
	.ToolTipText(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreviewTooltip","There is currently no Node selected or is invalid!\n\nPlease fix this as this Dialogue Graph will be invalidated upon start!"))
	[
		SNew(SBorder)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
		.Padding(1.0f)
		.BorderBackgroundColor(previewAccent)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
				.BorderBackgroundColor(bodyBackground)
			]

			+ SOverlay::Slot()
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundTop"))
				.Padding(FMargin(10.f, 6.f, 10.f, 6.f))
				.BorderBackgroundColor(previewAccent)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreview_Invalid", "INVALID"))
					.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
					.ColorAndOpacity(FLinearColor::White)
				]
			]

			+ SOverlay::Slot()
			.Padding(FMargin(10.f, 36.f, 10.f, 10.f))
			.VAlign(VAlign_Fill)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MounteaDialogueGraphNode_Details_SlectedNodePreview_InvalidBody", "No valid target node selected."))
				.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTag")
				.ColorAndOpacity(bodyForeground.CopyWithNewOpacity(0.85f))
				.AutoWrapText(true)
			]
		]
	];
}

void FMounteaDialogueGraphNode_Details::RefreshEligibleNodes()
{
	CachedEligibleNodes.Reset();
	const UMounteaDialogueGraphNode_ReturnToNode* ReturnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	if (!ReturnNode || !ReturnNode->Graph) return;

	for (UMounteaDialogueGraphNode* Node : ReturnNode->Graph->GetAllNodes())
	{
		if (!Node) continue;
		bool bFiltered = false;
		for (const TSubclassOf<UMounteaDialogueGraphNode>& FilterClass : ReturnNode->AllowedNodesFilter)
		{
			if (Node->IsA(FilterClass)) { bFiltered = true; break; }
		}
		if (!bFiltered)
			CachedEligibleNodes.Add(Node);
	}
}

FText FMounteaDialogueGraphNode_Details::GetPickerCurrentNodeTitle() const
{
	const UMounteaDialogueGraphNode_ReturnToNode* ReturnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	if (!ReturnNode || !ReturnNode->SelectedNode)
		return LOCTEXT("NodePicker_None", "— select a node —");
	return ReturnNode->SelectedNode->GetNodeTitle();
}

FSlateColor FMounteaDialogueGraphNode_Details::GetPickerCurrentNodeColor() const
{
	const UMounteaDialogueGraphNode_ReturnToNode* ReturnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	if (!ReturnNode || !ReturnNode->SelectedNode)
		return FLinearColor(0.25f, 0.25f, 0.25f, 1.f);
	return FMounteaDialogueGraphVisualTokens::GetNodeAccentColor(ReturnNode->SelectedNode);
}

TSharedRef<SWidget> FMounteaDialogueGraphNode_Details::MakeNodePickerButtonContent()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
			.BorderBackgroundColor(this, &Self::GetPickerCurrentNodeColor)
			.Padding(FMargin(0.f))
			[
				SNew(SBox)
				.WidthOverride(10.f)
				.HeightOverride(20.f)
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(FMargin(6.f, 0.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(this, &Self::GetPickerCurrentNodeTitle)
			.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
		];
}

TSharedRef<SWidget> FMounteaDialogueGraphNode_Details::GetNodePickerMenuContent()
{
	RefreshEligibleNodes();

	TSharedRef<SScrollBox> List = SNew(SScrollBox);

	if (CachedEligibleNodes.IsEmpty())
	{
		List->AddSlot()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NodePicker_Empty", "No eligible nodes found."))
			.Margin(FMargin(8.f))
		];
		return List;
	}

	for (UMounteaDialogueGraphNode* Node : CachedEligibleNodes)
	{
		const FLinearColor accentColor = FMounteaDialogueGraphVisualTokens::GetNodeAccentColor(Node);
		const FLinearColor fgColor     = FMounteaDialogueGraphVisualTokens::GetCardForeground();
		const FText title              = Node->GetNodeTitle();
		const FText typeName           = Node->GetInternalName();

		List->AddSlot()
		.Padding(FMargin(2.f, 1.f))
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.OnClicked_Lambda([this, Node]() -> FReply
			{
				OnPickerNodeSelected(Node);
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
				.BorderBackgroundColor(FMounteaDialogueGraphVisualTokens::GetCardBackground())
				.Padding(0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
						.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.Flat.RoundAll"))
						.BorderBackgroundColor(accentColor)
						.Padding(0.f)
						[
							SNew(SBox)
							.WidthOverride(8.f)
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					.Padding(FMargin(8.f, 4.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(title)
							.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
							.ColorAndOpacity(fgColor)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(typeName)
							.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTag")
							.ColorAndOpacity(fgColor.CopyWithNewOpacity(0.5f))
						]
					]
				]
			]
		];
	}

	return SNew(SBox).MaxDesiredHeight(300.f)[ List ];
}

void FMounteaDialogueGraphNode_Details::OnPickerNodeSelected(UMounteaDialogueGraphNode* Node)
{
	if (NodePickerComboButton.IsValid())
		NodePickerComboButton->SetIsOpen(false);

	UMounteaDialogueGraphNode_ReturnToNode* ReturnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(EditingNode);
	if (!ReturnNode) return;

	const FScopedTransaction Transaction(LOCTEXT("NodePicker_Select", "Select Return Target Node"));
	ReturnNode->Modify();

	ReturnNode->SelectedNode = Node;
	if (Node && ReturnNode->Graph)
	{
		const int32 Idx = ReturnNode->Graph->AllNodes.Find(Node);
		ReturnNode->SelectedNodeIndex = (Idx != INDEX_NONE) ? FString::FromInt(Idx) : TEXT("");
	}
	else
	{
		ReturnNode->SelectedNodeIndex = TEXT("");
	}

	ReturnNode->ReturnNodeUpdated.ExecuteIfBound();
	MakePreviewNode();
	if (SavedLayoutBuilder) SavedLayoutBuilder->ForceRefreshDetails();
}

TSharedRef<SWidget> FMounteaDialogueGraphNode_Details::MakeNodePickerWidget()
{
	return SNew(SBox)
	.Padding(FMargin(0.f, 2.f, 0.f, 4.f))
	[
		SAssignNew(NodePickerComboButton, SComboButton)
		.OnGetMenuContent(this, &Self::GetNodePickerMenuContent)
		.ContentPadding(FMargin(8.f, 4.f))
		.ToolTipText(LOCTEXT("NodePicker_Tooltip",
			"Select the target node this Return Node will jump to.\nThe dropdown shows all eligible nodes in the graph."))
		.ButtonContent()
		[
			MakeNodePickerButtonContent()
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

		IDetailCategoryBuilder& ReturnCategory = DetailBuilder.EditCategory(TEXT("Return"), FText::GetEmpty(), ECategoryPriority::Uncommon);

		ReturnCategory.AddCustomRow(LOCTEXT("NodePicker_Label", "Target Node"), false)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NodePicker_Name", "Target Node"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(200.f)
		[
			MakeNodePickerWidget()
		];

		MakePreviewNode();
		ReturnCategory.AddCustomRow(LOCTEXT("MounteaDialogueGraphNode_Details_Preview", "Node Dialogue Preview"), false)
		.WholeRowWidget
		[
			PreviewNode.ToSharedRef()
		];

		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_ReturnToNode, SelectedNode));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_ReturnToNode, SelectedNodeIndex));
	}

	if (UMounteaDialogueGraphNode_StartNode* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_StartNode>(EditingNode) )
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_StartNode, NodeDecorators));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_StartNode, bInheritGraphDecorators));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_StartNode, bAutoStarts));
	}

	if (UMounteaDialogueGraphNode_Delay* EditingDialogueNode = Cast<UMounteaDialogueGraphNode_Delay>(EditingNode) )
	{
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
