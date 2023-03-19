// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Engine/DataTable.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNode_DialogueNodeBase.generated.h"


/**
 * Mountea Dialogue Graph Node abstract Base class.
 * 
 * Enhances 'MounteaDialogueGraphNode' Base class with Dialogue data.
 * Provides DataTable and Row options that define the Dialogue data which will be displayed in UI.
 */
UCLASS(Abstract, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea", "Dialogue", "Mountea|Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_DialogueNodeBase : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_DialogueNodeBase();

public:
	
	virtual void ProcessNode(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual UDataTable* GetDataTable() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual FName GetRowName() const
	{ return RowName; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
	virtual bool DoesAutoStart() const
	{ return bAutoStarts; };

public:

#if WITH_EDITORONLY_DATA

	/**
	 * ❗Experimental Feature ❗
	 * Shows read-only Texts with localization of selected Dialogue Row.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Base", meta=(MultiLine=true, ShowOnlyInnerProperties))
	TArray<FText> Preview;

	FSimpleDelegate PreviewsUpdated;

#endif

protected:
	
	UPROPERTY(SaveGame, Category="Mountea|Dialogue", EditAnywhere, BlueprintReadOnly, meta=(RequiredAssetDataTags="RowStructure=DialogueRow", DisplayThumbnail=false, NoResetToDefault))
	UDataTable*	DataTable;

	/** Name of row in the table that we want */
	UPROPERTY(SaveGame, Category="Mountea|Dialogue", EditAnywhere, BlueprintReadOnly, meta=(GetOptions ="GetRowNames", NoResetToDefault, EditCondition="DataTable!=nullptr"))
	FName RowName;

#if WITH_EDITOR
	
	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual FText GetDescription_Implementation() const override;
	
public:
	TArray<FText> GetPreviews() const;
#endif

#if WITH_EDITORONLY_DATA
public:
	virtual void UpdatePreviews();
#endif

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
