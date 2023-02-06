#include "MounteaDialogueGraphFactory.h"

#include "MounteaDialogueGraph.h"

UMounteaDialogueGraphFactory::UMounteaDialogueGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UMounteaDialogueGraph::StaticClass();
}

UObject* UMounteaDialogueGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	Name = FName("NewMounteaDialogueTree");
	
	return NewObject<UObject>(InParent, Class, Name, Flags | RF_Transactional);
}