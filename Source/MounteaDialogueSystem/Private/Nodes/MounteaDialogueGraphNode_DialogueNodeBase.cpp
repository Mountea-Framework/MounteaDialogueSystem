// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_DialogueNodeBase"

UMounteaDialogueGraphNode_DialogueNodeBase::UMounteaDialogueGraphNode_DialogueNodeBase()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseTitle", "Dialogue Node Base");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseContextMenu", "Dialogue Node");
	BackgroundColor = FLinearColor(FColor::Orange);
	NodeGUID = FGuid::NewGuid();
}

FText UMounteaDialogueGraphNode_DialogueNodeBase::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseDescription", "Dialogue Base Node has no logic tied to itself.");
}

bool UMounteaDialogueGraphNode_DialogueNodeBase::CanCreateConnection(UMounteaDialogueGraphNode* Other, EEdGraphPinDirection Direction, FText& ErrorMessage)
{
	if (Other == nullptr)
	{
		ErrorMessage = FText::FromString("Invalid Other Node!");
	}

	if (Direction == EGPD_Output)
	{
		
		// Fast checking for native classes
		if ( AllowedInputClasses.Contains(Other->GetClass()) )
		{
			return true;
		}

		// Slower iterative checking for child classes
		for (auto Itr : AllowedInputClasses)
		{
			if (Other->GetClass()->IsChildOf(Itr))
			{
				return true;
			}
		}
		
		ErrorMessage = FText::FromString("Invalid Node Connection!");
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

