// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "Helpers/MounteaDialogueUIBFL.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Interfaces/UMG/MounteaDialogueRowInterface.h"
#include "Interfaces/UMG/MounteaDialogueViewportWidgetInterface.h"
#include "Internationalization/Regex.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

FDialogueOptionData UMounteaDialogueUIBFL::NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow)
{
	return FDialogueOptionData(Node, DialogueRow);
}

FWidgetDialogueRow UMounteaDialogueUIBFL::NewDialogueWidgetRowData(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData)
{
	return FWidgetDialogueRow(SourceRow, SourceRowData);
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

int32 UMounteaDialogueUIBFL::GetWidgetZOrder(UUserWidget* Widget, UObject* WorldContext)
{
	if (!Widget || !WorldContext)
		return -1;

	ULocalPlayer* localPlayer = Widget->GetOwningLocalPlayer();
	if (localPlayer)
	{
		if (UGameViewportSubsystem* viewportSubsystem = UGameViewportSubsystem::Get(WorldContext->GetWorld()))
		{
			FGameViewportWidgetSlot widgetSlot = viewportSubsystem->GetWidgetSlot(Widget);

			return widgetSlot.ZOrder;
		}
	}

	return -1;
}

void UMounteaDialogueUIBFL::AddChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget, const int32 ZOrder, const FAnchors WidgetAnchors, const FMargin& WidgetMargin)
{
	TScriptInterface<IMounteaDialogueViewportWidgetInterface> parentInterface = ParentWidget;
	if (!parentInterface.GetObject() || !parentInterface.GetInterface())
	{
		LOG_ERROR(TEXT("[AddChildWidget] ParentWidget does not implement `MounteaDialogueViewportWidgetInterface`!"))
		return;
	}

	parentInterface->Execute_AddChildWidget(ParentWidget, ChildWidget, ZOrder, WidgetAnchors, WidgetMargin);
}

void UMounteaDialogueUIBFL::RemoveChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget)
{
	TScriptInterface<IMounteaDialogueViewportWidgetInterface> parentInterface = ParentWidget;
	if (!parentInterface.GetObject() || !parentInterface.GetInterface())
	{
		LOG_ERROR(TEXT("[RemoveChildWidget] ParentWidget does not implement `MounteaDialogueViewportWidgetInterface`!"))
		return;
	}

	parentInterface->Execute_RemoveChildWidget(ParentWidget, ChildWidget);
}
