// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not 
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"

#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

namespace
{
	FLinearColor HexToLinearColor(const TCHAR* HexCode)
	{
		const FColor srgbColor = FColor::FromHex(HexCode);
		return FLinearColor::FromSRGBColor(srgbColor);
	}
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetCanvasBackground()
{
	return HexToLinearColor(TEXT("#121212"));
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetCardBackground()
{
	return HexToLinearColor(TEXT("#1f1f1f"));
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetCardForeground()
{
	return HexToLinearColor(TEXT("#f5f5f5"));
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetMutedForeground()
{
	return HexToLinearColor(TEXT("#a1a1a1"));
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetPrimaryAccent()
{
	return HexToLinearColor(TEXT("#f97316"));
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetSelectionRing()
{
	FLinearColor selectionColor = HexToLinearColor(TEXT("#f97316"));
	selectionColor.A = 0.20f;
	return selectionColor;
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetWireColor()
{
	return HexToLinearColor(TEXT("#ededed"));
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRing()
{
	FLinearColor selectionColor = HexToLinearColor(TEXT("#f97316"));
	selectionColor.A = 1.0f;
	return selectionColor;
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRingHovered()
{
	FLinearColor selectionColor = HexToLinearColor(TEXT("#fbbf24"));
	selectionColor.A = 1.0f;
	return selectionColor;
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetEdgeSelectionOverlay()
{
	FLinearColor overlayColor = HexToLinearColor(TEXT("#f97316"));
	overlayColor.A = 0.26f;
	return overlayColor;
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetEdgeSelectionOverlayHovered()
{
	FLinearColor overlayColor = HexToLinearColor(TEXT("#fb923c"));
	overlayColor.A = 0.34f;
	return overlayColor;
}

float FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRingInset()
{
	return -2.0f;
}

float FMounteaDialogueGraphVisualTokens::GetEdgeSelectionRingPadding()
{
	return 2.0f;
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetNodeAccentColor(const UMounteaDialogueGraphNode* Node)
{
	if (!Node)
		return GetPrimaryAccent();

#if WITH_EDITORONLY_DATA
	return Node->EditorNodeColour;
#else
	return GetPrimaryAccent();
#endif
}

FLinearColor FMounteaDialogueGraphVisualTokens::GetNodeHeaderForeground(const UMounteaDialogueGraphNode* Node)
{
	if (!Node)
		return FLinearColor::White;

#if WITH_EDITORONLY_DATA
	return Node->EditorHeaderForegroundColour;
#else
	return FLinearColor::White;
#endif
}

const FSlateBrush* FMounteaDialogueGraphVisualTokens::GetNodeIconBrush(const UMounteaDialogueGraphNode* Node)
{
	if (!Node)
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Default"));
	if (Node->IsA<UMounteaDialogueGraphNode_StartNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Start"));
	if (Node->IsA<UMounteaDialogueGraphNode_Delay>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Delay"));
	if (Node->IsA<UMounteaDialogueGraphNode_AnswerNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Answer"));
	if (Node->IsA<UMounteaDialogueGraphNode_LeadNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Lead"));
	if (Node->IsA<UMounteaDialogueGraphNode_CompleteNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Complete"));
	if (Node->IsA<UMounteaDialogueGraphNode_ReturnToNode>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Return"));
	if (Node->IsA<UMounteaDialogueGraphNode_OpenChildGraph>())
		return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.OpenChildGraph"));
	return FMounteaDialogueGraphEditorStyle::GetBrush(TEXT("MDSStyleSet.Node.Icon.Default"));
}

FVector2D FMounteaDialogueGraphVisualTokens::GetNodeSizeHint(const UMounteaDialogueGraphNode* Node)
{
	if (!Node)
		return FVector2D(372.f, 174.f);

	if (Node->IsA<UMounteaDialogueGraphNode_StartNode>())
		return FVector2D(308.f, 132.f);

	if (Node->IsA<UMounteaDialogueGraphNode_Delay>())
		return FVector2D(340.f, 146.f);

	if (Node->IsA<UMounteaDialogueGraphNode_ReturnToNode>())
		return FVector2D(332.f, 150.f);

	if (Node->IsA<UMounteaDialogueGraphNode_OpenChildGraph>())
		return FVector2D(332.f, 150.f);

	return FVector2D(372.f, 174.f);
}
