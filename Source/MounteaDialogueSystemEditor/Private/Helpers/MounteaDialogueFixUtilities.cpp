// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueFixUtilities.h"

#include "EditorUtilityLibrary.h"
#include "IPythonScriptPlugin.h"

void FMounteaDialogueFixUtilities::ExecutePythonFixer()
{
	if (IPythonScriptPlugin::Get()->IsPythonAvailable())
	{
		TArray<FString> ObjectPaths;
		for (const FAssetData& Asset : UEditorUtilityLibrary::GetSelectedAssetData())
		{
			ObjectPaths.Add(Asset.GetObjectPathString());
		}

		if (ObjectPaths.Num() > 0)
		{
			FString PathsStr = FString::Join(ObjectPaths, TEXT("','"));
			PathsStr = FString::Printf(TEXT("['%s']"), *PathsStr);
			FString PythonCommand = FString::Printf(TEXT("import mountea_node_replacer; mountea_node_replacer.process_assets(%s)"), *PathsStr);
			IPythonScriptPlugin::Get()->ExecPythonCommand(*PythonCommand);
		}
	}
}

bool FMounteaDialogueFixUtilities::CanExecute()
{
	return UEditorUtilityLibrary::GetSelectedAssetData().Num() > 0;
}
