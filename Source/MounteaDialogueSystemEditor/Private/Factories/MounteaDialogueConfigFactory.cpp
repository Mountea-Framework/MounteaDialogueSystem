// All rights reserved Dominik Morse (Pavlicek) 2024


#include "MounteaDialogueConfigFactory.h"

#include "Helpers/MounteaDialogueGraphEditorUtilities.h"
#include "Settings/MounteaDialogueConfiguration.h"

UMounteaDialogueConfigFactory::UMounteaDialogueConfigFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UMounteaDialogueConfiguration::StaticClass();
}

UObject* UMounteaDialogueConfigFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMounteaDialogueConfiguration>(InParent, ParentClass, Name, Flags, Context);
}

bool UMounteaDialogueConfigFactory::ConfigureProperties()
{
	static const FText TitleText = FText::FromString(TEXT("Pick Parent Class for new Mountea Dialogue Configuration"));
	
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = FMounteaDialogueGraphEditorUtilities::PickChildrenOfClass(TitleText, ChosenClass, UMounteaDialogueConfiguration::StaticClass());

	if ( bPressedOk )
	{
		ParentClass = ChosenClass;
	}

	return bPressedOk;
}
