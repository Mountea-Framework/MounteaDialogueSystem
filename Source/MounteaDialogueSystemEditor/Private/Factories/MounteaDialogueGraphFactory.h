#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MounteaDialogueGraphFactory.generated.h"

UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueGraphFactory  : public UFactory
{
	GENERATED_BODY()
	
public:

	UMounteaDialogueGraphFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	
};
