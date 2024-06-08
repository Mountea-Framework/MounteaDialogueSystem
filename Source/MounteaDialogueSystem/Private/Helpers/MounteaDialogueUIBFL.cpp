// Fill out your copyright notice in the Description page of Project Settings.


#include "Helpers/MounteaDialogueUIBFL.h"

#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

FDialogueOptionData UMounteaDialogueUIBFL::NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow)
{
	return FDialogueOptionData(Node, DialogueRow);
}

FGuid UMounteaDialogueUIBFL::GetDialogueNodeGuid(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode)
{
	return FromNode ? FromNode->GetNodeGUID() : FGuid();
}

FDialogueRow UMounteaDialogueUIBFL::GetDialogueNodeRow(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode)
{
	if (TObjectPtr<UMounteaDialogueGraphNode_DialogueNodeBase> dialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(FromNode) )
	{
		return UMounteaDialogueSystemBFC::GetDialogueRow(FromNode);
	}

	return FDialogueRow();
}
