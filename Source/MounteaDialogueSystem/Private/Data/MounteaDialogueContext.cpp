// All rights reserved Dominik Pavlicek 2023


#include "Data/MounteaDialogueContext.h"

bool UMounteaDialogueContext::IsValid() const
{
	return ActiveNode != nullptr;
}

void UMounteaDialogueContext::SetDialogueContext(UMounteaDialogueGraphNode* NewActiveNode, TArray<UMounteaDialogueGraphNode*> NewAllowedChildNodes)
{
	ActiveNode = NewActiveNode;
	AllowedChildNodes = NewAllowedChildNodes;
}
