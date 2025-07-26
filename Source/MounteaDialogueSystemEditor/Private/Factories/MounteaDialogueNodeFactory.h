// All rights reserved Dominik Morse 2024

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MounteaDialogueNodeFactory.generated.h"

class UMounteaDialogueGraphNode;

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueNodeFactory : public UFactory
{
	GENERATED_BODY()

	UMounteaDialogueNodeFactory();
	
public:
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ConfigureProperties() override;

private:
	// Holds the template of the class we are building
	UPROPERTY()
	TSubclassOf<UMounteaDialogueGraphNode> ParentClass;
};
