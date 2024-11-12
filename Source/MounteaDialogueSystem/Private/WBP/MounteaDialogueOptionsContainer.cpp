// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueOptionsContainer.h"

#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueHUDStatics.h"
#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "Interfaces/UMG/MounteaDialogueOptionInterface.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

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
