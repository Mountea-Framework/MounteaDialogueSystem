// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueOptionsContainer.h"

#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueHUDStatics.h"

#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"

#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

UMounteaDialogueOptionsContainer::UMounteaDialogueOptionsContainer(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer), FocusedOption(INDEX_NONE), LastFocusedOption(INDEX_NONE)
{
	SetIsFocusable(true);
}

void UMounteaDialogueOptionsContainer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TArray<TObjectPtr<UUserWidget>> dialogueOptions;
	DialogueOptions.GenerateValueArray(dialogueOptions);

	TObjectPtr<UUserWidget> focusableWidget = nullptr;
	if (dialogueOptions.Num() > 0)
	{
		if (dialogueOptions.IsValidIndex(FocusedOption))
			focusableWidget = dialogueOptions[FocusedOption];
		else if (dialogueOptions.IsValidIndex(LastFocusedOption))
			focusableWidget = dialogueOptions[LastFocusedOption];
		else
			focusableWidget = nullptr;

		if (focusableWidget != nullptr)
		{
			IMounteaFocusableWidgetInterface::Execute_SetFocusState(focusableWidget, true);
		}
	}
}

void UMounteaDialogueOptionsContainer::ClearChildOptionFocus(UUserWidget* Target)
{
	if (!IsValid(Target))
		return;
	
	if (Target && Target->Implements<UMounteaFocusableWidgetInterface>())
		IMounteaFocusableWidgetInterface::Execute_SetFocusState(Target, false);
}

void UMounteaDialogueOptionsContainer::ClearChildOptionsFocus()
{
	TArray<TObjectPtr<UUserWidget>> optionWidgets;
	DialogueOptions.GenerateValueArray(optionWidgets);
	
	for (const auto& optionWidget : optionWidgets)
	{
		ClearChildOptionFocus(optionWidget);
	}
}

void UMounteaDialogueOptionsContainer::ResetFocus(const UUserWidget* Requestor)
{
	ClearChildOptionsFocus();

	auto newFocus = UMounteaDialogueHUDStatics::GetOptionIndex(this, Requestor);
	if (newFocus == INDEX_NONE)
		return;

	Execute_SetFocusedOption(this, newFocus);
}

void UMounteaDialogueOptionsContainer::SetParentDialogueWidget_Implementation(UUserWidget* NewParentDialogueWidget)
{
	if (NewParentDialogueWidget != ParentDialogueWidget)
	{
		ParentDialogueWidget = NewParentDialogueWidget;
	}
}

UUserWidget* UMounteaDialogueOptionsContainer::GetParentDialogueWidget_Implementation() const
{
	return ParentDialogueWidget;
}

TSoftClassPtr<UUserWidget> UMounteaDialogueOptionsContainer::GetDialogueOptionClass_Implementation() const
{
	return DialogueOptionClass;
}

void UMounteaDialogueOptionsContainer::SetDialogueOptionClass_Implementation(const TSoftClassPtr<UUserWidget>& NewDialogueOptionClass)
{
	if (NewDialogueOptionClass.LoadSynchronous() != DialogueOptionClass.LoadSynchronous())
		DialogueOptionClass = NewDialogueOptionClass;
}

void UMounteaDialogueOptionsContainer::AddNewDialogueOption_Implementation(UMounteaDialogueGraphNode_DialogueNodeBase* NewDialogueOption)
{
	if (!NewDialogueOption)
	{
		LOG_ERROR(TEXT("[AddNewDialogueOption] Invalid Dialogue Option (Node)!"))
		return;
	}
	
	TObjectPtr<UUserWidget> dialogueOptionWidget =
	DialogueOptions.Contains(NewDialogueOption->GetNodeGUID())
	? DialogueOptions.FindRef(NewDialogueOption->GetNodeGUID())
	: TObjectPtr<UUserWidget>(CreateWidget<UUserWidget>(GetOwningPlayer(), DialogueOptionClass.LoadSynchronous()));

	
	if (dialogueOptionWidget)
	{		
		TScriptInterface<IMounteaDialogueOptionInterface> dialogueOption = dialogueOptionWidget;
		if (dialogueOption.GetObject() && dialogueOption.GetInterface())
		{
			dialogueOption->GetDialogueOptionSelectedHandle().AddUniqueDynamic(this, &UMounteaDialogueOptionsContainer::ProcessOptionSelected);
			dialogueOption->Execute_SetNewDialogueOptionData(dialogueOptionWidget, FDialogueOptionData( UMounteaDialogueHUDStatics::GetDialogueNodeGuid(NewDialogueOption),  UMounteaDialogueHUDStatics::GetDialogueNodeRow(NewDialogueOption)));
			dialogueOption->Execute_InitializeDialogueOption(dialogueOptionWidget);
		}
		TScriptInterface<IMounteaFocusableWidgetInterface> focusableDialogueOption = dialogueOptionWidget;
		{
			if (focusableDialogueOption.GetObject() && focusableDialogueOption.GetInterface())
			{
				focusableDialogueOption->GetOnMounteaFocusClearRequestedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueOptionsContainer::ResetFocus);
			}
		}
	}
	else
	{
		LOG_ERROR(TEXT("[AddNewDialogueOption] Failed to create new dialogue option!"))
		return;
	}
	
	DialogueOptions.Add(UMounteaDialogueHUDStatics::GetDialogueNodeGuid(NewDialogueOption), dialogueOptionWidget);
}

void UMounteaDialogueOptionsContainer::AddNewDialogueOptions_Implementation(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& NewDialogueOptions)
{
	for (const auto& Itr : NewDialogueOptions)
	{
		Execute_AddNewDialogueOption(this, Itr);
	}
}

void UMounteaDialogueOptionsContainer::RemoveDialogueOption_Implementation(UMounteaDialogueGraphNode_DialogueNodeBase* DirtyDialogueOption)
{
	if (DirtyDialogueOption)
	{
		if (TObjectPtr<UUserWidget> dirtyOptionWidget = DialogueOptions.FindRef(UMounteaDialogueHUDStatics::GetDialogueNodeGuid(DirtyDialogueOption)))
		{
			TScriptInterface<IMounteaDialogueOptionInterface> dialogueOption = dirtyOptionWidget;
			if (dialogueOption.GetObject() && dialogueOption.GetInterface())
			{
				dialogueOption->GetDialogueOptionSelectedHandle().RemoveDynamic(this, &UMounteaDialogueOptionsContainer::ProcessOptionSelected);
				dialogueOption->Execute_ResetDialogueOptionData(dirtyOptionWidget);
			}
			TScriptInterface<IMounteaFocusableWidgetInterface> focusableDialogueOption = dirtyOptionWidget;
			{
				if (focusableDialogueOption.GetObject() && focusableDialogueOption.GetInterface())
				{
					focusableDialogueOption->GetOnMounteaFocusClearRequestedEventHandle().RemoveDynamic(this, &UMounteaDialogueOptionsContainer::ResetFocus);
				}

			}
		}
		
	}
	DialogueOptions.Remove(UMounteaDialogueHUDStatics::GetDialogueNodeGuid(DirtyDialogueOption));
}

void UMounteaDialogueOptionsContainer::RemoveDialogueOptions_Implementation(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& DirtyDialogueOptions)
{
	for (const auto& Itr : DirtyDialogueOptions)
	{
		Execute_RemoveDialogueOption(this, Itr);
	}
}

void UMounteaDialogueOptionsContainer::ClearDialogueOptions_Implementation()
{
	for (const auto& Itr : DialogueOptions)
	{
		TScriptInterface<IMounteaDialogueOptionInterface> dialogueOption = Itr.Value;

		if (dialogueOption.GetObject() && dialogueOption.GetInterface())
		{
			dialogueOption->GetDialogueOptionSelectedHandle().RemoveDynamic(this, &UMounteaDialogueOptionsContainer::ProcessOptionSelected);
			dialogueOption->Execute_ResetDialogueOptionData( Itr.Value);
		}
	}

	DialogueOptions.Empty();
}

void UMounteaDialogueOptionsContainer::ProcessOptionSelected_Implementation(const FGuid& SelectedOption,UUserWidget* CallingWidget)
{
	if (ParentDialogueWidget)
	{
		TScriptInterface<IMounteaDialogueWBPInterface> dialogueInterface = ParentDialogueWidget;
		dialogueInterface->Execute_OnOptionSelected(ParentDialogueWidget, SelectedOption);
	}
}

TArray<UUserWidget*> UMounteaDialogueOptionsContainer::GetDialogueOptions_Implementation() const
{
	TArray<TObjectPtr<UUserWidget>> dialogueOptions;
	for (const auto dialogueOption : DialogueOptions)
		dialogueOptions.Add(dialogueOption.Value);

	return dialogueOptions;
}

int32 UMounteaDialogueOptionsContainer::GetFocusedOptionIndex_Implementation() const
{
	return FocusedOption;
}

void UMounteaDialogueOptionsContainer::SetFocusedOption_Implementation(const int32 NewFocusedOption)
{
	if (NewFocusedOption == FocusedOption)
		return;

	LastFocusedOption = FocusedOption;

	TArray<TObjectPtr<UUserWidget>> optionWidgets;
	DialogueOptions.GenerateValueArray(optionWidgets);
	
	if (!optionWidgets.IsValidIndex(NewFocusedOption))
		return;

	UUserWidget* foundWidget = optionWidgets[NewFocusedOption].Get();
	if (!IsValid(foundWidget))
		return;

	FocusedOption = NewFocusedOption;

	ClearChildOptionsFocus();

	if (foundWidget->Implements<UMounteaFocusableWidgetInterface>())
		IMounteaFocusableWidgetInterface::Execute_SetFocusState(foundWidget, true);
}
