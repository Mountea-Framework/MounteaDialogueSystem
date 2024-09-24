// All rights reserved Dominik Pavlicek 2023


#include "MounteaDialogueGraphAssetAction.h"

#include "AssetEditor/AssetEditor_MounteaDialogueGraph.h"
#include "DesktopPlatformModule.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueNodeAssetAction"

FMounteaDialogueGraphAssetAction::FMounteaDialogueGraphAssetAction()
{
}

FText FMounteaDialogueGraphAssetAction::GetName() const
{
	return LOCTEXT("MounteaDialogueGraphAssetAction", "Mountea Dialogue Tree");
}

/*
FText FMounteaDialogueGraphAssetAction::GetAssetDescription(const FAssetData& AssetData) const
{
	return LOCTEXT("MounteaDialogueGraphAssetAction", "Mountea Dialogue Tree Asset for creating Mountea Dialogues.");
}
*/

FColor FMounteaDialogueGraphAssetAction::GetTypeColor() const
{
	return FColor::Orange;
}

UClass* FMounteaDialogueGraphAssetAction::GetSupportedClass() const
{
	return UMounteaDialogueGraph::StaticClass();
}

void FMounteaDialogueGraphAssetAction::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UMounteaDialogueGraph* Graph = Cast<UMounteaDialogueGraph>(*ObjIt))
		{
			TSharedRef<FAssetEditor_MounteaDialogueGraph> NewGraphEditor(new FAssetEditor_MounteaDialogueGraph());
			NewGraphEditor->InitMounteaDialogueGraphAssetEditor(Mode, EditWithinLevelEditor, Graph);
		}
	}
}

uint32 FMounteaDialogueGraphAssetAction::GetCategories()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return FAssetToolsModule::GetModule().Get().FindAdvancedAssetCategory(FName("Mountea Dialogue"));
	}

	return EAssetTypeCategories::Misc;
}

void FMounteaDialogueGraphAssetAction::GetActions(const TArray<UObject *> &InObjects, FToolMenuSection &Section)
{
	FAssetTypeActions_Base::GetActions(InObjects, Section);

	auto DialogueGraphs = GetTypedWeakObjectPtrs<UObject>(InObjects);

	Section.AddMenuEntry(
		"MounteaDialogueGraph_ExportGraph",
		LOCTEXT("MounteaDialogueGraph_ExportGraphName", "Export Dialogue Graph"),
		LOCTEXT("MounteaDialogueGraph_ExportGraphTooltip", "Export the Dialogue Graph as a file containing MNTEADLG data."),
		FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.ExportGraph"),
		FUIAction(
			FExecuteAction::CreateSP( this, &FMounteaDialogueGraphAssetAction::ExecuteExportDialogue, DialogueGraphs ),
			FCanExecuteAction()
			)
		);

	// TODO: Enable only of SourceData contains valid path
	Section.AddMenuEntry(
		"MounteaDialogueGraph_ReimportGraph",
		LOCTEXT("MounteaDialogueGraph_ReimportGraphName", "Reimport Dialogue Graph"),
		LOCTEXT("MounteaDialogueGraph_ReimportGraphTooltip", "Tries to reimport Dialogue Graph from saved source."),
		FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.ReimportGraph"),
		FUIAction(
			FExecuteAction::CreateSP( this, &FMounteaDialogueGraphAssetAction::ExecuteReimportDialogue, DialogueGraphs ),
			FCanExecuteAction()
			)
		);
}

void FMounteaDialogueGraphAssetAction::ExecuteExportDialogue(TArray<TWeakObjectPtr<UObject>> Objects)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto DialogueGraph = Cast<UMounteaDialogueGraph>((*ObjIt).Get());
		if (!DialogueGraph) continue;

		const FText Title = FText::Format(LOCTEXT("MounteaDialogueGraph_ExportGraphDialogTitle", "Export '{0}' as MNTEADLG..."), FText::FromString(*DialogueGraph->GetName()));
		const FString CurrentFilename = DialogueGraph->GetName(); //TODO: Implement AssetImportData in Factory! //DialogueGraph->AssetImportData->GetFirstFilename();
		const FString FileTypes = TEXT("Mountea Dialogue Graph (*.mnteadlg)|*.mnteadlg");

		TArray<FString> OutFilenames;
		DesktopPlatform->SaveFileDialog(
			ParentWindowWindowHandle,
			Title.ToString(),
			(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetPath(CurrentFilename),
			(CurrentFilename.IsEmpty()) ? TEXT("") : FPaths::GetBaseFilename(CurrentFilename) + TEXT(".mnteadlg"),
			FileTypes,
			EFileDialogFlags::None,
			OutFilenames
			);

		if (OutFilenames.Num() > 0)
		{
			const FString& ChosenFilePath = OutFilenames[0];
			if (UMounteaDialogueSystemImportExportHelpers::ExportDialogueGraph(DialogueGraph, ChosenFilePath))
			{
				// Success notification
				FNotificationInfo Info(FText::Format(LOCTEXT("ExportSuccessful", "Successfully exported {0}"), FText::FromString(DialogueGraph->GetName())));
				Info.ExpireDuration = 5.0f;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				// Error notification
				FNotificationInfo Info(FText::Format(LOCTEXT("ExportFailed", "Failed to export {0}"), FText::FromString(DialogueGraph->GetName())));
				Info.ExpireDuration = 5.0f;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		}
	}
}

void FMounteaDialogueGraphAssetAction::ExecuteReimportDialogue(TArray<TWeakObjectPtr<UObject>> Objects)
{
};

#undef LOCTEXT_NAMESPACE
