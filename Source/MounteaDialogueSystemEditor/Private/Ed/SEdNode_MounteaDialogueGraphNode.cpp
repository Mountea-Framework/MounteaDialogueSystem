// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "Ed/SEdNode_MounteaDialogueGraphNode.h"

#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Ed/SMounteaDialogueGraphPin.h"
#include "Editor/EditorEngine.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Helpers/MounteaDialogueNodeSpacingUtils.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
#include "ScopedTransaction.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SEdNode_MounteaDialogueGraphNode"

namespace MDSNodeLayoutTokens
{
	constexpr int32 PreviewCharacterLimit = 50;
	constexpr int32 TagCharacterLimit = 40;
	constexpr float HeaderTextWrapAt = 320.f;
	constexpr float PreviewWrapAt = 336.f;
	constexpr float TextLineHeightPercentage = 1.3333334f;
	constexpr float ConnectorEdgeOffset = 5.0f;
	constexpr float CardInnerBorder = 2.0f;
	constexpr float CardOuterRing = 2.0f;
	constexpr float HeaderHorizontalPadding = 14.0f;
	constexpr float HeaderVerticalPadding = 9.0f;
	constexpr float BodyHorizontalPadding = 14.0f;
	constexpr float BodyTopPadding = 12.0f;
	constexpr float BodyBottomPadding = 10.0f;
	constexpr float BodyContainerBottomPadding = 10.0f;
	constexpr float SeparatorBottomPadding = 7.0f;
	constexpr float ChipRowBottomPadding = 10.0f;
	constexpr float ChipHorizontalSpacing = 6.0f;
	constexpr float ChipVerticalSpacing = 5.0f;
	constexpr float ChipHorizontalPaddingLeft = 8.0f;
	constexpr float ChipHorizontalPaddingRight = 10.0f;
	constexpr float ChipVerticalPadding = 3.0f;
}

namespace
{
	FText GetLastTagSegmentText(const FGameplayTag& Tag)
	{
		if (!Tag.IsValid())
			return FText::GetEmpty();

		const FString fullTagName = Tag.ToString();
		if (fullTagName.IsEmpty())
			return FText::GetEmpty();

		FString lastSegment;
		if (fullTagName.Split(TEXT("."), nullptr, &lastSegment, ESearchCase::IgnoreCase, ESearchDir::FromEnd) && !lastSegment.IsEmpty())
			return FText::FromString(lastSegment);

		return FText::FromString(fullTagName);
	}

	bool TryGetDialogueRow(const UEdNode_MounteaDialogueGraphNode* EdNode, FDialogueRow& OutRow)
	{
		if (!EdNode || !EdNode->DialogueGraphNode)
			return false;

		const FDialogueRow dialogueRow = UMounteaDialogueSystemBFC::GetDialogueRow(EdNode->DialogueGraphNode);
		if (!UMounteaDialogueSystemBFC::IsDialogueRowValid(dialogueRow))
			return false;

		OutRow = dialogueRow;
		return true;
	}

	FText TruncatePreview(const FText& SourceText)
	{
		const FString sourceString = SourceText.ToString();
		if (sourceString.Len() <= MDSNodeLayoutTokens::PreviewCharacterLimit)
			return SourceText;

		const int32 maxBaseLength = MDSNodeLayoutTokens::PreviewCharacterLimit - 3;
		const FString truncated = sourceString.Left(FMath::Max(0, maxBaseLength)).Append(TEXT("..."));
		return FText::FromString(truncated);
	}

	FText TruncateTag(const FText& SourceText)
	{
		const FString sourceString = SourceText.ToString().TrimStartAndEnd();
		if (sourceString.Len() <= MDSNodeLayoutTokens::TagCharacterLimit)
			return FText::FromString(sourceString);

		const int32 maxBaseLength = MDSNodeLayoutTokens::TagCharacterLimit - 3;
		const FString truncated = sourceString.Left(FMath::Max(0, maxBaseLength)).Append(TEXT("..."));
		return FText::FromString(truncated);
	}
}

void SEdNode_MounteaDialogueGraphNode::Construct(const FArguments& InArgs, UEdNode_MounteaDialogueGraphNode* InNode)
{
	GraphNode = InNode;
	GraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();
	UpdateGraphNode();
	InNode->SEdNode = this;
}

void SEdNode_MounteaDialogueGraphNode::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (GEditor && !GEditor->IsPlayingSessionInEditor())
		SetToolTipText(GetTooltipText());

	SGraphNode::OnMouseEnter(MyGeometry, MouseEvent);
}

void SEdNode_MounteaDialogueGraphNode::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SGraphNode::OnMouseLeave(MouseEvent);
}

void SEdNode_MounteaDialogueGraphNode::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
{
	FVector2D adjustedPosition = NewPosition;

	const UEdNode_MounteaDialogueGraphNode* dialogueNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	float requiredTopY = 0.0f;
	if (dialogueNode && MounteaDialogueNodeSpacingUtils::ComputeRequiredChildTopY(dialogueNode, requiredTopY))
		adjustedPosition.Y = FMath::Max(adjustedPosition.Y, requiredTopY);

	SGraphNode::MoveTo(adjustedPosition, NodeFilter, bMarkDirty);
}

void SEdNode_MounteaDialogueGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	InputPinBox.Reset();
	OutputPinBox.Reset();

	SetupErrorReporting();
	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	const FVector2D nodeSizeHint = GetNodeSizeHint();

	GetOrAddSlot(ENodeZone::Center)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Flat.RoundAll")))
			.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetSelectionRingColor)
			.Padding(FMargin(MDSNodeLayoutTokens::CardOuterRing))
		]
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.WidthOverride(nodeSizeHint.X)
			.MinDesiredHeight(nodeSizeHint.Y)
			[
				SNew(SBorder)
				.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Flat.RoundAll")))
				.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetCardBorderColor)
				.Padding(FMargin(MDSNodeLayoutTokens::CardInnerBorder))
				[
					SAssignNew(NodeBody, SBorder)
					.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Flat.RoundAll")))
					.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetCardBackgroundColor)
					.Padding(FMargin(0.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							CreateHeaderWidget()
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							CreateBodyWidget()
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.ZOrder(10)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, -MDSNodeLayoutTokens::ConnectorEdgeOffset, 0.0f, 0.0f))
		[
			SAssignNew(InputPinBox, SVerticalBox)
		]
		+ SOverlay::Slot()
		.ZOrder(10)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, -MDSNodeLayoutTokens::ConnectorEdgeOffset))
		[
			SAssignNew(OutputPinBox, SVerticalBox)
		]
	];

	CreatePinWidgets();
}

void SEdNode_MounteaDialogueGraphNode::CreatePinWidgets()
{
	for (UEdGraphPin* pin : GraphNode->Pins)
	{
		if (!pin)
			continue;

		TSharedPtr<SGraphPin> newPin = SNew(SMounteaDialogueGraphPin, pin).AccentColor(GetAccentColor());
		AddPin(newPin.ToSharedRef());
	}
}

void SEdNode_MounteaDialogueGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	if (PinToAdd->GetDirection() == EGPD_Input)
	{
		if (InputPinBox.IsValid())
		{
			InputPinBox->AddSlot()
			.HAlign(HAlign_Center)
			.AutoHeight()
			.Padding(FMargin(0.0f))
			[
				PinToAdd
			];
		}
		InputPins.Add(PinToAdd);
	}
	else
	{
		if (OutputPinBox.IsValid())
		{
			OutputPinBox->AddSlot()
			.HAlign(HAlign_Center)
			.AutoHeight()
			.Padding(FMargin(0.0f))
			[
				PinToAdd
			];
		}
		OutputPins.Add(PinToAdd);
	}
}

bool SEdNode_MounteaDialogueGraphNode::IsNameReadOnly() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!edNode || !edNode->DialogueGraphNode || !edNode->DialogueGraphNode->Graph)
		return true;

	const bool canRenameGraph = edNode->DialogueGraphNode->Graph->bCanRenameNode;
	const bool canRenameNode = edNode->DialogueGraphNode->bCanRenameNode;
	const bool canRenameFromSettings = !GraphEditorSettings || GraphEditorSettings->AllowRenameNodes();

	return !(canRenameGraph || canRenameNode) || !canRenameFromSettings || SGraphNode::IsNameReadOnly();
}

void SEdNode_MounteaDialogueGraphNode::OnNameTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
{
	if (InText.IsEmpty())
		return;

	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	UEdNode_MounteaDialogueGraphNode* myNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!myNode || !myNode->DialogueGraphNode)
		return;

	const FScopedTransaction transaction(LOCTEXT("MounteaDialogueGraphEditorRenameNode", "Mountea Dialogue Editor: Rename Node"));
	myNode->Modify(true);
	myNode->DialogueGraphNode->Modify();
	myNode->DialogueGraphNode->SetNodeTitle(InText);
	UpdateGraphNode();
}

FText SEdNode_MounteaDialogueGraphNode::GetNodeTitle() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	return edNode ? edNode->GetNodeTitle(ENodeTitleType::ListView) : FText::GetEmpty();
}

FText SEdNode_MounteaDialogueGraphNode::GetPreviewText() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!edNode || !edNode->DialogueGraphNode)
		return LOCTEXT("PreviewFallback", "No preview available.");

	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_StartNode>())
		return LOCTEXT("StartNodeEntryPointPreview", "Dialogue Entry Point");

	if (const UMounteaDialogueGraphNode_Delay* delayNode = Cast<UMounteaDialogueGraphNode_Delay>(edNode->DialogueGraphNode))
	{
		return FText::Format(
			LOCTEXT("DelayNodeWaitPreview", "Wait for {0} seconds"),
			FText::AsNumber(delayNode->GetDelayDuration())
		);
	}

	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_OpenChildGraph>())
		return LOCTEXT("OpenChildGraphPreview", "Open linked child dialogue");

	FDialogueRow dialogueRow;
	if (TryGetDialogueRow(edNode, dialogueRow))
	{
		const TArray<FDialogueRowData> dialogueRows = dialogueRow.RowData;
		for (const FDialogueRowData& dialogueRowData : dialogueRows)
		{
			if (!dialogueRowData.RowText.IsEmpty())
				return TruncatePreview(dialogueRowData.RowText);
		}

		if (!dialogueRow.RowTitle.IsEmpty())
			return TruncatePreview(dialogueRow.RowTitle);
	}

	return TruncatePreview(edNode->DialogueGraphNode->GetDescription());
}

int32 SEdNode_MounteaDialogueGraphNode::GetDialogueRowCount() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	FDialogueRow dialogueRow;
	if (!TryGetDialogueRow(edNode, dialogueRow))
		return 0;

	return dialogueRow.RowData.Num();
}

FText SEdNode_MounteaDialogueGraphNode::GetRowsMetaText() const
{
	const int32 rowCount = GetDialogueRowCount();
	if (rowCount <= 1)
		return FText::GetEmpty();

	if (rowCount == 2)
		return LOCTEXT("TwoRowsMeta", "2 row");

	return FText::Format(LOCTEXT("RowsMeta", "{0} rows"), FText::AsNumber(rowCount));
}

TArray<FText> SEdNode_MounteaDialogueGraphNode::GetParticipantChipTexts() const
{
	TArray<FText> chips;

	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!edNode || !edNode->DialogueGraphNode)
		return chips;

	FDialogueRow dialogueRow;
	if (TryGetDialogueRow(edNode, dialogueRow))
	{
		TArray<FGameplayTag> compatibleTags;
		dialogueRow.CompatibleTags.GetGameplayTagArray(compatibleTags);

		TSet<FString> uniqueKeys;
		for (const FGameplayTag& compatibleTag : compatibleTags)
		{
			const FText tagText = GetLastTagSegmentText(compatibleTag);
			const FString key = tagText.ToString().TrimStartAndEnd();
			if (!key.IsEmpty() && !uniqueKeys.Contains(key))
			{
				uniqueKeys.Add(key);
				chips.Add(FText::FromString(key));
			}
		}

		if (chips.Num() > 0)
			return chips;

		if (!dialogueRow.DialogueParticipant.IsEmpty())
		{
			chips.Add(dialogueRow.DialogueParticipant);
			return chips;
		}
	}

	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_LeadNode>())
	{
		chips.Add(LOCTEXT("NpcParticipantFallback", "NPC"));
		return chips;
	}

	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_AnswerNode>())
	{
		chips.Add(LOCTEXT("PlayerParticipantFallback", "Player"));
		return chips;
	}

	return chips;
}

TArray<FText> SEdNode_MounteaDialogueGraphNode::GetDecoratorChipTexts() const
{
	TArray<FText> chips;

	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!edNode || !edNode->DialogueGraphNode)
		return chips;

	const TArray<FMounteaDialogueDecorator> decorators = edNode->DialogueGraphNode->GetNodeDecorators();
	for (const FMounteaDialogueDecorator& decorator : decorators)
	{
		if (IsValid(decorator.DecoratorType))
		{
			const FText decoratorName = decorator.DecoratorType->GetDecoratorName();
			FText decoratorDisplayName = decoratorName;
			const FString decoratorRawName = decoratorDisplayName.ToString().TrimStartAndEnd();
			const FString objectName = decorator.DecoratorType->GetName();

			if (decoratorRawName.IsEmpty() || decoratorRawName.Equals(objectName, ESearchCase::CaseSensitive))
				decoratorDisplayName = decorator.DecoratorType->GetClass()->GetDisplayNameText();

			if (decoratorDisplayName.IsEmpty())
				decoratorDisplayName = FText::FromString(FName::NameToDisplayString(decorator.DecoratorType->GetClass()->GetName(), false));

			if (!decoratorDisplayName.IsEmpty())
				chips.Add(decoratorDisplayName);
		}
	}

	return chips;
}

FText SEdNode_MounteaDialogueGraphNode::GetTooltipText() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	return edNode ? edNode->GetTooltipText() : LOCTEXT("NodeTooltipFallback", "Invalid node selected.");
}

EVisibility SEdNode_MounteaDialogueGraphNode::GetDragOverMarkerVisibility() const
{
	return EVisibility::Visible;
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetNameIcon() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!edNode || !edNode->DialogueGraphNode)
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Small"));

	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_StartNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Start"));
	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_Delay>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Delay"));
	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_AnswerNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Answer"));
	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_LeadNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Lead"));
	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_CompleteNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Complete"));
	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_ReturnToNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Return"));
	if (edNode->DialogueGraphNode->IsA<UMounteaDialogueGraphNode_OpenChildGraph>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.OpenChildGraph"));

	return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Small"));
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetParticipantIcon() const
{
	return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Chip.ParticipantIcon"));
}

const FSlateBrush* SEdNode_MounteaDialogueGraphNode::GetDecoratorIcon() const
{
	return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Chip.DecoratorIcon"));
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetHeaderTextColor() const
{
	return FLinearColor::White;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetHeaderIconColor() const
{
	return FLinearColor::White;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetSelectionRingColor() const
{
	if (IsSelectedExclusively() || (ShouldUpdate() && IsNodeActive()))
		return FMounteaDialogueGraphVisualTokens::GetSelectionRing();

	return FLinearColor::Transparent;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetCardBorderColor() const
{
	if (IsSelectedExclusively() || (ShouldUpdate() && IsNodeActive()))
		return FMounteaDialogueGraphVisualTokens::GetPrimaryAccent();

	return GetAccentColor();
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetCardBackgroundColor() const
{
	return FMounteaDialogueGraphVisualTokens::GetCardBackground();
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetParticipantChipBackgroundColor() const
{
	FLinearColor accentColor = GetAccentColor();
	accentColor.A = 0.28f;
	return accentColor;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetParticipantChipForegroundColor() const
{
	FLinearColor accentColor = GetAccentColor();
	accentColor.A = 1.0f;
	return accentColor;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetDecoratorChipBackgroundColor() const
{
	FLinearColor accentColor = GetAccentColor();
	accentColor.A = 0.28f;
	return accentColor;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetDecoratorChipForegroundColor() const
{
	FLinearColor accentColor = GetAccentColor();
	accentColor.A = 1.0f;
	return accentColor;
}

FSlateColor SEdNode_MounteaDialogueGraphNode::GetSeparatorColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, 0.09f);
}

bool SEdNode_MounteaDialogueGraphNode::HasParticipantChip() const
{
	return GetParticipantChipTexts().Num() > 0;
}

bool SEdNode_MounteaDialogueGraphNode::HasDecoratorChips() const
{
	return GetDecoratorChipTexts().Num() > 0;
}

bool SEdNode_MounteaDialogueGraphNode::ShouldUpdate() const
{
	if (!GEditor || !GEditor->IsPlayingSessionInEditor())
		return false;

	const UEdNode_MounteaDialogueGraphNode* myNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!myNode)
		return false;

	const UEdGraph_MounteaDialogueGraph* dialogueGraphEditor = myNode->GetDialogueGraphEdGraph();
	if (!dialogueGraphEditor)
		return false;

	const FPIEInstanceData focusedInstance = dialogueGraphEditor->FocusedInstance;
	return focusedInstance.InstanceId != INDEX_NONE;
}

bool SEdNode_MounteaDialogueGraphNode::IsNodeActive() const
{
	const UEdNode_MounteaDialogueGraphNode* myNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!myNode)
		return false;

	const UEdGraph_MounteaDialogueGraph* dialogueGraphEditor = myNode->GetDialogueGraphEdGraph();
	if (!dialogueGraphEditor)
		return false;

	const FPIEInstanceData focusedInstance = dialogueGraphEditor->FocusedInstance;
	if (focusedInstance.InstanceId == INDEX_NONE || !focusedInstance.Participant.GetObject())
		return false;

	const TScriptInterface<IMounteaDialogueManagerInterface> dialogueManager = focusedInstance.Participant->GetDialogueManager();
	if (!dialogueManager.GetObject())
		return false;

	UMounteaDialogueContext* dialogueContext = dialogueManager->Execute_GetDialogueContext(dialogueManager.GetObject());
	return dialogueContext && dialogueContext->GetActiveNode() == myNode->DialogueGraphNode;
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
		.WrapTextAt(MDSNodeLayoutTokens::HeaderTextWrapAt)
		.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetHeaderTextColor);
}

TSharedRef<SWidget> SEdNode_MounteaDialogueGraphNode::CreateHeaderWidget()
{
	const FText rowsMetaText = GetRowsMetaText();
	const bool hasRowsMeta = !rowsMetaText.IsEmpty();

	TSharedRef<SHorizontalBox> headerRow = SNew(SHorizontalBox);
	headerRow->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
	[
		SNew(SImage)
		.Image(GetNameIcon())
		.ColorAndOpacity(FLinearColor::White)
	];

	headerRow->AddSlot()
	.FillWidth(1.0f)
	.VAlign(VAlign_Center)
	[
		CreateNameSlotWidget()
	];

	if (hasRowsMeta)
	{
		headerRow->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
		[
			SNew(STextBlock)
			.Text(rowsMetaText)
			.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTitle.Small")
			.ColorAndOpacity(this, &SEdNode_MounteaDialogueGraphNode::GetHeaderTextColor)
			.LineHeightPercentage(MDSNodeLayoutTokens::TextLineHeightPercentage)
		];
	}

	return SNew(SBorder)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Flat.RoundTop")))
		.BorderBackgroundColor(GetAccentColor())
		.Padding(FMargin(MDSNodeLayoutTokens::HeaderHorizontalPadding, MDSNodeLayoutTokens::HeaderVerticalPadding))
		[
			headerRow
		];
}

TSharedRef<SWidget> SEdNode_MounteaDialogueGraphNode::CreateBodyWidget()
{
	TSharedRef<SVerticalBox> bodyColumn = SNew(SVerticalBox);
	const bool hasParticipantChip = HasParticipantChip();
	const bool hasDecoratorChips = HasDecoratorChips();

	bodyColumn->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.0f, 0.0f, 0.0f, MDSNodeLayoutTokens::BodyBottomPadding))
	[
		SNew(STextBlock)
		.Text(this, &SEdNode_MounteaDialogueGraphNode::GetPreviewText)
		.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeBody")
		.ColorAndOpacity(FMounteaDialogueGraphVisualTokens::GetCardForeground())
		.AutoWrapText(true)
		.WrapTextAt(MDSNodeLayoutTokens::PreviewWrapAt)
		.LineHeightPercentage(MDSNodeLayoutTokens::TextLineHeightPercentage)
	];

	if (hasParticipantChip || hasDecoratorChips)
	{
		bodyColumn->AddSlot()
		.FillHeight(1.0f)
		[
			SNew(SBox)
		];
	}

	if (hasParticipantChip)
	{
		bodyColumn->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, MDSNodeLayoutTokens::SeparatorBottomPadding))
		[
			SNew(SBox)
			.HeightOverride(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetSeparatorColor)
			]
		];

		bodyColumn->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, MDSNodeLayoutTokens::ChipRowBottomPadding))
		[
			CreateParticipantChipsWidget()
		];
	}

	if (hasDecoratorChips)
	{
		bodyColumn->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, MDSNodeLayoutTokens::SeparatorBottomPadding))
		[
			SNew(SBox)
			.HeightOverride(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush")))
				.BorderBackgroundColor(this, &SEdNode_MounteaDialogueGraphNode::GetSeparatorColor)
			]
		];

		bodyColumn->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, MDSNodeLayoutTokens::ChipRowBottomPadding))
		[
			CreateDecoratorChipsWidget()
		];
	}

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
		.Padding(FMargin(
			MDSNodeLayoutTokens::BodyHorizontalPadding,
			MDSNodeLayoutTokens::BodyTopPadding,
			MDSNodeLayoutTokens::BodyHorizontalPadding,
			MDSNodeLayoutTokens::BodyContainerBottomPadding))
		[
			bodyColumn
		];
}

TSharedRef<SWidget> SEdNode_MounteaDialogueGraphNode::CreateParticipantChipsWidget()
{
	return CreateTagChipsWidget(
		GetParticipantChipTexts(),
		GetParticipantIcon(),
		GetParticipantChipForegroundColor(),
		GetParticipantChipBackgroundColor()
	);
}

TSharedRef<SWidget> SEdNode_MounteaDialogueGraphNode::CreateDecoratorChipsWidget()
{
	return CreateTagChipsWidget(
		GetDecoratorChipTexts(),
		GetDecoratorIcon(),
		GetDecoratorChipForegroundColor(),
		GetDecoratorChipBackgroundColor()
	);
}

TSharedRef<SWidget> SEdNode_MounteaDialogueGraphNode::CreateTagChipsWidget(const TArray<FText>& ChipTexts, const FSlateBrush* ChipIcon, const FSlateColor& ChipForegroundColor, const FSlateColor& ChipBackgroundColor) const
{
	const float tagWrapWidth = FMath::Max(140.0f, GetNodeSizeHint().X - 24.0f);
	TSharedRef<SWrapBox> chipsWrap = SNew(SWrapBox)
		.UseAllottedSize(true)
		.InnerSlotPadding(FVector2D(MDSNodeLayoutTokens::ChipHorizontalSpacing, MDSNodeLayoutTokens::ChipVerticalSpacing));

	for (const FText& chipText : ChipTexts)
	{
		chipsWrap->AddSlot()
		[
			CreateTagChip(
				chipText,
				ChipIcon,
				ChipForegroundColor,
				ChipBackgroundColor
			)
		];
	}

	return SNew(SBox)
		.WidthOverride(tagWrapWidth)
		[
			chipsWrap
		];
}

TSharedRef<SWidget> SEdNode_MounteaDialogueGraphNode::CreateTagChip(const FText& ChipText, const FSlateBrush* ChipIcon, const FSlateColor& ChipForegroundColor, const FSlateColor& ChipBackgroundColor) const
{
	const FText truncatedChipText = TruncateTag(ChipText);
	FLinearColor chipFillColor = ChipBackgroundColor.GetSpecifiedColor();
	chipFillColor.A = 0.15f;

	return SNew(SBorder)
		.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Flat.Tag")))
		.BorderBackgroundColor(chipFillColor)
		.Padding(FMargin(
			MDSNodeLayoutTokens::ChipHorizontalPaddingLeft,
			MDSNodeLayoutTokens::ChipVerticalPadding,
			MDSNodeLayoutTokens::ChipHorizontalPaddingRight,
			MDSNodeLayoutTokens::ChipVerticalPadding))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SImage)
				.Image(ChipIcon)
				.ColorAndOpacity(ChipForegroundColor)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(truncatedChipText)
				.TextStyle(FMounteaDialogueGraphEditorStyle::Get(), "MDSStyleSet.NodeTag")
				.ColorAndOpacity(ChipForegroundColor)
			]
		];
}

FLinearColor SEdNode_MounteaDialogueGraphNode::GetAccentColor() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!edNode || !edNode->DialogueGraphNode)
		return FMounteaDialogueGraphVisualTokens::GetPrimaryAccent();

	return FMounteaDialogueGraphVisualTokens::GetNodeAccentColor(edNode->DialogueGraphNode);
}

FVector2D SEdNode_MounteaDialogueGraphNode::GetNodeSizeHint() const
{
	const UEdNode_MounteaDialogueGraphNode* edNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode);
	if (!edNode || !edNode->DialogueGraphNode)
		return FVector2D(250.0f, 124.0f);

	return FMounteaDialogueGraphVisualTokens::GetNodeSizeHint(edNode->DialogueGraphNode);
}

#undef LOCTEXT_NAMESPACE
