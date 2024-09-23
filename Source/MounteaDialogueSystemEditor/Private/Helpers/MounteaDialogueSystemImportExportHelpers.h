// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueSystemImportExportHelpers.generated.h"

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueSystemImportExportHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	// TODO:
	// 1. Export logic
	// 1.1 Gather assets from Graph
	// 1.2 Translate to JSON files
	// 1.3 Export audio
	// 1.4 Pack to custom .mnteadlg file (zip)

	// 2. Import logic
	// Move all that is inside the Factory Class
};
