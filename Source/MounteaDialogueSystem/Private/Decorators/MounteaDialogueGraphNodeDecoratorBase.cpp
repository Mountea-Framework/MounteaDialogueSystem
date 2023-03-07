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