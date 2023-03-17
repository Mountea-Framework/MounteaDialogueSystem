#include "MounteaDialogueGraphFactory.h"

#include "Graph/MounteaDialogueGraph.h"

UMounteaDialogueGraphFactory::UMounteaDialogueGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UMounteaDialogueGraph::StaticClass();
}

UObject* UMounteaDialogueGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	/*
	FString NewName;
	Name.ToString(NewName);

	NewName = NewName.Replace(TEXT("Graph"), TEXT("Tree"));

	Name = FName(*NewName); 
	*/
	return NewObject<UObject>(InParent, Class, Name, Flags | RF_Transactional);
}