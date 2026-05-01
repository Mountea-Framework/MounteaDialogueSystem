// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "Helpers/MounteaDialogueHUDStatics.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Interfaces/HUD/MounteaDialogueHUDClassInterface.h"
#include "Interfaces/HUD/MounteaDialogueUIBaseInterface.h"
#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Interfaces/UMG/MounteaDialogueRowInterface.h"
#include "Interfaces/UMG/MounteaDialogueSkipInterface.h"
#include "Interfaces/UMG/MounteaDialogueViewportWidgetInterface.h"
#include "Interfaces/Nodes/MounteaDialogueSpeechDataInterface.h"
#include "Internationalization/Regex.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Subsystem/MounteaDialogueLocalPlayerSubsystem.h"
#include "WBP/MounteaDialogueOptionsContainer.h"

FDialogueOptionData UMounteaDialogueHUDStatics::NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow)
{
	return FDialogueOptionData(Node, DialogueRow);
}

FWidgetDialogueRow UMounteaDialogueHUDStatics::NewDialogueWidgetRowData(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData)
{
	return FWidgetDialogueRow(SourceRow, SourceRowData);
}

FGuid UMounteaDialogueHUDStatics::GetDialogueNodeGuid(UMounteaDialogueGraphNode* FromNode)
{
	return FromNode ? FromNode->GetNodeGUID() : FGuid();
}

FDialogueRow UMounteaDialogueHUDStatics::GetDialogueNodeRow(UMounteaDialogueGraphNode* FromNode)
{
	if (IsValid(FromNode) && FromNode->Implements<UMounteaDialogueSpeechDataInterface>())
		return UMounteaDialogueTraversalStatics::GetSpeechData(FromNode);

	return FDialogueRow::Invalid();
}

TArray<UMounteaDialogueGraphNode*> UMounteaDialogueHUDStatics::FilterDialogueFriendlyNodes(const TArray<UMounteaDialogueGraphNode*>& RawNodes)
{
	TArray<UMounteaDialogueGraphNode*> returnArray;
	
	for (const auto& Itr : RawNodes)
	{
		if (IsValid(Itr) && Itr->Implements<UMounteaDialogueSpeechDataInterface>())
			returnArray.Add(Itr);
	}

	returnArray.Sort([](const UMounteaDialogueGraphNode& Left, const UMounteaDialogueGraphNode& Right)
	{
		return Left.ExecutionOrder < Right.ExecutionOrder;
	});
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
		return INDEX_NONE;

	ULocalPlayer* localPlayer = Widget->GetOwningLocalPlayer();
	if (localPlayer)
	{
		if (UGameViewportSubsystem* viewportSubsystem = UGameViewportSubsystem::Get(WorldContext->GetWorld()))
		{
			FGameViewportWidgetSlot widgetSlot = viewportSubsystem->GetWidgetSlot(Widget);

			return widgetSlot.ZOrder;
		}
	}

	return INDEX_NONE;
}

EDialogueOptionState UMounteaDialogueHUDStatics::GetFocusState(UUserWidget* Widget)
{
	if (!IsValid(Widget))
		return EDialogueOptionState::Default;

	if (Widget->Implements<UMounteaFocusableWidgetInterface>())
		return IMounteaFocusableWidgetInterface::Execute_GetFocusState(Widget);
	
	LOG_ERROR(TEXT("[Get Focus State] Widget does not implement 'MounteaFocusableWidgetInterface'!"));
	return EDialogueOptionState::Default;
}

void UMounteaDialogueHUDStatics::SetFocusState(UUserWidget* Widget, const bool IsFocused)
{
	if (!IsValid(Widget))
		return;

	if (!Widget->Implements<UMounteaFocusableWidgetInterface>())
	{
		LOG_ERROR(TEXT("[Set Focus State] Widget does not implement 'MounteaFocusableWidgetInterface'!"));
		return;
	}

	IMounteaFocusableWidgetInterface::Execute_SetFocusState(Widget, IsFocused);
}

int32 UMounteaDialogueHUDStatics::GetSafeOptionIndex(UObject* Container, const EUINavigation Direction)
{
	if (!IsValid(Container))
		return INDEX_NONE;

	if (!Container->Implements<UMounteaDialogueOptionsContainerInterface>())
		return INDEX_NONE;

	auto optionValues = IMounteaDialogueOptionsContainerInterface::Execute_GetDialogueOptions(Container);
	if (optionValues.Num() <= 0)
		return INDEX_NONE;

	const int32 currentActive = IMounteaDialogueOptionsContainerInterface::Execute_GetFocusedOptionIndex(Container);
	int32 newActive = currentActive;

	switch (Direction)
	{
		case EUINavigation::Down:
		case EUINavigation::Next:
		case EUINavigation::Right:
			newActive++;
			break;
		case EUINavigation::Up:
		case EUINavigation::Previous:
		case EUINavigation::Left:
			newActive--;
			break;
		case EUINavigation::Num:
		case EUINavigation::Invalid:
			newActive = INDEX_NONE;
			break;
	}
	
	if (newActive < 0)
		newActive = optionValues.Num() - 1;
	else if (newActive >= optionValues.Num())
		newActive = 0;

	return newActive;
}


TSubclassOf<UUserWidget> UMounteaDialogueHUDStatics::GetViewportBaseClass(UObject* ViewportManager)
{
	if (!IsValid(ViewportManager))
		return nullptr;

	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
		return IMounteaDialogueHUDClassInterface::Execute_GetViewportBaseClass(ViewportManager);

	if (const APlayerController* playerController = Cast<APlayerController>(ViewportManager))
	{
		if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
		{
			if (UMounteaDialogueLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UMounteaDialogueLocalPlayerSubsystem>())
				return subsystem->GetViewportBaseClassSafe();
		}
	}

	return nullptr;
}

void UMounteaDialogueHUDStatics::InitializeViewportWidget(UObject* ViewportManager)
{
	if (!IsValid(ViewportManager))
		return;

	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		IMounteaDialogueHUDClassInterface::Execute_InitializeViewportWidget(ViewportManager);
		return;
	}

	if (APlayerController* playerController = Cast<APlayerController>(ViewportManager))
	{
		if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
		{
			if (UMounteaDialogueLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UMounteaDialogueLocalPlayerSubsystem>())
				subsystem->InitializeViewportWidget();
		}
	}
}

UUserWidget* UMounteaDialogueHUDStatics::GetViewportWidget(UObject* ViewportManager)
{
	if (!IsValid(ViewportManager))
		return nullptr;

	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
		return IMounteaDialogueHUDClassInterface::Execute_GetViewportWidget(ViewportManager);

	if (const APlayerController* playerController = Cast<APlayerController>(ViewportManager))
	{
		if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
		{
			if (UMounteaDialogueLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UMounteaDialogueLocalPlayerSubsystem>())
				return subsystem->GetViewportWidget();
		}
	}

	return nullptr;
}

void UMounteaDialogueHUDStatics::AddChildWidgetToViewport(UObject* ViewportManager, UUserWidget* ChildWidget, const FWidgetAdditionParams WidgetParams)
{
	if (!IsValid(ViewportManager) || !IsValid(ChildWidget))
		return;

	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		IMounteaDialogueHUDClassInterface::Execute_AddChildWidgetToViewport(ViewportManager, ChildWidget, WidgetParams.ZOrder, WidgetParams.Anchors, WidgetParams.Margin);
		return;
	}

	if (APlayerController* playerController = Cast<APlayerController>(ViewportManager))
	{
		if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
		{
			if (UMounteaDialogueLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UMounteaDialogueLocalPlayerSubsystem>())
			{
				subsystem->AttachWidget(ChildWidget, WidgetParams);
				return;
			}
		}
	}

	ChildWidget->AddToPlayerScreen(WidgetParams.ZOrder);
}

void UMounteaDialogueHUDStatics::RemoveChildWidgetFromViewport(UObject* ViewportManager, UUserWidget* ChildWidget)
{
	if (!IsValid(ViewportManager) || !IsValid(ChildWidget))
		return;

	if (ViewportManager->Implements<UMounteaDialogueHUDClassInterface>())
	{
		IMounteaDialogueHUDClassInterface::Execute_RemoveChildWidgetFromViewport(ViewportManager, ChildWidget);
		return;
	}

	if (APlayerController* playerController = Cast<APlayerController>(ViewportManager))
	{
		if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
		{
			if (UMounteaDialogueLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UMounteaDialogueLocalPlayerSubsystem>())
			{
				subsystem->RemoveWidget(ChildWidget);
				return;
			}
		}
	}

	ChildWidget->RemoveFromParent();
}

void UMounteaDialogueHUDStatics::AddChildWidget(UObject* ParentWidget, UUserWidget* ChildWidget, const FWidgetAdditionParams& WidgetParams)
{
	if (!IsValid(ParentWidget))
	{
		LOG_ERROR(TEXT("[AddChildWidget] Invalid Parent Widget provided!"));
		return;
	}
	
	if (ParentWidget->Implements<UMounteaDialogueViewportWidgetInterface>())
		return IMounteaDialogueViewportWidgetInterface::Execute_AddChildWidget(ParentWidget, ChildWidget, WidgetParams.ZOrder, WidgetParams.Anchors, WidgetParams.Margin);

	LOG_ERROR(TEXT("[AddChildWidget] ParentWidget does not implement `MounteaDialogueViewportWidgetInterface`!"));
}

void UMounteaDialogueHUDStatics::RemoveChildWidget(UObject* ParentWidget, UUserWidget* ChildWidget)
{
	if (!IsValid(ParentWidget))
	{
		LOG_ERROR(TEXT("[RemoveChildWidget] Invalid Parent Widget provided!"));
		return;
	}
	
	if (ParentWidget->Implements<UMounteaDialogueViewportWidgetInterface>())
		return IMounteaDialogueViewportWidgetInterface::Execute_RemoveChildWidget(ParentWidget, ChildWidget);

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

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		LOG_WARNING(TEXT("[Set Parent Dialogue Widget] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		return;
	}

	TScriptInterface<IMounteaDialogueWBPInterface> parentWidgetInterface;
	parentWidgetInterface.SetObject(ContainerObject);
	parentWidgetInterface.SetInterface(Cast<IMounteaDialogueWBPInterface>(NewParentDialogueWidget));

	if (!parentWidgetInterface.GetObject())
	{
		LOG_WARNING(TEXT("[Set Parent Dialogue Widget] Parent Widget Object %s doesn't properly implement `IMounteaDialogueWBPInterface`!"), *NewParentDialogueWidget->GetName())
		return;
	}

	return containerInterface->Execute_SetParentDialogueWidget(ContainerObject, NewParentDialogueWidget);
}

UUserWidget* UMounteaDialogueHUDStatics::GetParentDialogueWidget(UObject* ContainerObject)
{
	if (!IsValid(ContainerObject)) return nullptr;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Get Parent Dialogue Widget] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return nullptr;
	}

	return containerInterface->Execute_GetParentDialogueWidget(ContainerObject);
}

TSoftClassPtr<UUserWidget> UMounteaDialogueHUDStatics::GetDialogueOptionClass(UObject* ContainerObject)
{
	if (!IsValid(ContainerObject)) return nullptr;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Get Dialogue Option Class] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return nullptr;
	}

	return containerInterface->Execute_GetDialogueOptionClass(ContainerObject);
}

void UMounteaDialogueHUDStatics::SetDialogueOptionClass(UObject* ContainerObject, const TSoftClassPtr<UUserWidget>& NewDialogueOptionClass)
{
	if (!IsValid(ContainerObject)) return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Set Dialogue Option Class] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return;
	}

	containerInterface->Execute_SetDialogueOptionClass(ContainerObject, NewDialogueOptionClass);
}

void UMounteaDialogueHUDStatics::AddNewDialogueOption(UObject* ContainerObject, UMounteaDialogueGraphNode* NewDialogueOption)
{
	if (!IsValid(NewDialogueOption) || !NewDialogueOption->Implements<UMounteaDialogueSpeechDataInterface>())
	{
		LOG_WARNING(TEXT("[Add New Dialogue Option] Invalid Dialogue Option provided!"))
		return;
	}
	
	if (!IsValid(ContainerObject)) 
		return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Add New Dialogue Option] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return;
	}

	containerInterface->Execute_AddNewDialogueOption(ContainerObject, NewDialogueOption);
}

void UMounteaDialogueHUDStatics::AddNewDialogueOptions(UObject* ContainerObject, const TArray<UMounteaDialogueGraphNode*>& NewDialogueOptions)
{	
	if (!IsValid(ContainerObject))
		return;

	TScriptInterface<IMounteaDialogueOptionsContainerInterface> containerInterface;
	containerInterface.SetObject(ContainerObject);
	containerInterface.SetInterface(Cast<IMounteaDialogueOptionsContainerInterface>(ContainerObject));

	if (containerInterface.GetObject())
		containerInterface->Execute_AddNewDialogueOptions(ContainerObject, NewDialogueOptions);
	else
		LOG_WARNING(TEXT("[Add New Dialogue Options] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
}

void UMounteaDialogueHUDStatics::RemoveDialogueOption(UObject* ContainerObject, UMounteaDialogueGraphNode* DirtyDialogueOption)
{
	if (!IsValid(DirtyDialogueOption) || !DirtyDialogueOption->Implements<UMounteaDialogueSpeechDataInterface>())
	{
		LOG_WARNING(TEXT("[Remove Dialogue Option] Invalid Dialogue Option provided!"))
		return;
	}
	
	if (!IsValid(ContainerObject)) return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Remove Dialogue Option] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return;
	}

	containerInterface->Execute_RemoveDialogueOption(ContainerObject, DirtyDialogueOption);
}

void UMounteaDialogueHUDStatics::RemoveDialogueOptions(UObject* ContainerObject, const TArray<UMounteaDialogueGraphNode*>& DirtyDialogueOptions)
{
	if (!IsValid(ContainerObject)) return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Remove Dialogue Options] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return;
	}

	containerInterface->Execute_RemoveDialogueOptions(ContainerObject, DirtyDialogueOptions);
}

void UMounteaDialogueHUDStatics::ClearDialogueOptions(UObject* ContainerObject)
{
	if (!IsValid(ContainerObject)) return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Clear Dialogue Options] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return;
	}

	containerInterface->Execute_ClearDialogueOptions(ContainerObject);
}

void UMounteaDialogueHUDStatics::ProcessContainerOptionSelected(UObject* ContainerObject, const FGuid& SelectedOption, UUserWidget* CallingWidget)
{
	if (!IsValid(ContainerObject)) return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Process Container Option Selected] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return;
	}

	containerInterface->Execute_ProcessOptionSelected(ContainerObject, SelectedOption, CallingWidget);
}

TArray<UUserWidget*> UMounteaDialogueHUDStatics::GetDialogueOptions(UObject* ContainerObject)
{
	TArray<UUserWidget*> dialogueOptions;
	if (!IsValid(ContainerObject)) return dialogueOptions;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		 LOG_WARNING(TEXT("[Get Dialogue Options] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		 return dialogueOptions;
	}

	return containerInterface->Execute_GetDialogueOptions(ContainerObject);
}

int32 UMounteaDialogueHUDStatics::GetFocusedOptionIndex(UObject* ContainerObject)
{
	if (!IsValid(ContainerObject)) return INDEX_NONE;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		LOG_WARNING(TEXT("[Get Focused Option Index] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		return INDEX_NONE;
	}

	return containerInterface->Execute_GetFocusedOptionIndex(ContainerObject);
}

void UMounteaDialogueHUDStatics::SetFocusedOption(UObject* ContainerObject, const int32 NewFocusedOption)
{
	if (!IsValid(ContainerObject)) return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);

	if (!containerInterface.GetObject())
	{
		LOG_WARNING(TEXT("[Set Focused Option Index] Container Object %s doesn't properly implement `IMounteaDialogueOptionsContainerInterface`!"), *ContainerObject->GetName())
		return;
	}

	return containerInterface->Execute_SetFocusedOption(ContainerObject, NewFocusedOption);
}

int32 UMounteaDialogueHUDStatics::GetOptionIndex(UObject* ContainerObject, const UUserWidget* OptionWidget)
{
	if (!IsValid(ContainerObject) || !IsValid(OptionWidget))
		return INDEX_NONE;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);
	if (!containerInterface.GetObject())
		return INDEX_NONE;

	TArray<UUserWidget*> dialogueOptions = containerInterface->Execute_GetDialogueOptions(ContainerObject);
	return dialogueOptions.Find(const_cast<UUserWidget*>(OptionWidget));
}

void UMounteaDialogueHUDStatics::ToggleForcedFocus(UObject* ContainerObject, bool bIsEnabled)
{
	if (!IsValid(ContainerObject))
		return;

	auto containerInterface = GetOptionsContainerInterface(ContainerObject);
	if (!containerInterface.GetObject())
		return;

	containerInterface->Execute_ToggleForcedFocus(ContainerObject, bIsEnabled);
}

FWidgetDialogueRow UMounteaDialogueHUDStatics::GetDialogueWidgetRowData(UObject* RowObject)
{
	if (!IsValid(RowObject))
		return FWidgetDialogueRow();

	if (RowObject->Implements<UMounteaDialogueRowInterface>())
		return IMounteaDialogueRowInterface::Execute_GetDialogueWidgetRowData(RowObject);

	return FWidgetDialogueRow();
}

void UMounteaDialogueHUDStatics::SetNewWidgetDialogueRowData(UObject* RowObject, const FWidgetDialogueRow& NewData)
{
	if (!IsValid(RowObject))
		return;

	if (RowObject->Implements<UMounteaDialogueRowInterface>())
		return IMounteaDialogueRowInterface::Execute_SetNewWidgetDialogueRowData(RowObject, NewData);
}

void UMounteaDialogueHUDStatics::ResetWidgetDialogueRow(UObject* RowObject)
{
	if (!IsValid(RowObject))
		return;

	if (RowObject->Implements<UMounteaDialogueRowInterface>())
		return IMounteaDialogueRowInterface::Execute_ResetWidgetDialogueRow(RowObject);
}

void UMounteaDialogueHUDStatics::InitializeWidgetDialogueRow(UObject* RowObject)
{
	if (!IsValid(RowObject))
		return;

	if (RowObject->Implements<UMounteaDialogueRowInterface>())
		return IMounteaDialogueRowInterface::Execute_InitializeWidgetDialogueRow(RowObject);
}

void UMounteaDialogueHUDStatics::StopTypeWriterEffect(UObject* RowObject)
{
	if (!IsValid(RowObject))
		return;

	if (RowObject->Implements<UMounteaDialogueRowInterface>())
		return IMounteaDialogueRowInterface::Execute_StopTypeWriterEffect(RowObject);
}

void UMounteaDialogueHUDStatics::StartTypeWriterEffect(UObject* RowObject, const FText& SourceText, float Duration)
{
	if (!IsValid(RowObject))
		return;

	if (RowObject->Implements<UMounteaDialogueRowInterface>())
		return IMounteaDialogueRowInterface::Execute_StartTypeWriterEffect(RowObject, SourceText, Duration);
}

void UMounteaDialogueHUDStatics::EnableTypeWriterEffect(UObject* RowObject, bool bEnable)
{
	if (!IsValid(RowObject))
		return;

	if (RowObject->Implements<UMounteaDialogueRowInterface>())
		return IMounteaDialogueRowInterface::Execute_EnableTypeWriterEffect(RowObject, bEnable);
}

void UMounteaDialogueHUDStatics::RequestShowWidget(UObject* SkipObject, const FVector2D& FadeProgressDuration)
{
	if (!IsValid(SkipObject))
		return;

	if (SkipObject->Implements<UMounteaDialogueSkipInterface>())
		return IMounteaDialogueSkipInterface::Execute_RequestShowWidget(SkipObject, FadeProgressDuration);
}

void UMounteaDialogueHUDStatics::RequestHideWidget(UObject* SkipObject)
{
	if (!IsValid(SkipObject))
		return;

	if (SkipObject->Implements<UMounteaDialogueSkipInterface>())
		return IMounteaDialogueSkipInterface::Execute_RequestHideWidget(SkipObject);
}

TScriptInterface<IMounteaDialogueOptionsContainerInterface> UMounteaDialogueHUDStatics::GetOptionsContainerInterface(UObject* ContainerObject)
{
	TScriptInterface<IMounteaDialogueOptionsContainerInterface> containerInterface;
	containerInterface.SetObject(ContainerObject);
	containerInterface.SetInterface(Cast<IMounteaDialogueOptionsContainerInterface>(ContainerObject));

	return containerInterface;
}
