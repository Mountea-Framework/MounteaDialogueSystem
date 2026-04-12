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
#include "UObject/Interface.h"
#include "MounteaDialogueSpeechDataInterface.generated.h"

struct FDialogueRow;

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueSpeechDataInterface : public UInterface
{
	GENERATED_BODY()
};

class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueSpeechDataInterface
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Node")
	FDialogueRow GetSpeechData();
	virtual FDialogueRow GetSpeechData_Implementation() const = 0;
	
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Node")
	bool SetSpeechData(const FDialogueRow& NewSpeechData);
	virtual bool SetSpeechData_Implementation(const FDialogueRow& NewSpeechData) = 0;
};
