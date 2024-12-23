// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "SEdNode_MounteaDialogueGraphNode.h"

#include "EdGraph_MounteaDialogueGraph.h"
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
#include "UnrealEdGlobals.h"
#include "Blueprint/UserWidget.h"
#include "Data/MounteaDialogueContext.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScaleBox.h"


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
				case ENodeCornerType::ENT_SoftCorners:
					return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
				case ENodeCornerType::ENT_HardCorners: 
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
	//bIsHovered = false;
	
	GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();

	if (!GraphEditorSettings)
	{
		GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();
	}
}

void SEdNode_MounteaDialogueGraphNode::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//bIsHovered = true;
	if (GUnrealEd && !GUnrealEd->IsPlayingSessionInEditor())
	{
		SetToolTipText(GetTooltipText());
		OnVisualizeTooltip(GetToolTip()->AsWidget());
	}
	SGraphNode::OnMouseEnter(MyGeometry, MouseEvent);
}

void SEdNode_MounteaDialogueGraphNode::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	//bIsHovered = false;
	
	SetToolTipText(FText::GetEmpty());
	OnToolTipClosing();
	
	SGraphNode::OnMouseLeave(MouseEvent);
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetIndexBrush() const
{
	return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
}

FText SEdNode_MounteaDialogueGraphNode::GetNodeTitle() const
{
	FText NodeTitle =  INVTEXT("Dialogue Node");
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (EdParentNode->DialogueGraphNode)
		{
			NodeTitle = EdParentNode->DialogueGraphNode->GetNodeTitle();
		}
	}
	return NodeTitle;
}

TSharedRef<SWidget> SEdNode_MounteaDialogueGraphNode::CreateNameSlotWidget()
{
	return SAssignNew(InlineEditableText, SInlineEditableTextBlock)
	.Style(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitleInlineEditableText")
	.Text(this, &SEdNode_MounteaDialogueGraphNode::GetNodeTitle)
	.OnVerifyTextChanged(this, &SEdNode_MounteaDialogueGraphNode::OnVerifyNameTextChanged)
	.OnTextCommitted(this, &SEdNode_MounteaDialogueGraphNode::OnNameTextCommitted)
	.IsReadOnly(this, &SEdNode_MounteaDialogueGraphNode::IsNameReadOnly)
	.IsSelected(this, &SEdNode_MounteaDialogueGraphNode::IsSelectedExclusively)
	.Justification(ETextJustify::Center)
	.Visibility(EVisibility::Visible)
	.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SEdNode_MounteaDialogueGraphNode::UpdateGraphNode()
{
	if (GraphNode)
	{
		if (auto dialogueGraphNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
		{
			if (dialogueGraphNode->SEdNode == nullptr)
			{
				dialogueGraphNode->SEdNode = this;
			}
		}
	}
	
	const FMargin NodePadding = FMargin(2.0f);
	const FMargin UnifiedRowsPadding = FMargin(0.f, 1.15f, 0.f, 0.f);

	const FSlateColor DefaultFontColor = MounteaDialogueGraphColors::TextColors::Normal;

	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	OutputPinBox.Reset();

	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	TSharedPtr<SWidget> TitleWidget = CreateNameSlotWidget();

	TSharedPtr<SVerticalBox> StackBox;
	TSharedPtr<SVerticalBox> UniformBox;
		
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	
	const FSlateBrush* CircleBrush = FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.IndexCircle"));
	this->GetOrAddSlot(ENodeZone::Left)
	.SlotOffset(TAttribute<FVector2D>(this, &SEdNode_MounteaDialogueGraphNode::GetIndexSlotOffset))
	.SlotSize(TAttribute<FVector2D>(this, &SEdNode_MounteaDialogueGraphNode::GetIndexSlotSize))
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
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
			.BorderBackgroundColor(FLinearColor::Gray)
			.Padding(FMargin(4.0f))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.Text(this, &SEdNode_MounteaDialogueGraphNode::GetIndexText)
				.Visibility(this, &SEdNode_MounteaDialogueGraphNode::GetIndexSlotVisibility)
			]
		]
	];
	
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
						SNew(SVerticalBox)

#pragma region Stack

						+ SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							SNew(SBox)
							.MinDesiredWidth(FOptionalSize(145.f))
							.Visibility(this, &SEdNode_MounteaDialogueGraphNode::GetStackVisibility)
							[
								SNew(SVerticalBox)
#pragma region InheritanceOnly
								+ SVerticalBox::Slot()
								.VAlign(VAlign_Fill)
								[
									SNew(SBorder)
									.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
									.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsBackgroundColor)
									.HAlign(HAlign_Fill)
									.VAlign(VAlign_Fill)
									.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Stack)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										[
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.Padding(FMargin(4.0f, 0.f, 4.0f, 0.f))
											.HAlign(HAlign_Fill)
											[
												SNew(STextBlock)
												.Text(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsInheritanceText)
												.Justification(ETextJustify::Center)
												.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor)
											]
										]
									]
								]
#pragma endregion 

#pragma region ImplementsOnly
								+ SVerticalBox::Slot()
								.VAlign(VAlign_Fill)
								[
									SNew(SBorder)
									.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
									.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsBackgroundColor)
									.HAlign(HAlign_Fill)
									.VAlign(VAlign_Fill)
									.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowImplementsOnlySlot_Stack)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot()
										[
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.HAlign(HAlign_Fill)
											.Padding(FMargin(4.0f, 0.f, 4.0f, 0.f))
											[
												SNew(STextBlock)
												.Text(this, &SEdNode_MounteaDialogueGraphNode::GetDecoratorsText)
												.Justification(ETextJustify::Center)
												.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor)
											]
										]
									]
								]
#pragma endregion 
							]
						]

#pragma endregion 

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(NodeBody, SBorder)
							.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
							.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetNodeTitleBackgroundColor)
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
										SNew(SOverlay)
										+ SOverlay::Slot()
										  .HAlign(HAlign_Center)
										  .VAlign(VAlign_Fill)
										[
											SNew(SVerticalBox)
#pragma region NameSlot
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
															TitleWidget.ToSharedRef()
														]
														
														+ SVerticalBox::Slot()
														.AutoHeight()
														[
															NodeTitle.ToSharedRef()
														]
													]
												]
											]
#pragma endregion 
										]
									]
#pragma region Unified
									+ SVerticalBox::Slot()
									.VAlign(VAlign_Fill)
									[
										SNew(SBox)
										.Visibility(this, &SEdNode_MounteaDialogueGraphNode::GetUnifiedVisibility)
										.HAlign(HAlign_Fill)
										[
											SNew(SVerticalBox)
											+ SVerticalBox::Slot()
											[
												SNew(SBox)
												.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowDecoratorsBottomPadding)
												[
													SNew(SSpacer)
													.Size(FVector2D(0.f, 2.5f))
												]
											]

#pragma region InheritanceOnly
											// INHERITS ONLY
											+ SVerticalBox::Slot()
											.AutoHeight()
											.HAlign(HAlign_Fill)
											.VAlign(VAlign_Fill)
											.Padding(FMargin(8.0f, 0.f, 8.0f, 0.f))
											[
												SNew(SBox)
												.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified)
												.MaxDesiredWidth(FOptionalSize(130.f))
												.HAlign(HAlign_Fill)
												[
													SNew(SGridPanel)
													.Visibility(EVisibility::HitTestInvisible)
													.FillColumn(0, 2.f)
													.FillColumn(1, 1.f)
#pragma region Title
													+ SGridPanel::Slot(0,0)
													.HAlign(HAlign_Fill)
													[
														SNew(STextBlock)
														.Text(LOCTEXT("A", "DECORATORS"))
														.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
														.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor)
													]
#pragma endregion 

#pragma region Inherits
													+ SGridPanel::Slot(0,1)
													.HAlign(HAlign_Fill)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SBox)
														.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified)
														.HAlign(HAlign_Left)
														[
															SNew(SScaleBox)
															.Stretch(EStretch::ScaleToFit)
															[
																SNew(SHorizontalBox)
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.VAlign(VAlign_Center)
																[
																	SNew(SScaleBox)
																	.HAlign(HAlign_Left)
																	.VAlign(VAlign_Center)
																	.Stretch(EStretch::ScaleToFit)
																	[
																		SNew(SBox)
																		.MaxAspectRatio(FOptionalSize(1))
																		.MaxDesiredHeight(FOptionalSize(6.f))
																		.MaxDesiredWidth(FOptionalSize(6.f))
																		[
																			SNew(SImage)
																			.Image(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointImageBrush)
																			.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointsImagePointColor_Inherits)
																		]
																	]
																]

																+SHorizontalBox::Slot()
																[
																	SNew(SSpacer)
																	.Size(FVector2D(1.f, 0.f))
																]
	
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.AutoWidth()
																[
																	SNew(STextBlock)
																	.Text(LOCTEXT("B", "inherits"))
																	.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
																	.Justification(ETextJustify::Left)
																	.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor)
																]
															]
														]
													]
													+ SGridPanel::Slot(1,1)
													.HAlign(HAlign_Right)
													.VAlign(VAlign_Center)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SScaleBox)
														.Stretch(EStretch::ScaleToFit)
														.HAlign(HAlign_Center)
														[
															SNew(SBox)
															.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified)
															.MaxAspectRatio(FOptionalSize(1))
															.MaxDesiredHeight(FOptionalSize(12.f))
															.MaxDesiredWidth(FOptionalSize(12.f))
															[
																SNew(SImage)
																.Image(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageBrush)
																.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageTint)
															]
														]
													]
#pragma endregion
												]
											]
#pragma endregion 

#pragma region ImplementsOnly
											// IMPLEMENTS ONLY
											+ SVerticalBox::Slot()
											+ SVerticalBox::Slot()
											.AutoHeight()
											.HAlign(HAlign_Fill)
											.VAlign(VAlign_Fill)
											.Padding(FMargin(8.0f, 0.f, 8.0f, 0.f))
											[
												SNew(SBox)
												.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowImplementsOnlySlot_Unified)
												.MaxDesiredWidth(FOptionalSize(130.f))
												.HAlign(HAlign_Fill)
												[
													SNew(SGridPanel)
													.Visibility(EVisibility::HitTestInvisible)
													.FillColumn(0, 2.f)
													.FillColumn(1, 1.f)
#pragma region Title
													+ SGridPanel::Slot(0,0)
													.HAlign(HAlign_Fill)
													[
														SNew(STextBlock)
														.Text(LOCTEXT("A", "DECORATORS"))
														.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
														.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor)
													]
#pragma endregion 

#pragma region Implements
													+ SGridPanel::Slot(0,1)
													.HAlign(HAlign_Fill)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SBox)
														.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowImplementsOnlySlot_Unified)
														.HAlign(HAlign_Left)
														[
															SNew(SScaleBox)
															.Stretch(EStretch::ScaleToFit)
															[
																SNew(SHorizontalBox)
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.VAlign(VAlign_Center)
																[
																	SNew(SScaleBox)
																	.HAlign(HAlign_Left)
																	.VAlign(VAlign_Center)
																	.Stretch(EStretch::ScaleToFit)
																	[
																		SNew(SBox)
																		.MaxAspectRatio(FOptionalSize(1))
																		.MaxDesiredHeight(FOptionalSize(6.f))
																		.MaxDesiredWidth(FOptionalSize(6.f))
																		[
																			SNew(SImage)
																			.Image(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointImageBrush)
																			.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointsImagePointColor_Implements)
																		]
																	]
																]

																+SHorizontalBox::Slot()
																[
																	SNew(SSpacer)
																	.Size(FVector2D(1.f, 0.f))
																]
															
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.AutoWidth()
																[
																	SNew(STextBlock)
																	.Text(LOCTEXT("C", "implements"))
																	.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
																	.Justification(ETextJustify::Left)
																	.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetImplementsRowColor)
																]
															]
														]
													]
													+ SGridPanel::Slot(1,1)
													.HAlign(HAlign_Right)
													.VAlign(VAlign_Center)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SOverlay)
														+SOverlay::Slot()
														[
															SNew(SScaleBox)
															.Stretch(EStretch::ScaleToFit)
															.HAlign(HAlign_Center)
															[
																SNew(SBox)
																.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowImplementsOnlySlot_Unified)
																.MaxAspectRatio(FOptionalSize(1))
																.MaxDesiredHeight(FOptionalSize(12.f))
																.MaxDesiredWidth(FOptionalSize(12.f))
																.WidthOverride(12.f)
															]
														]
														
														+SOverlay::Slot()
														.HAlign(HAlign_Center)
														[
															SNew(STextBlock)
															.Text(this, &SEdNode_MounteaDialogueGraphNode::GetNumberOfDecorators)
															.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
															.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetImplementsRowColor)
															.Justification(ETextJustify::Center)
														]
													]
#pragma endregion 
												]
											]
#pragma endregion

#pragma region Both
											// BOTH
											+ SVerticalBox::Slot()
											.AutoHeight()
											.HAlign(HAlign_Fill)
											.VAlign(VAlign_Fill)
											.Padding(FMargin(8.0f, 0.f, 8.0f, 0.f))
											[
												SNew(SBox)
												.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
												.MaxDesiredWidth(FOptionalSize(130.f))
												.HAlign(HAlign_Fill)
												[
													SNew(SGridPanel)
													.Visibility(EVisibility::HitTestInvisible)
													.FillColumn(0, 2.f)
													.FillColumn(1, 1.f)
#pragma region Title
													+ SGridPanel::Slot(0,0)
													.HAlign(HAlign_Fill)
													[
														SNew(STextBlock)
														.Text(LOCTEXT("A", "DECORATORS"))
														.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
														.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor)
													]
#pragma endregion 

#pragma region Inherits
													+ SGridPanel::Slot(0,1)
													.HAlign(HAlign_Fill)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SBox)
														.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
														.HAlign(HAlign_Left)
														[
															SNew(SScaleBox)
															.Stretch(EStretch::ScaleToFit)
															[
																SNew(SHorizontalBox)
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.VAlign(VAlign_Center)
																[
																	SNew(SScaleBox)
																	.HAlign(HAlign_Left)
																	.VAlign(VAlign_Center)
																	.Stretch(EStretch::ScaleToFit)
																	[
																		SNew(SBox)
																		.MaxAspectRatio(FOptionalSize(1))
																		.MaxDesiredHeight(FOptionalSize(6.f))
																		.MaxDesiredWidth(FOptionalSize(6.f))
																		[
																			SNew(SImage)
																			.Image(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointImageBrush)
																			.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointsImagePointColor_Inherits)
																		]
																	]
																]

																+SHorizontalBox::Slot()
																[
																	SNew(SSpacer)
																	.Size(FVector2D(1.f, 0.f))
																]
	
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.AutoWidth()
																[
																	SNew(STextBlock)
																	.Text(LOCTEXT("B", "inherits"))
																	.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
																	.Justification(ETextJustify::Left)
																	.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetFontColor)
																]
															]
														]
													]
													+ SGridPanel::Slot(1,1)
													.HAlign(HAlign_Right)
													.VAlign(VAlign_Center)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SScaleBox)
														.Stretch(EStretch::ScaleToFit)
														.HAlign(HAlign_Center)
														[
															SNew(SBox)
															.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
															.MaxAspectRatio(FOptionalSize(1))
															.MaxDesiredHeight(FOptionalSize(12.f))
															.MaxDesiredWidth(FOptionalSize(12.f))
															[
																SNew(SImage)
																.Image(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageBrush)
																.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetInheritsImageTint)
															]
														]
													]
#pragma endregion

#pragma region Implements
													+ SGridPanel::Slot(0,2)
													.HAlign(HAlign_Fill)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SBox)
														.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
														.HAlign(HAlign_Left)
														[
															SNew(SScaleBox)
															.Stretch(EStretch::ScaleToFit)
															[
																SNew(SHorizontalBox)
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.VAlign(VAlign_Center)
																[
																	SNew(SScaleBox)
																	.HAlign(HAlign_Left)
																	.VAlign(VAlign_Center)
																	.Stretch(EStretch::ScaleToFit)
																	[
																		SNew(SBox)
																		.MaxAspectRatio(FOptionalSize(1))
																		.MaxDesiredHeight(FOptionalSize(6.f))
																		.MaxDesiredWidth(FOptionalSize(6.f))
																		[
																			SNew(SImage)
																			.Image(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointImageBrush)
																			.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetBulletPointsImagePointColor_Implements)
																		]
																	]
																]

																+SHorizontalBox::Slot()
																[
																	SNew(SSpacer)
																	.Size(FVector2D(1.f, 0.f))
																]
															
																+SHorizontalBox::Slot()
																.HAlign(HAlign_Fill)
																.AutoWidth()
																[
																	SNew(STextBlock)
																	.Text(LOCTEXT("C", "implements"))
																	.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
																	.Justification(ETextJustify::Left)
																	.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetImplementsRowColor)
																]
															]
														]
													]
													+ SGridPanel::Slot(1,2)
													.HAlign(HAlign_Right)
													.VAlign(VAlign_Center)
													.Padding(UnifiedRowsPadding)
													[
														SNew(SOverlay)
														+SOverlay::Slot()
														[
															SNew(SScaleBox)
															.Stretch(EStretch::ScaleToFit)
															.HAlign(HAlign_Center)
															[
																SNew(SBox)
																.Visibility(this, &SEdNode_MounteaDialogueGraphNode::ShowAllDecorators)
																.MaxAspectRatio(FOptionalSize(1))
																.MaxDesiredHeight(FOptionalSize(12.f))
																.MaxDesiredWidth(FOptionalSize(12.f))
																.WidthOverride(12.f)
															]
														]
														
														+SOverlay::Slot()
														.HAlign(HAlign_Center)
														[
															SNew(STextBlock)
															.Text(this, &SEdNode_MounteaDialogueGraphNode::GetNumberOfDecorators)
															.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
															.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetImplementsRowColor)
															.Justification(ETextJustify::Center)
														]
													]
#pragma endregion 
												]
											]
#pragma endregion
											+ SVerticalBox::Slot()
											.HAlign(HAlign_Center)
											.VAlign(VAlign_Fill)
											[
												SNew(SSpacer)
												.Size(FVector2D(0.f, 2.5f))
											]
										]
									]
#pragma endregion 
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

	return (!MounteaDialogueGraphNode->bCanRenameNode && !EdNode_Node->DialogueGraphNode->bCanRenameNode) || SGraphNode::IsNameReadOnly();
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
		MyNode->Modify(true);
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
			case ENodeCornerType::ENT_SoftCorners:
				return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.SoftEdges");
			case ENodeCornerType::ENT_HardCorners: 
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
		case ENodeCornerType::ENT_SoftCorners:
			return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
		case ENodeCornerType::ENT_HardCorners: 
			return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextHardEdges");
		}
	}

	return FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges");
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetFontColor() const
{
	return  ShouldUpdate() ? (IsNodeActive() ? MounteaDialogueGraphColors::TextColors::Normal : MounteaDialogueGraphColors::TextColors::Disabled) : MounteaDialogueGraphColors::TextColors::Normal;
}

bool SEdNode_MounteaDialogueGraphNode::ShouldUpdate() const
{
	if (!GEditor)
		return false;
		
	if (!GEditor->IsPlayingSessionInEditor())
		return false;

	UEdNode_MounteaDialogueGraphNode* MyNode = CastChecked<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	const auto dialogueGraphEditor = MyNode->GetDialogueGraphEdGraph();
	if (!dialogueGraphEditor)
		return false;

	auto focusedInstance = dialogueGraphEditor->FocusedInstance;
	if (focusedInstance.InstanceId == INDEX_NONE)
		return false;
		
	return true;
}

bool SEdNode_MounteaDialogueGraphNode::IsNodeActive() const
{
	UEdNode_MounteaDialogueGraphNode* MyNode = CastChecked<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	const auto dialogueGraphEditor = MyNode->GetDialogueGraphEdGraph();
	if (!dialogueGraphEditor)
		return false;

	auto focusedInstance = dialogueGraphEditor->FocusedInstance;
	if (focusedInstance.InstanceId == INDEX_NONE)
		return false;

	if (focusedInstance.Participant.GetObject() == nullptr)
		return false;

	auto dialogueManager = focusedInstance.Participant->GetDialogueManager();
	if (!dialogueManager.GetObject())
		return false;

	auto dialogueContext = dialogueManager->Execute_GetDialogueContext(dialogueManager.GetObject());
	if (!dialogueContext)
		return false;

	return dialogueContext->GetActiveNode() == MyNode->DialogueGraphNode;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBorderBackgroundColor() const
{
	UEdNode_MounteaDialogueGraphNode* MyNode = CastChecked<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	auto nodeBackgroundColor = MyNode ? MyNode->GetBackgroundColor() : MounteaDialogueGraphColors::NodeBorder::HighlightAbortRange0;
	
	return  ShouldUpdate() ? (IsNodeActive() ? nodeBackgroundColor : nodeBackgroundColor * MounteaDialogueGraphColors::NodeBorder::InactiveBorder) : nodeBackgroundColor;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBorderFrontColor() const
{
	auto selectedTheme = MounteaDialogueGraphColors::Overlay::DarkTheme;
	if (GraphEditorSettings)
	{
		switch (GraphEditorSettings->GetNodeTheme())
		{
			case ENodeTheme::ENT_DarkTheme:
				selectedTheme = MounteaDialogueGraphColors::Overlay::DarkTheme;
				break;
			case ENodeTheme::ENT_LightTheme:
				selectedTheme = MounteaDialogueGraphColors::Overlay::LightTheme;
				break;
		} 
	}
	
	return ShouldUpdate() ? (IsNodeActive() ? selectedTheme : selectedTheme * MounteaDialogueGraphColors::NodeBorder::InactiveBorder) : selectedTheme;
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
	return FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
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
		if (EdParentNode->DialogueGraphNode)
			bHasDecorators = EdParentNode->DialogueGraphNode->DoesInheritDecorators();
	
	const FSlateColor BaseColor = bHasDecorators ? 
		FSlateColor(FLinearColor::Green) : 
		FSlateColor(FLinearColor::Red);
	
	if (!ShouldUpdate())
		return BaseColor;
	
	return IsNodeActive() ? BaseColor : MounteaDialogueGraphColors::TextColors::Disabled;
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetBulletPointImageBrush() const
{
	return FMounteaDialogueGraphEditorStyle::GetBrush( "MDSStyleSet.Icon.BulletPoint" );
}

FText SEdNode_MounteaDialogueGraphNode::GetIndexOverlayTooltipText() const
{
	return LOCTEXT("NodeIndexTooltip", "Node index");
}

FText SEdNode_MounteaDialogueGraphNode::GetIndexText() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		if (const auto Node = EdParentNode->DialogueGraphNode)
		{
			if (const auto Graph = Node->Graph)
			{
				const int32 Index = Graph->AllNodes.Find(Node);
				return FText::AsNumber(Index);
			}
			
			return FText::AsNumber(EdParentNode->DialogueGraphNode->GetNodeIndex());
		}
	}
	return FText::AsNumber(INDEX_NONE);
}

EVisibility SEdNode_MounteaDialogueGraphNode::GetIndexSlotVisibility() const
{
	if (IsHovered())
	{
		return EVisibility::SelfHitTestInvisible;
	}

	return EVisibility::Collapsed;
}

FVector2D SEdNode_MounteaDialogueGraphNode::GetIndexSlotOffset() const
{
	if (IsHovered())
	{
		return FVector2D(-20.f);
	}

	return FVector2D(-15.f);
}

FVector2D SEdNode_MounteaDialogueGraphNode::GetIndexSlotSize() const
{
	if (IsHovered())
	{
		return FVector2D(24.f);
	}

	return FVector2D(12.f);
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
			for (const auto& Itr :  EdParentNode->DialogueGraphNode->Graph->GetGraphDecorators())
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

				for (const auto& Itr : EdParentNode->DialogueGraphNode->GetNodeDecorators())
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

EVisibility SEdNode_MounteaDialogueGraphNode::ShowImplementsOnlySlot_Unified() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->ShowDetailedInfo_NumDecorators() && !GraphEditorSettings->ShowDetailedInfo_InheritsDecorators())
		{
			return EVisibility::SelfHitTestInvisible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
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
			const int32 Number = EdParentNode->DialogueGraphNode->GetNodeDecorators().Num();
			if (Number <= 0) return FText::FromString(TEXT("-"));
			if (Number > 9) return FText::FromString(TEXT("9+"));
			
			return FText::FromString(FString::FromInt(Number));
		}
	}
	return FText::FromString("-");
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Unified() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->GetDecoratorsStyle() == EDecoratorsInfoStyle::EDIS_Unified)
		{
			if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators() && !GraphEditorSettings->ShowDetailedInfo_NumDecorators())
			{
				return EVisibility::SelfHitTestInvisible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
			}
		}
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowImplementsOnlySlot_Stack() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->GetDecoratorsStyle() == EDecoratorsInfoStyle::EDSI_Stack)
		{
			if (GraphEditorSettings->ShowDetailedInfo_NumDecorators())
			{
				return EVisibility::SelfHitTestInvisible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
			}
		}
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowInheritsDecoratorsSlot_Stack() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->GetDecoratorsStyle() == EDecoratorsInfoStyle::EDSI_Stack)
		{
			if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators())
			{
				return EVisibility::SelfHitTestInvisible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
			}
		}
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::ShowAllDecorators() const
{
	if (GraphEditorSettings)
	{
		if (GraphEditorSettings->GetDecoratorsStyle() == EDecoratorsInfoStyle::EDIS_Unified)
		{
			if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators() && GraphEditorSettings->ShowDetailedInfo_NumDecorators())
			{
				return EVisibility::SelfHitTestInvisible; //return HasGraphDecorators() ? EVisibility::Visible : EVisibility::Collapsed;
			}
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
			return EVisibility::SelfHitTestInvisible;
		}
	}
	return EVisibility::Collapsed;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetImplementsRowColor() const
{
	const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!EdParentNode || !EdParentNode->DialogueGraphNode)
		return GetFontColor();

	return EdParentNode->DialogueGraphNode->GetNodeDecorators().Num() == 0 
		? MounteaDialogueGraphColors::TextColors::Disabled 
		: GetFontColor();
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBulletPointsImagePointColor_Implements() const
{
	const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!EdParentNode || !EdParentNode->DialogueGraphNode)
		return MounteaDialogueGraphColors::BulletPointsColors::Normal;

	const bool bImplements = EdParentNode->DialogueGraphNode->GetNodeDecorators().Num() > 0;

	if (!ShouldUpdate())
		return bImplements
		? MounteaDialogueGraphColors::BulletPointsColors::Normal
		: MounteaDialogueGraphColors::BulletPointsColors::Disabled;

	return IsNodeActive() ? bImplements ? MounteaDialogueGraphColors::BulletPointsColors::Normal
		: MounteaDialogueGraphColors::BulletPointsColors::Disabled : MounteaDialogueGraphColors::BulletPointsColors::Disabled;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBulletPointsImagePointColor_Inherits() const
{
	const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!EdParentNode || !EdParentNode->DialogueGraphNode)
		return MounteaDialogueGraphColors::BulletPointsColors::Normal;

	const bool bInherits = EdParentNode->DialogueGraphNode->DoesInheritDecorators();

	if (!ShouldUpdate())
		return bInherits
		? MounteaDialogueGraphColors::BulletPointsColors::Normal
		: MounteaDialogueGraphColors::BulletPointsColors::Disabled;

	return IsNodeActive() ? bInherits ? MounteaDialogueGraphColors::BulletPointsColors::Normal
		: MounteaDialogueGraphColors::BulletPointsColors::Disabled : MounteaDialogueGraphColors::BulletPointsColors::Disabled;
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
	if (GetDecoratorsStyle() == EDecoratorsInfoStyle::EDSI_Stack )
	{
		if (GraphEditorSettings)
		{
			if (GraphEditorSettings->ShowDetailedInfo_InheritsDecorators() || GraphEditorSettings->ShowDetailedInfo_NumDecorators())
			{
				return EVisibility::SelfHitTestInvisible;
			}
			return EVisibility::Collapsed;
		}
		
		const auto TempSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();
		if (TempSettings) return GetDecoratorsStyle() == EDecoratorsInfoStyle::EDSI_Stack ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		{
			if (TempSettings->ShowDetailedInfo_InheritsDecorators() || TempSettings->ShowDetailedInfo_NumDecorators())
			{
				return EVisibility::SelfHitTestInvisible;
			}
			return EVisibility::Collapsed;
		}
		
	}
	return EVisibility::Collapsed;
}

EVisibility SEdNode_MounteaDialogueGraphNode::GetUnifiedVisibility() const
{
	return GetDecoratorsStyle() == EDecoratorsInfoStyle::EDIS_Unified ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

FText SEdNode_MounteaDialogueGraphNode::GetTooltipText() const
{
	if (const UEdNode_MounteaDialogueGraphNode* EdParentNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		return EdParentNode->GetTooltipText();
	}
	return LOCTEXT("SEdNode_MounteaDialogueGraphNode_Tooltip", "invalid node selected");
}

#undef LOCTEXT_NAMESPACE