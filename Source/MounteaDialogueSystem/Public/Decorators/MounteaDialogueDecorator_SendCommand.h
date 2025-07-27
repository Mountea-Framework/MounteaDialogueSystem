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
UCLASS( BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue,Command"), DisplayName="Send Command")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecorator_SendCommand : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:
	
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(UPARAM(ref) TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;
	virtual bool IsDecoratorStackable_Implementation() const override {return true; };

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://mountea.tools/docs/DialogueSystem/DialogueDecorators/SendCommand"); }

protected:

	UPROPERTY(SaveGame, Category="Command", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault))
	FString Command;

	UPROPERTY(SaveGame, Category="Payload", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault))
	TObjectPtr<UObject> OptionalPayload;
};
