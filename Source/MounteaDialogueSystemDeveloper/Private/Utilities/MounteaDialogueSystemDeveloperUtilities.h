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

	UFUNCTION(BlueprintCallable, Category="Mountea|Developer", meta=(CustomTag="MounteaK2Getter"))
	static void CopyToClipboard(const FString& Input);
};
