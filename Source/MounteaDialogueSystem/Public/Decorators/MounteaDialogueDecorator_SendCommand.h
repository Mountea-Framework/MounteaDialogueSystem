// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueDecoratorBase.h"
#include "MounteaDialogueDecorator_SendCommand.generated.h"

/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support send Command to Dialogue Participant.
 */
UCLASS( BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Send Command")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecorator_SendCommand : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:
	
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Send-Command"); }

protected:

	UPROPERTY(SaveGame, Category="Command", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault))
	FString Command;

	UPROPERTY(SaveGame, Category="Payload", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault))
	TObjectPtr<UObject> OptionalPayload;
};
