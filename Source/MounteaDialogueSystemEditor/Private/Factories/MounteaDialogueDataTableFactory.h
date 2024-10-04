// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MounteaDialogueDataTableFactory.generated.h"

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueDataTableFactory : public UFactory
{
	GENERATED_BODY()

public:

	UMounteaDialogueDataTableFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	
	TObjectPtr<const class UScriptStruct> Struct;
};
