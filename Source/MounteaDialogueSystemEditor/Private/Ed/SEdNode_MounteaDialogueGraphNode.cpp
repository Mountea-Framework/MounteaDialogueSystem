// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "SEdNode_MounteaDialogueGraphNode.h"

#include "Nodes/MounteaDialogueGraphNode.h"
#include "Helpers/MounteaDialogueGraphColors.h"
#include "Ed/SEdNode_MounteaDialogueGraphNodeIndex.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"

#include "SLevelOfDetailBranchNode.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SCommentBubble.h"
#include "SlateOptMacros.h"
#include "SGraphPin.h"
#include "GraphEditorSettings.h"
#include "Blueprint/UserWidget.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "EdNode_MounteaDialogueGraph"

#pragma region Pin

class SMounteaDialogueGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SMounteaDialogueGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
	{
		this->SetCursor(EMouseCursor::Default);

		bShowLabel = true;

		GraphPinObj = InPin;
		check(GraphPinObj != nullptr);

		const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
		check(Schema);

		// Pins Out/In Border
		SBorder::Construct(SBorder::FArguments()
			.BorderImage(this, &SMounteaDialogueGraphPin::GetPinBorder)
			.BorderBackgroundColor(this, &SMounteaDialogueGraphPin::GetPinColor)
			.OnMouseButtonDown(this, &SMounteaDialogueGraphPin::OnPinMouseDown)
			.Cursor(this, &SMounteaDialogueGraphPin::GetPinCursor)
			.Padding(FMargin(5.0f))
		);
	}

protected:
	virtual FSlateColor GetPinColor() const override
	{
		if (const UMounteaDialogueGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
		{
			switch (GraphEditorSettings->GetNodeTheme())
			{
				case ENodeTheme::ENT_DarkTheme:
					return MounteaDialogueGraphColors::PinsDock::LightTheme;
				case ENodeTheme::ENT_LightTheme:
					return MounteaDialogueGraphColors::PinsDock::DarkTheme;
				default:
					return MounteaDialogueGraphColors::PinsDock::LightTheme;
			} 
		}
		
		return MounteaDialogueGraphColors::Pin::Default;
	}

	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		return SNew(STextBlock);
	}

	const FSlateBrush* GetPinBorder() const
	{
		if (const UMounteaDialogueGraphEditorSettings* GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
		{
			switch (GraphEditorSettings->GetNodeType())
			{
				case ENodeType::ENT_SoftCorners:
					return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
				case ENodeType::ENT_HardCorners: 
					return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextHardEdges");
			}
		}

		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Graph.PinDocksOverlay"));
	}
};

#pragma endregion

void SEdNode_MounteaDialogueGraphNode::Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphNode* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
	InNode->SEdNode = this;

	GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();

	if (!GraphEditorSettings)
	{
		GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SBox> SEdNode_MounteaDialogueGraphNode::ConstructMainNodeElements_Stack(TSharedPtr<SErrorText>& ErrorText, TSharedPtr<SNodeTitle>& NodeTitle, TSharedPtr<STextBlock>& DecoratorsText, TSharedPtr<SVerticalBox>& NameVerticalBox)
{
	return
	SNew(SBox)
     .MinDesiredHeight(FOptionalSize(30.f))
     .VAlign(VAlign_Fill)
	.Visibility(this, &SEdNode_MounteaDialogueGraphNode::GetStackVisibility)
	[
		SNew(SVerticalBox)

		// DECORATORS INHERITANCE SLOT
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
			.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsBackgroundColor)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Stack)
			[
				SNew(SBox)
				.MinDesiredWidth(FOptionalSize(145.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						  .HAlign(HAlign_Fill)
						  .VAlign(VAlign_Fill)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							  .AutoHeight()
							  .HAlign(HAlign_Center)
							  .VAlign(VAlign_Fill)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Center)
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.Padding(FMargin(4.0f, 0.f, 4.0f, 0.f))
										[
											SNew(STextBlock)
											.Text(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsInheritanceText)
											.Justification(ETextJustify::Center)
										]
									]
								]
							]
						]
					]
				]
			]
		]

		// SPACER
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Stack)
			[
				SNew(SSpacer)
				.Size(FVector2D(0.f, 0.25f))
			]
			
		]

		// DECORATORS SLOT
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(DecoratorsBody, SBorder)
			.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
			.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsBackgroundColor)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowDecoratorsSlot_Stack)
			[
				SNew(SBox)
				.MinDesiredWidth(FOptionalSize(140.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						  .HAlign(HAlign_Fill)
						  .VAlign(VAlign_Fill)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							  .AutoHeight()
							  .HAlign(HAlign_Center)
							  .VAlign(VAlign_Fill)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									  .AutoWidth()
									  .HAlign(HAlign_Center)
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.Padding(FMargin(4.0f, 0.f, 4.0f, 0.f))
										[
											SAssignNew(DecoratorsText, STextBlock)
											.Text(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsText)
											.Justification(ETextJustify::Center)
										]
									]
								]
							]
						]
					]
				]
			]
		]

		// SPACER
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowDecoratorsSlot_Stack)
			[
				SNew(SSpacer)
				.Size(FVector2D(0.f, 0.25f))
			]
			
		]

		// NAME SLOT
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(NodeBody, SBorder)
			.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
			.BorderBackgroundColor(
			this, &SEdNode_MounteaDialogueGraphNode::GetNodeTitleBackgroundColor)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SNew(SBox)
				.MinDesiredWidth(FOptionalSize(145.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSpacer)
						.Size(FVector2D(5.f, 0.f))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						  .HAlign(HAlign_Fill)
						  .VAlign(VAlign_Fill)
						[
							SAssignNew(NameVerticalBox, SVerticalBox)
							+ SVerticalBox::Slot()
							  .AutoHeight()
							  .HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									// POPUP ERROR MESSAGE
									SAssignNew(ErrorText, SErrorText)
									.BackgroundColor(
	                                 this,
	                                 &SEdNode_MounteaDialogueGraphNode::GetErrorColor)
									.ToolTipText(
	                                 this,
	                                 &SEdNode_MounteaDialogueGraphNode::GetErrorMsgToolTip)
								]

								+ SHorizontalBox::Slot()
								  .AutoWidth()
								  .HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										  .HAlign(HAlign_Center)
										  .AutoHeight()
										[
											SAssignNew(InlineEditableText, SInlineEditableTextBlock)
											.Style(FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
											.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
											.OnVerifyTextChanged(
											this, &SEdNode_MounteaDialogueGraphNode::OnVerifyNameTextChanged)
											.OnTextCommitted(
											this, &SEdNode_MounteaDialogueGraphNode::OnNameTextCommitted)
											.IsReadOnly(this, &SEdNode_MounteaDialogueGraphNode::IsNameReadOnly)
											.IsSelected(this, &SEdNode_MounteaDialogueGraphNode::IsSelectedExclusively)
											.Justification(ETextJustify::Center)
										]
										+ SVerticalBox::Slot()
										.AutoHeight()
										[
											NodeTitle.ToSharedRef()
										]
									]
								]
							]
						]
					]
				]
			]
		]
	];
}

TSharedRef<SBox> SEdNode_MounteaDialogueGraphNode::ConstructMainNodeElements_Unified(TSharedPtr<SErrorText>& ErrorText, TSharedPtr<SNodeTitle>& NodeTitle, TSharedPtr<STextBlock>& DecoratorsText, TSharedPtr<SVerticalBox>& NameVerticalBox)
{
	TSharedPtr<SBox> SBox_InheritanceOnly;
	TSharedPtr<SBox> SBox_ImplementsOnly;
	TSharedPtr<SBox> SBox_All;
	TSharedPtr<SBox> SBox_BottomPadding;
	
	return
	SNew(SBox)
     .MinDesiredHeight(FOptionalSize(30.f))
     .VAlign(VAlign_Fill)
	.Visibility(this, &SEdNode_MounteaDialogueGraphNode::GetUnifiedVisibility)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(NodeBody, SBorder)
			.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
			.BorderBackgroundColor(
			this, &SEdNode_MounteaDialogueGraphNode::GetNodeTitleBackgroundColor)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SNew(SBox)
				.MinDesiredWidth(FOptionalSize(140.f))
				[
					SNew(SVerticalBox)
					
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						  .HAlign(HAlign_Center)
						  .VAlign(VAlign_Fill)
						[
							SAssignNew(NameVerticalBox, SVerticalBox)

							// NAME SLOT
							+ SVerticalBox::Slot()
							  .AutoHeight()
							  .HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									// POPUP ERROR MESSAGE
									SAssignNew(ErrorText, SErrorText)
									.BackgroundColor(
	                                 this,
	                                 &SEdNode_MounteaDialogueGraphNode::GetErrorColor)
									.ToolTipText(
	                                 this,
	                                 &SEdNode_MounteaDialogueGraphNode::GetErrorMsgToolTip)
								]
								+ SHorizontalBox::Slot()
								  .AutoWidth()
								  .HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										  .HAlign(HAlign_Center)
										  .AutoHeight()
										[
											SAssignNew(InlineEditableText, SInlineEditableTextBlock)
											.Style(FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
											.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
											.OnVerifyTextChanged(
											this, &SEdNode_MounteaDialogueGraphNode::OnVerifyNameTextChanged)
											.OnTextCommitted(
											this, &SEdNode_MounteaDialogueGraphNode::OnNameTextCommitted)
											.IsReadOnly(this, &SEdNode_MounteaDialogueGraphNode::IsNameReadOnly)
											.IsSelected(this, &SEdNode_MounteaDialogueGraphNode::IsSelectedExclusively)
											.Justification(ETextJustify::Center)
											.Visibility(EVisibility::Visible)
										]
										
										+ SVerticalBox::Slot()
										.AutoHeight()
										[
											NodeTitle.ToSharedRef()
										]
									]
								]
							]
						]
					]

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					[
						SNew(SBox)
						.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowDecoratorsBottomPadding)
						[
							SNew(SSpacer)
							.Size(FVector2D(0.f, 2.5f))
						]
						
					]

					// INHERITS ONLY
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					.Padding(FMargin(4.0f, 0.f, 4.0f, 0.f))
					[
						SNew(SBox)
						.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified)
						.MaxDesiredWidth(FOptionalSize(135.f))
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFitX)
							[
								SNew(SUniformGridPanel)
								.Visibility(EVisibility::HitTestInvisible)
								.SlotPadding(FMargin(6.0f, 0.f, 0.0f, 0.f))

								+ SUniformGridPanel::Slot(0,0)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("A", "DECORATORS"))
								]
								
								+ SUniformGridPanel::Slot(0,1)
								[
									SNew(SBox)
									.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("B", "inherits"))
									]
									
								]
								+ SUniformGridPanel::Slot(1,1)
								[
									SNew(SBox)
									.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified)
									.MaxAspectRatio(FOptionalSize(1))
									.MaxDesiredHeight(FOptionalSize(8.f))
									.MaxDesiredWidth(FOptionalSize(8.f))
									[
										SNew(SImage)
										.Image(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageBrush)
										.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageTint)
									]
								]
							]
						]
					]
					
					// IMPLEMENTS ONLY
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					.Padding(FMargin(4.0f, 0.f, 4.0f, 0.f))
					[
						SNew(SBox)
						.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowDecoratorsSlot_Unified)
						.MaxDesiredWidth(FOptionalSize(135.f))
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFitX)
							[
								SNew(SUniformGridPanel)
								.Visibility(EVisibility::HitTestInvisible)
								.SlotPadding(FMargin(6.0f, 0.f, 0.0f, 0.f))

								+ SUniformGridPanel::Slot(0,0)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("A", "DECORATORS"))
								]

								+ SUniformGridPanel::Slot(0,1)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("C", "implements"))
								]
								+ SUniformGridPanel::Slot(1,1)
								[
									SNew(STextBlock)
									.Text(this, &SEdNode_MounteaDialogueGraphNode::GetNumberOfDecorators)
								]
							]
						]
					]
						
					// BOTH
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					.Padding(FMargin(4.0f, 0.f, 4.0f, 0.f))
					[
						SNew(SBox)
						.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
						.MaxDesiredWidth(FOptionalSize(135.f))
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFitX)
							[
								SNew(SUniformGridPanel)
								.Visibility(EVisibility::HitTestInvisible)
								.SlotPadding(FMargin(6.0f, 0.f, 0.0f, 0.f))

								+ SUniformGridPanel::Slot(0,0)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("A", "DECORATORS"))
								]
				
								+ SUniformGridPanel::Slot(0,1)
								[
									SNew(SBox)
									.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("B", "inherits"))
									]
					
								]
								+ SUniformGridPanel::Slot(1,1)
								[
									SNew(SBox)
									.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
									.MaxAspectRatio(FOptionalSize(1))
									.MaxDesiredHeight(FOptionalSize(8.f))
									.MaxDesiredWidth(FOptionalSize(8.f))
									[
										SNew(SImage)
										.Image(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageBrush)
										.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageTint)
									]
								]

								+ SUniformGridPanel::Slot(0,2)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("C", "implements"))
								]
								+ SUniformGridPanel::Slot(1,2)
								[
									SNew(STextBlock)
									.Text(this, &SEdNode_MounteaDialogueGraphNode::GetNumberOfDecorators)
								]
							]
						]
					]
					
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					[
						SNew(SBox)
						.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowDecoratorsBottomPadding)
						[
							SNew(SSpacer)
							.Size(FVector2D(0.f, 2.5f))
						]
					]
				]
			]
		]
	];
}

void SEdNode_MounteaDialogueGraphNode::UpdateGraphNode()
{
	const FMargin NodePadding = FMargin(2.0f);

	InputPins.Empty();
	OutputPins.Empty();

	// Used in GetOverlayWidgets
	IndexOverlayWidget = SNew(SEdNode_MounteaDialogueGraphNodeIndex)
		.OverlayBody
		(
			SNew(STextBlock)
			.Text(this, &SEdNode_MounteaDialogueGraphNode::GetIndexText)
			.ColorAndOpacity(FLinearColor::White)
			.Font(FEditorStyle::GetFontStyle("BTEditor.Graph.BTNode.IndexText"))
		)
		.ToolTipText(this, &SEdNode_MounteaDialogueGraphNode::GetIndexOverlayTooltipText)
		.Visibility(EVisibility::Visible)
		.OnHoverStateChanged(this, &SEdNode_MounteaDialogueGraphNode::OnIndexHoverStateChanged)
		.OnGetBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetOverlayWidgetBackgroundColor);

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	OutputPinBox.Reset();

	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	TSharedPtr<STextBlock> DecoratorsText;
	TSharedPtr<SVerticalBox> NameVerticalBox;

	TSharedPtr<SVerticalBox> StackBox;
	TSharedPtr<SVerticalBox> UniformBox;
		
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill) 
		.VAlign(VAlign_Fill)
		[
			SNew(SBox)
			[
				// OUTER STYLE
				SNew(SBorder)
				.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetNodeTypeBrush)
				.Padding(3.0f)
				.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetBorderBackgroundColor)
				[
					SNew(SOverlay)

					// Adding some colours so its not so boring
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						// INNER STYLE
						SNew(SBorder)
						.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetNodeTypeBrush)
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetBorderFrontColor)
					]
					
					// Pins and node details
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SVerticalBox)

						// INPUT PIN AREA
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.MinDesiredHeight(NodePadding.Top)
							[
								SAssignNew(LeftNodeBox, SVerticalBox)
							]
						]

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
							ConstructMainNodeElements_Stack(ErrorText, NodeTitle, DecoratorsText, NameVerticalBox)
						]

						+ SVerticalBox::Slot()
						.Padding(FMargin(NodePadding.Left, 0.0f, NodePadding.Right, 0.0f))
						.VAlign(VAlign_Fill)
						[
							ConstructMainNodeElements_Unified(ErrorText, NodeTitle, DecoratorsText, NameVerticalBox)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SSpacer)
							.Size(FVector2D(0.f, 10.f))
						]
						
						// OUTPUT PIN AREA
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.MinDesiredHeight(NodePadding.Bottom)
							[
								SAssignNew(RightNodeBox, SVerticalBox)
								+ SVerticalBox::Slot()
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Fill)
								.Padding(20.0f, 0.0f)
								.FillHeight(1.0f)
								[
									SAssignNew(OutputPinBox, SHorizontalBox)
								]
							]
						]
					]
				]
			]
		];

	// Create comment bubble
	TSharedPtr<SCommentBubble> CommentBubble;
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

	SAssignNew(CommentBubble, SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.ColorAndOpacity(CommentColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
		.SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
		.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
		.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
		.VAlign(VAlign_Top)
		[
			CommentBubble.ToSharedRef()
		];

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
}

void SEdNode_MounteaDialogueGraphNode::CreatePinWidgets()
{
	UEdNode_MounteaDialogueGraphNode* StateNode = CastChecked<UEdNode_MounteaDialogueGraphNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < StateNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = StateNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SMounteaDialogueGraphPin, MyPin);
			
			AddPin(NewPin.ToSharedRef());
		}
	}
}

void SEdNode_MounteaDialogueGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			.Padding(20.0f,0.0f)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
		{
		OutputPinBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillWidth(1.0f)
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
		}
}

bool SEdNode_MounteaDialogueGraphNode::IsNameReadOnly() const
{
	UEdNode_MounteaDialogueGraphNode* EdNode_Node = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	check(EdNode_Node != nullptr);

	UMounteaDialogueGraph* MounteaDialogueGraphNode = EdNode_Node->DialogueGraphNode->Graph;
	check(MounteaDialogueGraphNode != nullptr);

	return !MounteaDialogueGraphNode->bCanRenameNode || SGraphNode::IsNameReadOnly();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SEdNode_MounteaDialogueGraphNode::OnNameTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
{
	if (InText.IsEmpty()) return;
	
	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	UEdNode_MounteaDialogueGraphNode* MyNode = CastChecked<UEdNode_MounteaDialogueGraphNode>(GraphNode);

	if (MyNode != nullptr && MyNode->DialogueGraphNode != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("MounteaDiaogueGraphEditorRenameNode", "Mountea Diaogue Editor: Rename Node"));
		MyNode->Modify();
		MyNode->DialogueGraphNode->Modify();
		MyNode->DialogueGraphNode->SetNodeTitle(InText);
		UpdateGraphNode();
	}
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetNodeTypeBrush() const
{
	if (GraphEditorSettings)
	{
		switch (GraphEditorSettings->GetNodeType())
		{
			case ENodeType::ENT_SoftCorners:
				return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.SoftEdges");
			case ENodeType::ENT_HardCorners: 
				return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.HardEdges");
		}
	}

	return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.SoftEdges");
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush() const
{
	if (GraphEditorSettings)
	{
		switch (GraphEditorSettings->GetNodeType())
		{
		case ENodeType::ENT_SoftCorners:
			return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
		case ENodeType::ENT_HardCorners: 
			return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextHardEdges");
		}
	}

	return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBorderBackgroundColor() const
{
	UEdNode_MounteaDialogueGraphNode* MyNode = CastChecked<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	return MyNode ? MyNode->GetBackgroundColor() : MounteaDialogueGraphColors::NodeBorder::HighlightAbortRange0;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBorderFrontColor() const
{
	if (GraphEditorSettings)
	{
		switch (GraphEditorSettings->GetNodeTheme())
		{
			case ENodeTheme::ENT_DarkTheme:
				return  MounteaDialogueGraphColors::Overlay::DarkTheme;
			case ENodeTheme::ENT_LightTheme:
				return MounteaDialogueGraphColors::Overlay::LightTheme;
		} 
	}

	return MounteaDialogueGraphColors::Overlay::DarkTheme;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetNodeTitleBackgroundColor() const
{
	return MounteaDialogueGraphColors::NodeBody::Default;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetDecoratorsBackgroundColor() const
{
	return MounteaDialogueGraphColors::DecoratorsBody::Default;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetPinsDockColor() const
{
	if (GraphEditorSettings)
	{
		switch (GraphEditorSettings->GetNodeTheme())
		{
		case ENodeTheme::ENT_DarkTheme:
			return  MounteaDialogueGraphColors::PinsDock::DarkTheme;
		case ENodeTheme::ENT_LightTheme:
			return MounteaDialogueGraphColors::PinsDock::LightTheme;
		}
	}

	return MounteaDialogueGraphColors::Overlay::DarkTheme;
}

EVisibility SEdNode_MounteaDialogueGraphNode::GetDragOverMarkerVisibility() const
{
	return EVisibility::Visible;
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetNameIcon() const
{
	return FEditorStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetInheritsImageBrush() const
{
	bool bHasDecorators = false;
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			bHasDecorators =  EdParentNode->DialogueGraphNode->DoesInheritDecorators() ;
		}
	}
	
	return FMounteaDialogueGraphEditorStyle::GetBrush( bHasDecorators ? "MDSStyleSet.Icon.OK" : "MDSStyleSet.Icon.Error" );
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetInheritsImageTint() const
{
	bool bHasDecorators = false;
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			bHasDecorators =  EdParentNode->DialogueGraphNode->DoesInheritDecorators() ;
		}
	}
	
	return bHasDecorators ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Red);
}

FText SEdNode_MounteaDialogueGraphNode::GetIndexOverlayTooltipText() const
{
	return LOCTEXT("NodeIndexTooltip", "Node index");
}

FText SEdNode_MounteaDialogueGraphNode::GetIndexText() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			return FText::AsNumber(EdParentNode->DialogueGraphNode->GetNodeIndex());
		}
	}
	return FText::AsNumber(INDEX_NONE);
}

void SEdNode_MounteaDialogueGraphNode::OnIndexHoverStateChanged(bool bArg) const
{
	// TODO something
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetOverlayWidgetBackgroundColor(bool bArg) const
{
	return bArg ? MounteaDialogueGraphColors::IndexBorder::HoveredState : MounteaDialogueGraphColors::IndexBorder::NormalState;
}

bool SEdNode_MounteaDialogueGraphNode::HasGraphDecorators() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode && EdParentNode->DialogueGraphNode->Graph)
		{
			for (const auto Itr :  EdParentNode->DialogueGraphNode->Graph->GetGraphDecorators())
			{
				if (Itr.DecoratorType != nullptr)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool SEdNode_MounteaDialogueGraphNode::HasNodeDecorators() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			if (EdParentNode->DialogueGraphNode->GetNodeDecorators().Num() > 0)
			{
				bool bAllValid = true;

				for (const auto Itr : EdParentNode->DialogueGraphNode->GetNodeDecorators())
				{
					if (Itr.DecoratorType == nullptr)
					{
						bAllValid = false;
					}
				}
				return bAllValid;
			}
		}
	}

	return false;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowDecoratorsSlot_Unified() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->ShowDetailedInfo_NumDecorators() && !GraphEditorSettings->ShowDetailedInfo_InheritsDecorators())
		{
			return EVisibility::Visible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

FText SEdNode_MounteaDialogueGraphNode::GetDecoratorsText() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			FString Number = FString::FromInt(EdParentNode->DialogueGraphNode->GetNodeDecorators().Num());
			FString ReturnText = FString(TEXT("DECORATORS: "));
			return FText::FromString( ReturnText.Append(Number) );
		}
	}
	return FText::FromString("DECORATORS: none");
}

FText SEdNode_MounteaDialogueGraphNode::GetNumberOfDecorators() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			return FText::FromString( FString::FromInt(EdParentNode->DialogueGraphNode->GetNodeDecorators().Num()) );
		}
	}
	return FText::FromString("-");
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators() && !GraphEditorSettings->ShowDetailedInfo_NumDecorators())
		{
			return EVisibility::Visible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowDecoratorsSlot_Stack() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->ShowDetailedInfo_NumDecorators())
		{
			return EVisibility::Visible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Stack() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators())
		{
			return EVisibility::Visible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowAllDecorators() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators() && GraphEditorSettings->ShowDetailedInfo_NumDecorators())
		{
			return EVisibility::Visible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowDecoratorsBottomPadding() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators() || GraphEditorSettings->ShowDetailedInfo_NumDecorators())
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

FText SEdNode_MounteaDialogueGraphNode::GetDecoratorsInheritanceText() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			FString Result =  EdParentNode->DialogueGraphNode->DoesInheritDecorators() ? TEXT("yes") : TEXT("no") ;
			return FText::FromString( FString(TEXT("INHERITS: ")).Append(Result) );
		}
	}
	return FText::FromString("invalid");
}

EDecoratorsInfoStyle SEdNode_MounteaDialogueGraphNode::GetDecoratorsStyle() const
{
	if (GraphEditorSettings)
	{
		return GraphEditorSettings->GetDecoratorsStyle();
	}

	if (const auto TempSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>())
	{
		return TempSettings->GetDecoratorsStyle();
	}

	return EDecoratorsInfoStyle::EDSI_Stack;
}

EVisibility SEdNode_MounteaDialogueGraphNode::GetStackVisibility() const
{
	return GetDecoratorsStyle() == EDecoratorsInfoStyle::EDSI_Stack ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::GetUnifiedVisibility() const
{
	return GetDecoratorsStyle() == EDecoratorsInfoStyle::EDIS_Unified ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
