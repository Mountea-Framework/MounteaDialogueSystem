// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "SEdNode_MounteaDialogueGraphNode.h"

#include "MounteaDialogueGraphNode.h"
#include "Helpers/MounteaDialogueGraphColors.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"

#include "SLevelOfDetailBranchNode.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SCommentBubble.h"
#include "SlateOptMacros.h"
#include "SGraphPin.h"
#include "GraphEditorSettings.h"
#include "MounteaDialogueGraph.h"

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

		SBorder::Construct(SBorder::FArguments()
			.BorderImage(this, &SMounteaDialogueGraphPin::GetPinBorder)
			.BorderBackgroundColor(this, &SMounteaDialogueGraphPin::GetPinColor)
			.OnMouseButtonDown(this, &SMounteaDialogueGraphPin::OnPinMouseDown)
			.Cursor(this, &SMounteaDialogueGraphPin::GetPinCursor)
			.Padding(FMargin(10.0f))
		);
	}

protected:
	virtual FSlateColor GetPinColor() const override
	{
		return MounteaDialogueGraphColors::Pin::Default;
	}

	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		return SNew(STextBlock);
	}

	const FSlateBrush* GetPinBorder() const
	{
		return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Body"));
	}
};

#pragma endregion

void SEdNode_MounteaDialogueGraphNode::Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphNode* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
	InNode->SEdNode = this;
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

	// TODO: any way to disable areas?
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0.0f)
			.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetBorderBackgroundColor)
			[
				SNew(SOverlay)

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

					// STATE NAME AREA
					+ SVerticalBox::Slot()
					.Padding(FMargin(NodePadding.Left, 0.0f, NodePadding.Right, 0.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(NodeBody, SBorder)
							.BorderImage(FEditorStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
							.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetBackgroundColor)
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Center)
							.Visibility(EVisibility::SelfHitTestInvisible)
							[
								SNew(SOverlay)
								+ SOverlay::Slot()
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Fill)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.AutoHeight()
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
										[
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
											[
												SNew(SVerticalBox)
												+ SVerticalBox::Slot()
												.AutoHeight()
												[
													SAssignNew(InlineEditableText, SInlineEditableTextBlock)
													.Style(FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
													.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
													.OnVerifyTextChanged(this, &SEdNode_MounteaDialogueGraphNode::OnVerifyNameTextChanged)
													.OnTextCommitted(this, &SEdNode_MounteaDialogueGraphNode::OnNameTextCommited)
													.IsReadOnly(this, &SEdNode_MounteaDialogueGraphNode::IsNameReadOnly)
													.IsSelected(this, &SEdNode_MounteaDialogueGraphNode::IsSelectedExclusively)
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

void SEdNode_MounteaDialogueGraphNode::OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
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

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBorderBackgroundColor() const
{
	UEdNode_MounteaDialogueGraphNode* MyNode = CastChecked<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	return MyNode ? MyNode->GetBackgroundColor() : MounteaDialogueGraphColors::NodeBorder::HighlightAbortRange0;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetBackgroundColor() const
{
	return MounteaDialogueGraphColors::NodeBody::Default;
}

EVisibility SEdNode_MounteaDialogueGraphNode::GetDragOverMarkerVisibility() const
{
	return EVisibility::Visible;
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetNameIcon() const
{
	return FEditorStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
}

#undef LOCTEXT_NAMESPACE
