// All rights reserved Dominik Pavlicek 2022.


#include "Factories/MounteaDialogueAdditionalDataAssetFactory.h"

#include "Data/DialogueAdditionalData.h"


UMounteaDialogueAdditionalDataAssetFactory::UMounteaDialogueAdditionalDataAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UDialogueAdditionalData::StaticClass();
}

UObject* UMounteaDialogueAdditionalDataAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDialogueAdditionalData>(InParent, Class, Name, Flags, Context);
}
