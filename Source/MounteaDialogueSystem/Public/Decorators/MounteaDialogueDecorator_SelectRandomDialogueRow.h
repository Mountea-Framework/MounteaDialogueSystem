// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "MounteaDialogueDecorator_SelectRandomDialogueRow.generated.h"

class UMounteaDialogueContext;
class IMounteaDialogueManagerInterface;
/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support to pick random Row from Dialogue Data.
 */
UCLASS( BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Use Random Dialogue Row Data")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecorator_SelectRandomDialogueRow : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:
	
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(UPARAM(ref) TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://mountea.tools/docs/DialogueSystem/DialogueDecorators/SelectRandomDialogueRow"); }

protected:

	UPROPERTY(SaveGame, Category="Random", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault, InlineEditConditionToggle))
	bool bUseRange;

	/**
	 * Allows select random number from given Range.
	 * If range exceeds or is invalid, first valid random index is used.
	 */
	UPROPERTY(SaveGame, Category="Random", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault, EditCondition="bUseRange"))
	FIntPoint RandomRange;
};
