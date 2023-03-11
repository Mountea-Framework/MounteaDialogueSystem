// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecoratorBase.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#define LOCTEXT_NAMESPACE "MounteaDialogueDecoratorBase"

bool UMounteaDialogueDecoratorBase::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = true;
	const FText Name = FText::FromString(GetName());

	// This is to ensure we are not throwing InvalidWorld errors in Editor with no Gameplay.
	bool bIsEditorCall = false;
#if WITH_EDITOR
	if (GEditor != nullptr)
	{
		bIsEditorCall = !GEditor->GetPlayInEditorSessionInfo().IsSet();
	}
#endif
	
	if (GetOwningWorld() == nullptr && bIsEditorCall == false)
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_Base_Validation_World", "[{0}]: No valid World!"), Name);
		ValidationMessages.Add(TempText);

		bSatisfied = false;
	}

	
	if (GetOwner() == nullptr)
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_Base_Validation_Owner", "[{0}]: No valid Owner!"), Name);
		ValidationMessages.Add(TempText);

		bSatisfied = false;
	}
		
	return bSatisfied;
}

bool UMounteaDialogueDecoratorBase::EvaluateDecorator_Implementation()
{
	return OwningWorld != nullptr;
}

void UMounteaDialogueDecoratorBase::ExecuteDecorator_Implementation()
{
	return;
}

UMounteaDialogueGraphNode* UMounteaDialogueDecoratorBase::GetOwningNode() const
{
	return GetTypedOuter<UMounteaDialogueGraphNode>();
}

UMounteaDialogueGraph* UMounteaDialogueDecoratorBase::GetOwningGraph() const
{
	return GetTypedOuter<UMounteaDialogueGraph>();
}

UObject* UMounteaDialogueDecoratorBase::GetOwner() const
{
	return GetOuter();
}

#undef LOCTEXT_NAMESPACE