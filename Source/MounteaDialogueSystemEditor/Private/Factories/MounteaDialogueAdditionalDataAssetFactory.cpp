// All rights reserved Dominik Pavlicek 2022.


#include "Factories/MounteaDialogueAdditionalDataAssetFactory.h"

#include "Data/DialogueAdditionalData.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"


UMounteaDialogueAdditionalDataAssetFactory::UMounteaDialogueAdditionalDataAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UDialogueAdditionalData::StaticClass();
}

UObject* UMounteaDialogueAdditionalDataAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDialogueAdditionalData>(InParent, ParentClass, Name, Flags, Context);
}

bool UMounteaDialogueAdditionalDataAssetFactory::ConfigureProperties()
{
	static const FText TitleText = FText::FromString(TEXT("Pick Parent Class for new Mountea Dialogue Additional Data"));
	
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = FMounteaDialogueGraphEditorUtilities::PickChildrenOfClass(TitleText, ChosenClass, UDialogueAdditionalData::StaticClass());

	if ( bPressedOk )
	{
		ParentClass = ChosenClass;
	}

	return bPressedOk;
}