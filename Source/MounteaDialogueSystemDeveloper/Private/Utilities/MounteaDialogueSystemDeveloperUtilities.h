// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueSystemDeveloperUtilities.generated.h"

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEMDEVELOPER_API UMounteaDialogueSystemDeveloperUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Mountea|Developer")
	static void CopyToClipboard(const FString& Input);
};
