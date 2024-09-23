// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "IAssetTools.h"
#include "Factories/Factory.h"
#include "MounteaDialogueGraphFactory.generated.h"

class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class UStringTable;
class UDataTable;

UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	UMounteaDialogueGraphFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual UObject* ImportObject(UClass* InClass, UObject* InOuter, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, bool& OutCanceled) override;
	// End of UFactory interface
};
