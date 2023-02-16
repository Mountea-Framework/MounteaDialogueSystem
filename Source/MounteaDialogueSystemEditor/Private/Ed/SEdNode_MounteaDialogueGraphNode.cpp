// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "SEdNode_MounteaDialogueGraphNode.h"

#include "Nodes/MounteaDialogueGraphNode.h"
#include "Helpers/MounteaDialogueGraphColors.h"
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

	if (GraphEditorSettings)
	{
		switch (GraphEditorSettings->GetNodeTheme())
		{
		case ENodeTheme::ENT_DarkTheme:
			NodeInnerColor = MounteaDialogueGraphColors::Overlay::DarkTheme;
			PinsDockColor = MounteaDialogueGraphColors::Overlay::LightTheme;
			break;
		case ENodeTheme::ENT_LightTheme:
			NodeInnerColor = MounteaDialogueGraphColors::Overlay::LightTheme;
			PinsDockColor = MounteaDialogueGraphColors::Overlay::DarkTheme;
			break;
		default:
			NodeInnerColor = MounteaDialogueGraphColors::Overlay::DarkTheme;
			PinsDockColor = MounteaDialogueGraphColors::Overlay::LightTheme;
			break;
		}
	}
	else
	{
		GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();
		NodeInnerColor = MounteaDialogueGraphColors::Overlay::DarkTheme;
		PinsDockColor = MounteaDialogueGraphColors::Overlay::LightTheme;
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SEdNode_MounteaDialogueGraphNode::UpdateGraphNode()
{
	const FMargin NodePadding = FMargin(2.0f);

	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	OutputPinBox.Reset();

	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
		
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBox)
			//.MinDesiredHeight(FOptionalSize(50.f))
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
												
						// STATE NAME AREA
						+ SVerticalBox::Slot()
						.Padding(FMargin(NodePadding.Left, 0.0f, NodePadding.Right, 0.0f))
						.VAlign(VAlign_Fill)
						[
							SNew(SBox)
							.MinDesiredHeight(FOptionalSize(30.f))
							.VAlign(VAlign_Fill)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(NodeBody, SBorder)
									//.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetNodeTypeBrush)
									.BorderImage(this, &SEdNode_MounteaDialogueGraphNode::GetTextNodeTypeBrush)
									.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetBackgroundColor)
									.HAlign(HAlign_Fill)
									.VAlign(VAlign_Center)
									.Visibility(EVisibility::SelfHitTestInvisible)
									[
										SNew(SBox)
										.MinDesiredWidth(FOptionalSize(110.f))
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
												[
													SNew(SHorizontalBox)
													+ SHorizontalBox::Slot()
													.AutoWidth()
													[
														// POPUP ERROR MESSAGE
														SAssignNew(ErrorText, SErrorText)
														.BackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetErrorColor)
														.ToolTipText(this, &SEdNode_MounteaDialogueGraphNode::GetErrorMsgToolTip)
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
																.OnVerifyTextChanged(this, &SEdNode_MounteaDialogueGraphNode::OnVerifyNameTextChanged)
																.OnTextCommitted(this, &SEdNode_MounteaDialogueGraphNode::OnNameTextCommitted)
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

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBackgroundColor() const
{
	return MounteaDialogueGraphColors::NodeBody::Default;
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

// TODO: Make functions to generate Slate elements so I can stack them together
TSharedRef<SVerticalBox> SEdNode_MounteaDialogueGraphNode::MakeTextBox(const FText& InText)
{
	return

	SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.HAlign(HAlign_Center)
	.AutoHeight()
	[
		SAssignNew(InlineEditableText, SInlineEditableTextBlock)
		.Style(FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
		.Text(InText)
		.OnVerifyTextChanged(this, &SEdNode_MounteaDialogueGraphNode::OnVerifyNameTextChanged)
		.OnTextCommitted(this, &SEdNode_MounteaDialogueGraphNode::OnNameTextCommitted)
		.IsReadOnly(this, &SEdNode_MounteaDialogueGraphNode::IsNameReadOnly)
		.IsSelected(this, &SEdNode_MounteaDialogueGraphNode::IsSelectedExclusively)
		.Justification(ETextJustify::Center)
	]
	;
}

#undef LOCTEXT_NAMESPACE
