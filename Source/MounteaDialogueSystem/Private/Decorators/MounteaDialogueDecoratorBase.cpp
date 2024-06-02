// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecoratorBase.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#define LOCTEXT_NAMESPACE "MounteaDialogueDecoratorBase"

void UMounteaDialogueDecoratorBase::SetOwningManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager)
{
	OwningManager = NewOwningManager;
}

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

	if (DecoratorState == EDecoratorState::Uninitialized && bIsEditorCall == false)
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_Base_Validation_State", "[{0}]: Not Initialized properly!"), Name);
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
	if (!OwningManager)
	{
		LOG_ERROR(TEXT("[ExecuteDecorator] Decorator %s has no Manager!"), *GetDecoratorName().ToString())
	}
	
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

FText UMounteaDialogueDecoratorBase::GetDecoratorName() const
{
#if WITH_EDITORONLY_DATA
	return GetClass()->GetDisplayNameText();
#endif
	return FText::FromString(GetName());
}

void UMounteaDialogueDecoratorBase::RegisterTick_Implementation( const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().AddUniqueDynamic(this, &UMounteaDialogueDecoratorBase::TickMounteaEvent);
	}
}

void UMounteaDialogueDecoratorBase::UnregisterTick_Implementation( const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().RemoveDynamic(this, &UMounteaDialogueDecoratorBase::TickMounteaEvent);
	}
}

void UMounteaDialogueDecoratorBase::TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick, float DeltaTime)
{
	DecoratorTickEvent.Broadcast(SelfRef, ParentTick, DeltaTime);

	LOG_INFO(TEXT("[%s] %s"), *GetDecoratorName().ToString(), *(OwningManager != nullptr ? OwningManager.GetObject()->GetName() : TEXT("NO MANAGER")))
}

#undef LOCTEXT_NAMESPACE
