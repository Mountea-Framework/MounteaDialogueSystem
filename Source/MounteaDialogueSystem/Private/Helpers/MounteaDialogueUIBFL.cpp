// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "Helpers/MounteaDialogueUIBFL.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/HUD/MounteaDialogueHUDClassInterface.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Interfaces/UMG/MounteaDialogueRowInterface.h"
#include "Interfaces/UMG/MounteaDialogueViewportWidgetInterface.h"
#include "Internationalization/Regex.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "WBP/MounteaDialogueOptionsContainer.h"

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

TSubclassOf<UUserWidget> UMounteaDialogueUIBFL::GetViewportBaseClass(AActor* ViewportManager)
{
	if (!IsValid(ViewportManager))
	{
		LOG_ERROR(TEXT("[GetViewportBaseClass] Invalid Viewport Manager provided!"));
		return nullptr;
	}
	
	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		return IMounteaDialogueHUDClassInterface::Execute_GetViewportBaseClass(ViewportManager);
	}

	LOG_ERROR(TEXT("[GetViewportWidget] Viewport manager does not implement 'MounteaDialogueHUDClassInterface'!"));
	return nullptr;
}

void UMounteaDialogueUIBFL::InitializeViewportWidget(AActor* ViewportManager)
{
	if (!IsValid(ViewportManager))
	{
		LOG_ERROR(TEXT("[InitializeViewportWidget] Invalid Viewport Manager provided!"));
		return;
	}
	
	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		return IMounteaDialogueHUDClassInterface::Execute_InitializeViewportWidget(ViewportManager);
	}

	LOG_ERROR(TEXT("[InitializeViewportWidget] Viewport manager does not implement 'MounteaDialogueHUDClassInterface'!"));
}

UUserWidget* UMounteaDialogueUIBFL::GetViewportWidget(AActor* ViewportManager)
{
	if (!IsValid(ViewportManager))
	{
		LOG_ERROR(TEXT("[GetViewportWidget] Invalid Viewport Manager provided!"));
		return nullptr;
	}
	
	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		return IMounteaDialogueHUDClassInterface::Execute_GetViewportWidget(ViewportManager);
	}

	LOG_ERROR(TEXT("[GetViewportWidget] Viewport manager does not implement 'MounteaDialogueHUDClassInterface'!"));
	return nullptr;
}

void UMounteaDialogueUIBFL::AddChildWidgetToViewport(AActor* ViewportManager, UUserWidget* ChildWidget, const FWidgetAdditionParams& WidgetParams)
{
	if (!IsValid(ViewportManager))
	{
		LOG_ERROR(TEXT("[AddChildWidgetToViewport] Invalid Viewport Manager provided!"));
		return;
	}
	
	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		return IMounteaDialogueHUDClassInterface::Execute_AddChildWidgetToViewport(ViewportManager, ChildWidget, WidgetParams.ZOrder, WidgetParams.Anchors, WidgetParams.Margin);
	}

	LOG_ERROR(TEXT("[AddChildWidgetToViewport] Viewport manager does not implement 'MounteaDialogueHUDClassInterface'!"));
}

void UMounteaDialogueUIBFL::RemoveChildWidgetFromViewport(AActor* ViewportManager, UUserWidget* ChildWidget)
{
	if (!IsValid(ViewportManager))
	{
		LOG_ERROR(TEXT("[RemoveChildWidgetFromViewport] Invalid Viewport Manager provided!"));
		return;
	}
	
	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		return IMounteaDialogueHUDClassInterface::Execute_RemoveChildWidgetFromViewport(ViewportManager, ChildWidget);
	}

	LOG_ERROR(TEXT("[RemoveChildWidgetFromViewport] Viewport manager does not implement 'MounteaDialogueHUDClassInterface'!"));
}

void UMounteaDialogueUIBFL::AddChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget, const FWidgetAdditionParams& WidgetParams)
{
	if (!IsValid(ParentWidget))
	{
		LOG_ERROR(TEXT("[AddChildWidget] Invalid Parent Widget provided!"));
		return;
	}
	
	if (ParentWidget->Implements<UMounteaDialogueViewportWidgetInterface>())
	{
		return IMounteaDialogueViewportWidgetInterface::Execute_AddChildWidget(ParentWidget, ChildWidget, WidgetParams.ZOrder, WidgetParams.Anchors, WidgetParams.Margin);
	}

	LOG_ERROR(TEXT("[AddChildWidget] ParentWidget does not implement `MounteaDialogueViewportWidgetInterface`!"));
}

void UMounteaDialogueUIBFL::RemoveChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget)
{
	if (!IsValid(ParentWidget))
	{
		LOG_ERROR(TEXT("[RemoveChildWidget] Invalid Parent Widget provided!"));
		return;
	}
	
	if (ParentWidget->Implements<UMounteaDialogueViewportWidgetInterface>())
	{
		return IMounteaDialogueViewportWidgetInterface::Execute_RemoveChildWidget(ParentWidget, ChildWidget);
	}

	LOG_ERROR(TEXT("[RemoveChildWidget] ParentWidget does not implement `MounteaDialogueViewportWidgetInterface`!"));
}

TArray<UUserWidget*> UMounteaDialogueUIBFL::GetDialogueOptions(UUserWidget* ParentWidget)
{
	TArray<UUserWidget*> dialogueOptions;
	if (!IsValid(ParentWidget))
	{
		LOG_ERROR(TEXT("[GetDialogueOptions] Invalid Parent Widget provided!"));
		return dialogueOptions;
	}
	
	if (ParentWidget->Implements<UMounteaDialogueOptionsContainer>())
	{
		return IMounteaDialogueOptionsContainerInterface::Execute_GetDialogueOptions(ParentWidget);
	}

	LOG_ERROR(TEXT("[GetDialogueOptions] ParentWidget does not implement `MounteaDialogueOptionsContainerInterface`!"));
	return dialogueOptions;
}
