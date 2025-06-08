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
UCLASS(NotBlueprintable, NotBlueprintType, config=MounteaDialogueImportConfig, ProjectUserConfig, DefaultConfig, meta=(DisplayName = "Mountea Dialogue System (Import)"))
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueImportConfig final : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UMounteaDialogueImportConfig(const FObjectInitializer& ObjectInitializer);

	bool IsReimport(const FGuid& GraphGuid) const;
	FString WriteToConfig(const FGuid& DialogueGuid, const FDialogueImportSourceData& NewSourceData);
	
public:

	UPROPERTY(config, VisibleDefaultsOnly, Category=Import)
	TMap<FGuid,FDialogueImportSourceData> ImportHistory;
};
