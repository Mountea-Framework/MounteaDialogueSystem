// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "Helpers/MounteaDialogueUIBFL.h"

#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Internationalization/Regex.h"
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

TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> UMounteaDialogueUIBFL::FilterDialogueFriendlyNodes(const TArray<UMounteaDialogueGraphNode*>& RawNodes)
{
	TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> returnArray;

	for (const auto& Itr : RawNodes)
	{
		if (!Itr) continue;

		if (TObjectPtr<UMounteaDialogueGraphNode_DialogueNodeBase> dialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Itr))
		{
			returnArray.Add(dialogueNode);
		}
	}

	return returnArray;
}

FText UMounteaDialogueUIBFL::ReplaceRegexInText(const FString& Regex, const FText& Replacement, const FText& SourceText)
{
	FString					sourceString = SourceText.ToString();
	FRegexPattern	regexPattern(Regex);
	FRegexMatcher	regexMatcher(regexPattern, sourceString);

	FString					formattedString;

	int32					previousPosition = 0;
	FString					replacementText = Replacement.ToString();

	while (regexMatcher.FindNext())
	{
		formattedString += sourceString.Mid(previousPosition, regexMatcher.GetMatchBeginning() - previousPosition);
		formattedString += replacementText;
		previousPosition = regexMatcher.GetMatchEnding();
	}
	
	formattedString += sourceString.Mid(previousPosition);
	
	return FText::FromString(formattedString);
}
