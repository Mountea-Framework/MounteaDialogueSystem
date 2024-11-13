// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "Helpers/MounteaDialogueHUDStatics.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/HUD/MounteaDialogueHUDClassInterface.h"
#include "Interfaces/HUD/MounteaDialogueUIBaseInterface.h"
#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Interfaces/UMG/MounteaDialogueRowInterface.h"
#include "Interfaces/UMG/MounteaDialogueViewportWidgetInterface.h"
#include "Internationalization/Regex.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "WBP/MounteaDialogueOptionsContainer.h"

FDialogueOptionData UMounteaDialogueHUDStatics::NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow)
{
	return FDialogueOptionData(Node, DialogueRow);
}

FWidgetDialogueRow UMounteaDialogueHUDStatics::NewDialogueWidgetRowData(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData)
{
	return FWidgetDialogueRow(SourceRow, SourceRowData);
}

FGuid UMounteaDialogueHUDStatics::GetDialogueNodeGuid(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode)
{
	return FromNode ? FromNode->GetNodeGUID() : FGuid();
}

FDialogueRow UMounteaDialogueHUDStatics::GetDialogueNodeRow(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode)
{
	if (TObjectPtr<UMounteaDialogueGraphNode_DialogueNodeBase> dialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(FromNode) )
	{
		return UMounteaDialogueSystemBFC::GetDialogueRow(FromNode);
	}

	return FDialogueRow::Invalid();
}

TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> UMounteaDialogueHUDStatics::FilterDialogueFriendlyNodes(const TArray<UMounteaDialogueGraphNode*>& RawNodes)
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

	UMounteaDialogueSystemBFC::SortNodes(returnArray);
	return returnArray;
}

FText UMounteaDialogueHUDStatics::ReplaceRegexInText(const FString& Regex, const FText& Replacement, const FText& SourceText)
{
	FString	sourceString = SourceText.ToString();
	FRegexPattern	regexPattern(Regex);
	FRegexMatcher	regexMatcher(regexPattern, sourceString);

	FString	formattedString;

	int32	previousPosition = 0;
	FString	replacementText = Replacement.ToString();

	while (regexMatcher.FindNext())
	{
		formattedString += sourceString.Mid(previousPosition, regexMatcher.GetMatchBeginning() - previousPosition);
		formattedString += replacementText;
		previousPosition = regexMatcher.GetMatchEnding();
	}
	
	formattedString += sourceString.Mid(previousPosition);
	
	return FText::FromString(formattedString);
}

int32 UMounteaDialogueHUDStatics::GetWidgetZOrder(UUserWidget* Widget, UObject* WorldContext)
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

TSubclassOf<UUserWidget> UMounteaDialogueHUDStatics::GetViewportBaseClass(AActor* ViewportManager)
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

void UMounteaDialogueHUDStatics::InitializeViewportWidget(AActor* ViewportManager)
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

UUserWidget* UMounteaDialogueHUDStatics::GetViewportWidget(AActor* ViewportManager)
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

void UMounteaDialogueHUDStatics::AddChildWidgetToViewport(AActor* ViewportManager, UUserWidget* ChildWidget, const FWidgetAdditionParams& WidgetParams)
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

void UMounteaDialogueHUDStatics::RemoveChildWidgetFromViewport(AActor* ViewportManager, UUserWidget* ChildWidget)
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

void UMounteaDialogueHUDStatics::AddChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget, const FWidgetAdditionParams& WidgetParams)
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

void UMounteaDialogueHUDStatics::RemoveChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget)
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

bool UMounteaDialogueHUDStatics::BindEvents(UUserWidget* Target)
{
	if (!IsValid(Target))
		return false;

	if (Target->Implements<UMounteaDialogueUIBaseInterface>())
		return IMounteaDialogueUIBaseInterface::Execute_BindEvents(Target);

	return false;
}

bool UMounteaDialogueHUDStatics::UnbindEvents(UUserWidget* Target)
{
	if (!IsValid(Target))
		return false;

	if (Target->Implements<UMounteaDialogueUIBaseInterface>())
		return IMounteaDialogueUIBaseInterface::Execute_UnbindEvents(Target);

	return false;
}

void UMounteaDialogueHUDStatics::ProcessStringCommand(UUserWidget* Target, const FString& Command, UObject* OptionalPayload)
{
	if (!IsValid(Target))
		return;

	if (Target->Implements<UMounteaDialogueUIBaseInterface>())
		return IMounteaDialogueUIBaseInterface::Execute_ProcessStringCommand(Target, Command, OptionalPayload);
}

void UMounteaDialogueHUDStatics::ApplyTheme(UUserWidget* Target)
{
	if (!IsValid(Target))
		return;

	if (Target->Implements<UMounteaDialogueUIBaseInterface>())
		return IMounteaDialogueUIBaseInterface::Execute_ApplyTheme(Target);
}

void UMounteaDialogueHUDStatics::RefreshDialogueWidget(UObject* Target, const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command)
{
	if (!IsValid(Target))
		return;

	if (Target->Implements<UMounteaDialogueWBPInterface>())
		return IMounteaDialogueWBPInterface::Execute_RefreshDialogueWidget(Target, DialogueManager, Command);
}

void UMounteaDialogueHUDStatics::OnOptionSelected(UObject* Target, const FGuid& SelectionGUID)
{
	if (!IsValid(Target))
		return;

	if (Target->Implements<UMounteaDialogueWBPInterface>())
		return IMounteaDialogueWBPInterface::Execute_OnOptionSelected(Target, SelectionGUID);
}

FDialogueOptionData UMounteaDialogueHUDStatics::GetDialogueOptionData(UObject* DialogueOption)
{
	if (!IsValid(DialogueOption))
		return FDialogueOptionData();

	if (DialogueOption->Implements<UMounteaDialogueOptionInterface>())
		return IMounteaDialogueOptionInterface::Execute_GetDialogueOptionData(DialogueOption);

	return FDialogueOptionData();
}

void UMounteaDialogueHUDStatics::SetNewDialogueOptionData(UObject* DialogueOption, const FDialogueOptionData& NewData)
{
	if (!IsValid(DialogueOption))
		return;

	if (DialogueOption->Implements<UMounteaDialogueOptionInterface>())
		return IMounteaDialogueOptionInterface::Execute_SetNewDialogueOptionData(DialogueOption, NewData);
}

void UMounteaDialogueHUDStatics::ResetDialogueOptionData(UObject* DialogueOption)
{
	if (!IsValid(DialogueOption))
		return;

	if (DialogueOption->Implements<UMounteaDialogueOptionInterface>())
		return IMounteaDialogueOptionInterface::Execute_ResetDialogueOptionData(DialogueOption);
}

void UMounteaDialogueHUDStatics::InitializeDialogueOption(UObject* DialogueOption)
{
	if (!IsValid(DialogueOption))
		return;

	if (DialogueOption->Implements<UMounteaDialogueOptionInterface>())
		return IMounteaDialogueOptionInterface::Execute_InitializeDialogueOption(DialogueOption);
}

void UMounteaDialogueHUDStatics::ProcessOptionSelected(UObject* DialogueOption)
{
	if (!IsValid(DialogueOption))
		return;

	if (DialogueOption->Implements<UMounteaDialogueOptionInterface>())
		return IMounteaDialogueOptionInterface::Execute_ProcessOptionSelected(DialogueOption);
}

void UMounteaDialogueHUDStatics::SetParentDialogueWidget(UObject* ContainerObject, UUserWidget* NewParentDialogueWidget)
{
	if (!IsValid(ContainerObject))
		return;

	if (!IsValid(NewParentDialogueWidget))
		return;

	if (!ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return;

	if (NewParentDialogueWidget->Implements<UMounteaDialogueWBPInterface>())
		return;

	return IMounteaDialogueOptionsContainerInterface::Execute_SetParentDialogueWidget(ContainerObject, NewParentDialogueWidget);
}

UUserWidget* UMounteaDialogueHUDStatics::GetParentDialogueWidget(UObject* ContainerObject)
{
	if (!IsValid(ContainerObject))
		return nullptr;

	if (!ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return nullptr;

	return IMounteaDialogueOptionsContainerInterface::Execute_GetParentDialogueWidget(ContainerObject);
}

TSoftClassPtr<UUserWidget> UMounteaDialogueHUDStatics::GetDialogueOptionClass(UObject* ContainerObject)
{
	if (!IsValid(ContainerObject))
		return nullptr;

	if (!ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return nullptr;

	return IMounteaDialogueOptionsContainerInterface::Execute_GetDialogueOptionClass(ContainerObject);
}

void UMounteaDialogueHUDStatics::SetDialogueOptionClass(UObject* ContainerObject, const TSoftClassPtr<UUserWidget>& NewDialogueOptionClass)
{
	if (!IsValid(ContainerObject))
		return;

	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return IMounteaDialogueOptionsContainerInterface::Execute_SetDialogueOptionClass(ContainerObject, NewDialogueOptionClass);
}

void UMounteaDialogueHUDStatics::AddNewDialogueOption(UObject* ContainerObject, UMounteaDialogueGraphNode_DialogueNodeBase* NewDialogueOption)
{
	if (!IsValid(ContainerObject))
		return;

	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return IMounteaDialogueOptionsContainerInterface::Execute_AddNewDialogueOption(ContainerObject, NewDialogueOption);
}

void UMounteaDialogueHUDStatics::AddNewDialogueOptions(UObject* ContainerObject, const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& NewDialogueOptions)
{
	if (!IsValid(ContainerObject))
		return;

	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return IMounteaDialogueOptionsContainerInterface::Execute_AddNewDialogueOptions(ContainerObject, NewDialogueOptions);
}

void UMounteaDialogueHUDStatics::RemoveDialogueOption(UObject* ContainerObject, UMounteaDialogueGraphNode_DialogueNodeBase* DirtyDialogueOption)
{
	if (!IsValid(ContainerObject))
		return;

	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return IMounteaDialogueOptionsContainerInterface::Execute_RemoveDialogueOption(ContainerObject, DirtyDialogueOption);
}

void UMounteaDialogueHUDStatics::RemoveDialogueOptions(UObject* ContainerObject, const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& DirtyDialogueOptions)
{
	if (!IsValid(ContainerObject))
		return;

	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return IMounteaDialogueOptionsContainerInterface::Execute_RemoveDialogueOptions(ContainerObject, DirtyDialogueOptions);
}

void UMounteaDialogueHUDStatics::ClearDialogueOptions(UObject* ContainerObject)
{
	if (!IsValid(ContainerObject))
		return;

	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return IMounteaDialogueOptionsContainerInterface::Execute_ClearDialogueOptions(ContainerObject);
}

void UMounteaDialogueHUDStatics::ProcessContainerOptionSelected(UObject* ContainerObject, const FGuid& SelectedOption, UUserWidget* CallingWidget)
{
	if (!IsValid(ContainerObject))
		return;

	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
		return IMounteaDialogueOptionsContainerInterface::Execute_ProcessOptionSelected(ContainerObject, SelectedOption, CallingWidget);
}

TArray<UUserWidget*> UMounteaDialogueHUDStatics::GetDialogueOptions(UObject* ContainerObject)
{
	TArray<UUserWidget*> dialogueOptions;
	if (!IsValid(ContainerObject))
	{
		LOG_ERROR(TEXT("[GetDialogueOptions] Invalid Parent Widget provided!"));
		return dialogueOptions;
	}
	
	if (ContainerObject->Implements<UMounteaDialogueOptionsContainer>())
	{
		return IMounteaDialogueOptionsContainerInterface::Execute_GetDialogueOptions(ContainerObject);
	}

	LOG_ERROR(TEXT("[GetDialogueOptions] ParentWidget does not implement `MounteaDialogueOptionsContainerInterface`!"));
	return dialogueOptions;
}
