// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Engine/DataTable.h"
#include "MounteaDialogueDecorator_OverrideDialogue.generated.h"

/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support for selecting Dialogue Row data.
 */
UCLASS( Blueprintable, BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea, Dialogue"), DisplayName="Override Dialogue Row Data")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecorator_OverrideDialogue : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:

	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;

protected:

	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(RequiredAssetDataTags="RowStructure=DialogueRow", DisplayThumbnail=false, NoResetToDefault))
	UDataTable*	DataTable;

	/** Name of row in the table that we want */
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames", NoResetToDefault, EditCondition="DataTable!=nullptr"))
	FName RowName;

	UPROPERTY(Category="Override", EditAnywhere, BlueprintReadOnly, meta=(UIMin=0, ClampMin=0, NoResetToDefault, EditCondition="DataTableHandle.DataTable!=nullptr"))
	int32 RowIndex;

private:

	UFUNCTION()
	TArray<FName> GetRowNames() const
	{
		if (DataTable)
		{
			return DataTable->GetRowNames();
		}

		return TArray<FName>();
	}
};
