// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecoratorBase.h"

bool UMounteaDialogueDecoratorBase::ValidateDecorator_Implementation()
{
	return GetOwner() != nullptr && GetOwningWorld() != nullptr;
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
