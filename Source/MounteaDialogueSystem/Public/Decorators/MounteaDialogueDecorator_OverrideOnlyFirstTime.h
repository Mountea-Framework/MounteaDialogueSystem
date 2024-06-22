// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecorator_OnlyFirstTime.h"
#include "MounteaDialogueDecorator_OverrideOnlyFirstTime.generated.h"

/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support for triggering logic if Owning node has been entered for the first time.
 * Overrides Dialogue Data only if Owning Node has been traversed for the first Time.
 * Does not block Owning node from Evaluation, therefore Owning node is available to selection.
 *
 * For proper usage, save Dialogue Participants when saving game!
 */
UCLASS(  BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Override Only First Time")
class MOUNTEADIALOGUESYSTEM_API  UMounteaDialogueDecorator_OverrideOnlyFirstTime : public UMounteaDialogueDecorator_OnlyFirstTime
{
	GENERATED_BODY()

public:
	
	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Override-Only-Frist-Time"); }


protected:

	UPROPERTY(Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, RequiredAssetDataTags = "RowStructure=/Script/MounteaDialogueSystem.DialogueRow"))
	UDataTable* DataTable;

	/** Name of row in the table that we want */
	UPROPERTY(Category="Override", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames", NoResetToDefault, EditCondition="DataTable!=nullptr"))
	FName RowName;
	
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
