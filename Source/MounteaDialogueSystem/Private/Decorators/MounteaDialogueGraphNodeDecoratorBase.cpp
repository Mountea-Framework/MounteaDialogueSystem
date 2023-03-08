// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueGraphNodeDecoratorBase.h"

bool UMounteaDialogueGraphNodeDecoratorBase::EvaluateDecorator_Implementation()
{
	return OwningWorld != nullptr;
}

void UMounteaDialogueGraphNodeDecoratorBase::ExecuteDecorator_Implementation()
{
	return;
}

UMounteaDialogueGraphNode* UMounteaDialogueGraphNodeDecoratorBase::GetOwningNode() const
{
	return GetTypedOuter<UMounteaDialogueGraphNode>();
}

UMounteaDialogueGraph* UMounteaDialogueGraphNodeDecoratorBase::GetOwningGraph() const
{
	return GetTypedOuter<UMounteaDialogueGraph>();
}

UObject* UMounteaDialogueGraphNodeDecoratorBase::GetOwner() const
{
	return GetOuter();
}
