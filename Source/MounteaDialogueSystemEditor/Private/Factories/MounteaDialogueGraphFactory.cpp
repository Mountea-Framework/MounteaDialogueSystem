// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraphFactory.h"

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
		FNotificationInfo Info(FText::Format(LOCTEXT("PreImportFailed", "Unable to import Dialogue:\n{0}"), FText::FromString(InName.ToString())));
		Info.ExpireDuration = 5.0f;
		Info.Image = FAppStyle::GetBrush(TEXT("MDSStyleSet.Icon.Error"));
		FSlateNotificationManager::Get().AddNotification(Info);

		return nullptr;
	}

	if (UMounteaDialogueSystemImportExportHelpers::IsReimport(Filename))
	{
		UMounteaDialogueGraph* ExistingGraph = nullptr;
		if (UMounteaDialogueSystemImportExportHelpers::ReimportDialogueGraph(Filename,ExistingGraph ))
		{
			if (ExistingGraph)
			{
				return ExistingGraph;
			}
		}
		else
		{
			// Error notification
			FNotificationInfo Info(FText::Format(LOCTEXT("ReImportFailed", "Failed to reimport Dialogue:\n{0}"), FText::FromString(InName.ToString())));
			Info.ExpireDuration = 5.0f;
			Info.Image = FAppStyle::GetBrush(TEXT("MDSStyleSet.Icon.Error"));
			FSlateNotificationManager::Get().AddNotification(Info);
			
			bOutOperationCanceled = true;
			return nullptr;
		}
	}
	
	UMounteaDialogueGraph* NewGraph = NewObject<UMounteaDialogueGraph>(InParent, InClass, InName, Flags);
	if (UMounteaDialogueSystemImportExportHelpers::ImportDialogueGraph(Filename, InParent, InName, Flags, NewGraph))
	{
		// Success notification
		FNotificationInfo Info(FText::Format(LOCTEXT("ImportSuccessful", "Successfully imported Dialogue:\n{0}"), FText::FromString(InName.ToString())));
		Info.ExpireDuration = 5.0f;
		Info.Image = FAppStyle::GetBrush(TEXT("MDSStyleSet.Icon.Success"));
		FSlateNotificationManager::Get().AddNotification(Info);
		
		return NewGraph;
	}
	else
	{
		// Error notification
		FNotificationInfo Info(FText::Format(LOCTEXT("ImportFailed", "Failed to import Dialogue:\n{0}"), FText::FromString(InName.ToString())));
		Info.ExpireDuration = 5.0f;
		Info.Image = FAppStyle::GetBrush(TEXT("MDSStyleSet.Icon.Error"));
		FSlateNotificationManager::Get().AddNotification(Info);
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
