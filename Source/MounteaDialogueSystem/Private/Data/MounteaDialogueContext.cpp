// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueContext.h"


bool UMounteaDialogueContext::IsValid() const
{
	return ActiveNode != nullptr;
}

void UMounteaDialogueContext::SetDialogueContext(UMounteaDialogueGraphNode* NewActiveNode, const TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	ActiveNode = NewActiveNode;
	AllowedChildNodes = NewAllowedChildNodes;
}

void UMounteaDialogueContext::UpdateActiveDialogueNode(UMounteaDialogueGraphNode* NewActiveNode)
{
	ActiveNode = NewActiveNode;
}

void UMounteaDialogueContext::UpdateActiveDialogueRow(const FDialogueRow& NewActiveRow)
{
	ActiveDialogueRow = NewActiveRow;
}

void UMounteaDialogueContext::UpdateActiveDialogueRowDataIndex(const int32 NewIndex)
{
	ActiveDialogueRowDataIndex = NewIndex;
}
