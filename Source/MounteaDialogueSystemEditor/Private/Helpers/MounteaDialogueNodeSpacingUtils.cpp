
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

#include "Helpers/MounteaDialogueNodeSpacingUtils.h"

#include "Consts/MounteaDialogueEditorConsts.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Ed/SEdNode_MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode.h"

namespace MounteaDialogueNodeSpacingUtils
{
	namespace
	{
		constexpr float ExtraVerticalPadding = 10.0f;
		constexpr float FallbackNodeHeight = 150.0f;
	}

	float GetMinParentChildGap()
	{
		return 2.0f * MounteaDialogueWireConsts::WireStubOffset + ExtraVerticalPadding;
	}

	float GetNodeHeight(const UEdNode_MounteaDialogueGraphNode* Node)
	{
		if (!Node)
			return FallbackNodeHeight;

		if (Node->SEdNode)
		{
			const FVector2D cachedSize = Node->SEdNode->GetCachedGeometry().GetLocalSize();
			if (cachedSize.Y > KINDA_SMALL_NUMBER)
				return cachedSize.Y;

			const FVector2D desiredSize = Node->SEdNode->GetDesiredSize();
			if (desiredSize.Y > KINDA_SMALL_NUMBER)
				return desiredSize.Y;
		}

		return FallbackNodeHeight;
	}

	bool ComputeRequiredChildTopY(const UEdNode_MounteaDialogueGraphNode* ChildNode, float& OutRequiredTopY)
	{
		OutRequiredTopY = 0.0f;
		if (!ChildNode || !ChildNode->DialogueGraphNode)
			return false;

		UEdGraph_MounteaDialogueGraph* graph = ChildNode->GetDialogueGraphEdGraph();
		if (!graph)
			return false;

		bool bHasParent = false;
		float requiredTopY = -FLT_MAX;
		const float minGap = GetMinParentChildGap();

		for (UMounteaDialogueGraphNode* parentNode : ChildNode->DialogueGraphNode->ParentNodes)
		{
			if (!parentNode)
				continue;

			UEdNode_MounteaDialogueGraphNode* const* parentEdNodePtr = graph->NodeMap.Find(parentNode);
			UEdNode_MounteaDialogueGraphNode* parentEdNode = parentEdNodePtr ? *parentEdNodePtr : nullptr;
			if (!parentEdNode)
				continue;

			const float parentBottom = static_cast<float>(parentEdNode->NodePosY) + GetNodeHeight(parentEdNode);
			requiredTopY = FMath::Max(requiredTopY, parentBottom + minGap);
			bHasParent = true;
		}

		if (!bHasParent)
			return false;

		OutRequiredTopY = requiredTopY;
		return true;
	}

	void EnforceMinimumParentChildYSpacing(UEdGraph_MounteaDialogueGraph* Graph, int32 MaxPasses)
	{
		if (!Graph)
			return;

		const int32 clampedPasses = FMath::Clamp(MaxPasses, 1, 64);
		for (int32 pass = 0; pass < clampedPasses; ++pass)
		{
			bool bChanged = false;

			for (UEdGraphNode* graphNode : Graph->Nodes)
			{
				UEdNode_MounteaDialogueGraphNode* dialogueNode = Cast<UEdNode_MounteaDialogueGraphNode>(graphNode);
				if (!dialogueNode)
					continue;

				float requiredTopY = 0.0f;
				if (!ComputeRequiredChildTopY(dialogueNode, requiredTopY))
					continue;

				if (static_cast<float>(dialogueNode->NodePosY) < requiredTopY)
				{
					dialogueNode->NodePosY = FMath::CeilToInt(requiredTopY);
					bChanged = true;
				}
			}

			if (!bChanged)
				break;
		}
	}
}
