// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MounteaDialogueGraphFactory.generated.h"

class UMounteaDialogueGraph;

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

private:
	bool IsZipFile(const TArray<uint8>& FileData);
	bool ExtractFilesFromZip(const TArray<uint8>& ZipData, TMap<FString, FString>& OutExtractedFiles);
	FString BytesToString(const uint8* Bytes, int32 Count);
	bool ValidateExtractedContent(const TMap<FString, FString>& ExtractedFiles);
	bool PopulateGraphFromExtractedFiles(UMounteaDialogueGraph* Graph, const TMap<FString, FString>& ExtractedFiles);
	void ImportAudioFiles(const TMap<FString, FString>& ExtractedFiles, UObject* InParent, EObjectFlags Flags);

	// Helper functions for populating graph data
	bool PopulateDialogueData(UMounteaDialogueGraph* Graph, const FString& Json);
	bool PopulateCategories(UMounteaDialogueGraph* Graph, const FString& Json);
	bool PopulateParticipants(UMounteaDialogueGraph* Graph, const FString& Json);
	bool PopulateNodes(UMounteaDialogueGraph* Graph, const FString& Json);
	bool PopulateEdges(UMounteaDialogueGraph* Graph, const FString& Json);
	bool PopulateDialogueRows(UMounteaDialogueGraph* Graph, const FString& Json);
};
