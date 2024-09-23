// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraphFactory.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphFactory"

UMounteaDialogueGraphFactory::UMounteaDialogueGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UMounteaDialogueGraph::StaticClass();
	bEditorImport = true;
	Formats.Add("mnteadlg;Mountea Dialogue File");
}

UObject* UMounteaDialogueGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UObject>(InParent, Class, Name, Flags | RF_Transactional);
}

UObject* UMounteaDialogueGraphFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	if (!FactoryCanImport(Filename))
		return nullptr;

	UMounteaDialogueGraph* NewGraph = NewObject<UMounteaDialogueGraph>(InParent, InClass, InName, Flags);
	if (UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraph(Filename, InParent, InName, Flags, NewGraph))
	{
		return NewGraph;
	}

	bOutOperationCanceled = true;
	return nullptr;
}

UObject* UMounteaDialogueGraphFactory::ImportObject(UClass* InClass, UObject* InOuter, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, bool& OutCanceled)
{
	if (!FactoryCanImport(Filename))
		return nullptr;
	return FactoryCreateFile(InClass, InOuter, InName, Flags, Filename, Parms, nullptr, OutCanceled);
}

bool UMounteaDialogueGraphFactory::FactoryCanImport(const FString& Filename)
{
	const FString extension = FPaths::GetExtension(Filename);
	return (extension == TEXT("mnteadlg") || extension == TEXT("zip"));
}

#undef LOCTEXT_NAMESPACE
