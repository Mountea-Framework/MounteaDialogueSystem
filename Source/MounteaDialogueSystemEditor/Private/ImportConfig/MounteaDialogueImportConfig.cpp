// All rights reserved Dominik Morse (Pavlicek) 2024


#include "MounteaDialogueImportConfig.h"

#include "Interfaces/IPluginManager.h"

UMounteaDialogueImportConfig::UMounteaDialogueImportConfig(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System (Import)");

	const FString configPath = GetImportConfigFilePath();
	if (FPaths::FileExists(configPath))
		LoadConfig(nullptr, *configPath);
}

bool UMounteaDialogueImportConfig::IsReimport(const FGuid& GraphGuid) const
{
	return ImportHistory.Contains(GraphGuid);
}

FString UMounteaDialogueImportConfig::WriteToConfig(const FGuid& DialogueGuid, const FDialogueImportSourceData& NewSourceData)
{
	FString outputResult;
	if (ImportHistory.Contains(DialogueGuid)) outputResult = TEXT("UPDATE");
	else outputResult = TEXT("CREATE");

	ImportHistory.Add(DialogueGuid, NewSourceData);
	
	return outputResult;
}
