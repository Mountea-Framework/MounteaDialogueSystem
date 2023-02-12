// All rights reserved Dominik Pavlicek 2023


#include "EdNode_MounteaDialogueGraphEdge.h"

#include "EdNode_MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"

void UEdNode_MounteaDialogueGraphEdge::SetEdge(UMounteaDialogueGraphEdge* Edge)
{
	MounteaDialogueGraphEdge = Edge;
}

void UEdNode_MounteaDialogueGraphEdge::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
	Outputs->bHidden = true;
}

FText UEdNode_MounteaDialogueGraphEdge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	//return Super::GetNodeTitle(TitleType);
	return FText();
}

void UEdNode_MounteaDialogueGraphEdge::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() == 0)
	{
		// Commit suicide; transitions must always have an input and output connection
		Modify();

		// Our parent graph will have our graph in SubGraphs so needs to be modified to record that.
		if (UEdGraph* ParentGraph = GetGraph())
		{
			ParentGraph->Modify();
		}

		DestroyNode();
	}
}

void UEdNode_MounteaDialogueGraphEdge::PrepareForCopying()
{
	MounteaDialogueGraphEdge->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UEdNode_MounteaDialogueGraphEdge::CreateConnections(UEdNode_MounteaDialogueGraphNode* Start, UEdNode_MounteaDialogueGraphNode* End)
{
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	Start->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(Start->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	End->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(End->GetInputPin());
}

UEdNode_MounteaDialogueGraphNode* UEdNode_MounteaDialogueGraphEdge::GetStartNode()
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UEdNode_MounteaDialogueGraphNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}
	
	return nullptr;
}

UEdNode_MounteaDialogueGraphNode* UEdNode_MounteaDialogueGraphEdge::GetEndNode()
{
	if (Pins[1]->LinkedTo.Num() > 0)
	{
		return Cast<UEdNode_MounteaDialogueGraphNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
	}

	return nullptr;
}
