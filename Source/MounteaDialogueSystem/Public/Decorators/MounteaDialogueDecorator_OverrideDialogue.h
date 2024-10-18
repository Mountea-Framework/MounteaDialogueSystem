// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Engine/DataTable.h"
#include "MounteaDialogueDecorator_OverrideDialogue.generated.h"

class UMounteaDialogueContext;
class IMounteaDialogueManagerInterface;
/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support for selecting Dialogue Row data.
 */
UCLASS( BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Override Dialogue Row Data")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecorator_OverrideDialogue : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:
	
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(UPARAM(ref) TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;
	virtual bool IsDecoratorAllowedForGraph_Implementation() const override {  return false;  };

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Override-Dialogue-Row-Data"); }

protected:

	UPROPERTY(Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, RequiredAssetDataTags = "RowStructure=/Script/MounteaDialogueSystem.DialogueRow"))
	TObjectPtr<UDataTable>		DataTable;

	/** Name of row in the table that we want */
	UPROPERTY(Category="Override", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames", NoResetToDefault, EditCondition="DataTable!=nullptr"))
	FName						RowName;

	UPROPERTY(Category="Override", EditAnywhere, BlueprintReadOnly, meta=(UIMin=0, ClampMin=0, NoResetToDefault, EditCondition="DataTable!=nullptr"))
	int32						RowIndex;

private:
	
	UPROPERTY()
	TObjectPtr<UMounteaDialogueContext> Context = nullptr;
	
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
