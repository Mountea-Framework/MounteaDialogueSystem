// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MounteaDialogueProjectFactory.generated.h"

/**
 * Factory that imports a .mnteadlgproj file.
 *
 * A project archive is a ZIP that contains:
 *   projectData.json        — project-level metadata
 *   categories.json         — shared category definitions
 *   participants.json        — shared participant definitions
 *   decorators.json          — shared decorator definitions
 *   conditions.json          — shared condition definitions
 *   Thumbnails/{id}.png      — participant thumbnails
 *   dialogues/*.mnteadlg     — individual dialogue archives
 *
 * Each nested .mnteadlg is extracted to a temp location and imported through
 * the standard ImportDialogueGraph() path. The import-config entry for every
 * resulting UMounteaDialogueGraph is then updated so that its
 * DialogueSourcePath points to the original .mnteadlgproj file rather than
 * the ephemeral temp file.
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueProjectFactory : public UFactory
{
	GENERATED_BODY()

public:

	UMounteaDialogueProjectFactory();

	//~ UFactory interface
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName,
		EObjectFlags Flags, const FString& Filename,
		const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

	virtual bool FactoryCanImport(const FString& Filename) override;
	//~ End UFactory interface
};
