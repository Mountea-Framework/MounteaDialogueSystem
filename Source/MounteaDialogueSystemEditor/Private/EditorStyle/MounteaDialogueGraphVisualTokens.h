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

#pragma once

#include "CoreMinimal.h"

class UMounteaDialogueGraphNode;

struct FMounteaDialogueGraphVisualTokens
{
	static FLinearColor GetCanvasBackground();
	static FLinearColor GetCardBackground();
	static FLinearColor GetCardForeground();
	static FLinearColor GetMutedForeground();
	static FLinearColor GetPrimaryAccent();
	static FLinearColor GetSelectionRing();
	static FLinearColor GetWireColor();
	static FLinearColor GetEdgeSelectionRing();
	static FLinearColor GetEdgeSelectionRingHovered();
	static FLinearColor GetEdgeSelectionOverlay();
	static FLinearColor GetEdgeSelectionOverlayHovered();
	static float GetEdgeSelectionRingInset();
	static float GetEdgeSelectionRingPadding();

	static FLinearColor GetNodeAccentColor(const UMounteaDialogueGraphNode* Node);
	static FLinearColor GetNodeHeaderForeground(const UMounteaDialogueGraphNode* Node);
	static const FSlateBrush* GetNodeIconBrush(const UMounteaDialogueGraphNode* Node);

	static FVector2f GetNodeSizeHint(const UMounteaDialogueGraphNode* Node);
};
