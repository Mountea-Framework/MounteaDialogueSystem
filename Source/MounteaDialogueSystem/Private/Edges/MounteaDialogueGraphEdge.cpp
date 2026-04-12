// All rights reserved Dominik Pavlicek 2023


#include "Edges/MounteaDialogueGraphEdge.h"

UMounteaDialogueGraphEdge::UMounteaDialogueGraphEdge()
{
}

UMounteaDialogueGraph* UMounteaDialogueGraphEdge::GetGraph() const
{
	return Graph;
}

FMounteaDialogueEdgeConditions UMounteaDialogueGraphEdge::GetEdgeConditions() const
{
	return EdgeConditions;
}
