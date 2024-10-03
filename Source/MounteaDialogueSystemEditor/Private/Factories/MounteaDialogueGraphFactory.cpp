// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraphFactory.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"
#include "Widgets/Notifications/SNotificationList.h"

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
	{
		// Error notification
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
		FText::Format(LOCTEXT("PreImportFailed", "Unable to import Dialogue:\n{0}"), FText::FromString(InName.ToString())),
		5.f,
		TEXT("MDSStyleSet.Info.Error"));
		bOutOperationCanceled = true;
		return nullptr;
	}

	FString outMessage = TEXT("none");
	if (UMounteaDialogueSystemImportExportHelpers::IsReimport(Filename))
	{
		UMounteaDialogueGraph* ExistingGraph = nullptr;
		if (UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(Filename,InParent, ExistingGraph, outMessage))
		{
			if (ExistingGraph)
			{
				// Success notification
				UMounteaDialogueSystemImportExportHelpers::ShowNotification(
				FText::Format(LOCTEXT("ReImportSuccess", "Successfully reimported Dialogue:\n{0}\n\nAdditional info: {1}"), FText::FromString(InName.ToString()), FText::FromString(outMessage)),
				5.f,
				TEXT("MDSStyleSet.Icon.Success"));
				return ExistingGraph;
			}
		}
		else
		{
			// Error notification
			UMounteaDialogueSystemImportExportHelpers::ShowNotification(
			FText::Format(LOCTEXT("ReImportFailed", "Failed to reimport Dialogue:\n{0}\n\nAdditional info: {1}"), FText::FromString(InName.ToString()), FText::FromString(outMessage)),
			5.f,
			TEXT("MDSStyleSet.Info.Error"));
			
			bOutOperationCanceled = true;
			return nullptr;
		}
	}
	
	UMounteaDialogueGraph* NewGraph = NewObject<UMounteaDialogueGraph>(InParent, InClass, InName, Flags);
	if (UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraph(Filename, InParent, InName, Flags, NewGraph, outMessage))
	{
		// Success notification
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
		FText::Format(LOCTEXT("ImportSuccessful", "Successfully imported Dialogue:\n{0}\n\nAdditional info: {1}"), FText::FromString(InName.ToString()), FText::FromString(outMessage)),
		5.f,
		TEXT("MDSStyleSet.Icon.Success"));
		
		return NewGraph;
	}
	else
	{
		// Error notification
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
		FText::Format(LOCTEXT("ImportFailed", "Failed to import Dialogue:\n{0}\n\nAdditional info: {1}"), FText::FromString(InName.ToString()), FText::FromString(outMessage)),
		5.f,
		TEXT("MDSStyleSet.Info.Error"));
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

bool UMounteaDialogueGraphFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	return UMounteaDialogueSystemImportExportHelpers::CanReimport(Obj,OutFilenames );
}

void UMounteaDialogueGraphFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	ReimportPaths = NewReimportPaths;
}

EReimportResult::Type UMounteaDialogueGraphFactory::Reimport(UObject* Obj)
{
	FString outMessage;
	
	if (!Obj)
		return EReimportResult::Failed;

	UObjectRedirector* objectRedirector = Cast<UObjectRedirector>(Obj);
	UMounteaDialogueGraph* targetGraph = Cast<UMounteaDialogueGraph>(Obj);
	if (!objectRedirector && !targetGraph)
	{
		return EReimportResult::Failed;
	}

	if (!targetGraph)
	{
		targetGraph = Cast<UMounteaDialogueGraph>(objectRedirector->DestinationObject);
	}

	if (!targetGraph)
	{
		return EReimportResult::Failed;;
	}
	
	const bool bReimportSuccess = UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(ReimportPaths[0], Obj, targetGraph, outMessage);

	// This is extra notification added to the native Unreal one
	if (bReimportSuccess)
	{
		// Success notification
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
		FText::Format(LOCTEXT("ReImportSuccessful", "Successfully reimported Dialogue:\n{0}\n\nAdditional info: {1}"), FText::FromString(targetGraph->GetName()), FText::FromString(outMessage)),
		5.f,
		TEXT("MDSStyleSet.Icon.Success"));
	}
	else
	{
		// Error notification
		UMounteaDialogueSystemImportExportHelpers::ShowNotification(
		FText::Format(LOCTEXT("ReImportFailed", "Failed to reimported Dialogue:\n{0}\n\nAdditional info: {1}"), FText::FromString(targetGraph->GetName()), FText::FromString(outMessage)),
		5.f,
		TEXT("MDSStyleSet.Info.Error"));
	}
	
	return bReimportSuccess ? EReimportResult::Succeeded : EReimportResult::Failed;
}

int32 UMounteaDialogueGraphFactory::GetPriority() const
{
	return 1000;
}

void UMounteaDialogueGraphFactory::PostImportCleanUp()
{
	FReimportHandler::PostImportCleanUp();

	CleanUp();

	ReimportPaths.Empty();
}

bool UMounteaDialogueGraphFactory::FactoryCanImport(const FString& Filename)
{
	const FString extension = FPaths::GetExtension(Filename);
	if (!(extension == TEXT("mnteadlg") || extension == TEXT("zip")))
	{
		return false;
	}
	
	return true;
}

#undef LOCTEXT_NAMESPACE
