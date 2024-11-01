// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueScriptedAction.h"

#include "ContentBrowserModule.h"
#include "EditorUtilityLibrary.h"
#include "IPythonScriptPlugin.h"
#include "WidgetBlueprint.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueScriptedAction"

static TSharedRef<FExtender> ExtendMenu(const TArray<FString>& SelectedPaths)
{
	return MakeShared<FExtender>();
}

void FMounteaDialogueScriptedAction::Register()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedPaths>& MenuExtenderDelegates = ContentBrowserModule.GetAllPathViewContextMenuExtenders();

	MenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedPaths::CreateStatic(&ExtendMenu));
}

void FMounteaDialogueScriptedAction::Unregister()
{
}

void FMounteaDialogueScriptedAction::ExecutePythonFixer()
{
	if (IPythonScriptPlugin::Get()->IsPythonAvailable())
	{
		TArray<FString> ObjectPaths;
		const TArray<FAssetData> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssetData();
		
		// Check if we have any Blueprints or Widget Blueprints
		for (const FAssetData& Asset : SelectedAssets)
		{
			if (Asset.GetClass()->IsChildOf(UBlueprint::StaticClass()) || 
				Asset.GetClass()->IsChildOf(UWidgetBlueprint::StaticClass()))
			{
				ObjectPaths.Add(Asset.GetObjectPathString());
			}
		}

		if (ObjectPaths.Num() > 0)
		{
			FString PathsStr = FString::Join(ObjectPaths, TEXT("','"));
			PathsStr = FString::Printf(TEXT("['%s']"), *PathsStr);

			FString PythonCommand = FString::Printf(TEXT("import mountea_node_replacer; mountea_node_replacer.process_assets(%s)"), *PathsStr);
			IPythonScriptPlugin::Get()->ExecPythonCommand(*PythonCommand);
			
			// Log for feedback
			UE_LOG(LogTemp, Log, TEXT("Processing %d assets for Mountea node fixes"), ObjectPaths.Num());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No valid blueprints selected for Mountea node fixes"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Python is not available. Cannot execute Mountea node fixes"));
	}
}

bool FMounteaDialogueScriptedAction::CanExecute()
{
	// Check if any selected assets are Blueprints or Widget Blueprints
	const TArray<FAssetData> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssetData();
	
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (Asset.GetClass()->IsChildOf(UBlueprint::StaticClass()) || 
			Asset.GetClass()->IsChildOf(UWidgetBlueprint::StaticClass()))
		{
			return true;
		}
	}
	
	return false;
}

#undef LOCTEXT_NAMESPACE
