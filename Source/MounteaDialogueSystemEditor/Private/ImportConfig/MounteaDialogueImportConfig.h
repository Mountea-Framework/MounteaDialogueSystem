// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphExtraDataTypes.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueImportConfig.generated.h"

/**
 * Mountea Dialogue System (Import)
 *
 * This is read-only config file.
 * This file stores information about import history. This way you can keep info about imports.
 * Serves also purpose of storing data for reimports, so we can map existing graph from folder A to imported graph in folder B.
 */
UCLASS(NotBlueprintable, NotBlueprintType, config=MounteaDialogueImportConfig, meta=(DisplayName = "Mountea Dialogue System (Import)"))
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueImportConfig final : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UMounteaDialogueImportConfig(const FObjectInitializer& ObjectInitializer);

	/** Canonical path: <ProjectDir>/Config/Import/MounteaDialogueImportConfig.json */
	static FString GetImportConfigFilePath()
	{
		return FPaths::ProjectDir() / TEXT("Config/Import/MounteaDialogueImportConfig.json");
	}

	bool IsReimport(const FGuid& GraphGuid) const;
	FString WriteToConfig(const FGuid& DialogueGuid, const FDialogueImportSourceData& NewSourceData);

	/** Serialize ImportHistory to JSON and write to GetImportConfigFilePath(). */
	void SaveToFile();

	/** Read GetImportConfigFilePath() and deserialize into ImportHistory. */
	void LoadFromFile();
	
public:

	UPROPERTY(config, VisibleDefaultsOnly, Category=Import)
	TMap<FGuid,FDialogueImportSourceData> ImportHistory;
};
