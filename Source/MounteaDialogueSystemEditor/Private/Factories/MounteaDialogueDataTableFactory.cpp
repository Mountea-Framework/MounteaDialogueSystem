// All rights reserved Dominik Morse (Pavlicek) 2024


#include "MounteaDialogueDataTableFactory.h"

#include "Data/MounteaDialogueDataTable.h"
#include "Data/MounteaDialogueGraphDataTypes.h"

UMounteaDialogueDataTableFactory::UMounteaDialogueDataTableFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UMounteaDialogueDataTable::StaticClass();
	Struct = FDialogueRow::StaticStruct();
}

UObject* UMounteaDialogueDataTableFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UDataTable* DialogueTable = nullptr;
	if (Struct && ensure(SupportedClass == Class))
	{
		ensure(0 != (RF_Public & Flags));
		DialogueTable = NewObject<UDataTable>(InParent, Name, Flags);
		if (DialogueTable)
		{
			DialogueTable->RowStruct = const_cast<UScriptStruct*>(ToRawPtr(Struct));
		}
	}

	return DialogueTable;
}
