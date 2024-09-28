// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class  FMounteaDialogueGraphAssetAction : public FAssetTypeActions_Base
{
public:
	FMounteaDialogueGraphAssetAction();

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	virtual void GetActions(const TArray<UObject*>& InObjects, struct FToolMenuSection& Section) override;

protected:

	void ExecuteExportDialogue(TArray< TWeakObjectPtr<UObject> > Objects);
	void ExecuteReimportDialogue(TArray< TWeakObjectPtr<UObject> > Objects);
	void ExecuteOpenDialogueSource(TArray< TWeakObjectPtr<UObject> > Objects);

	bool CanExecuteReimportDialogue(TArray< TWeakObjectPtr<UObject> > Objects);
	bool CanExecuteOpenDialogueSource(TArray< TWeakObjectPtr<UObject> > Objects);
	/*
	virtual FText GetAssetDescription(const FAssetData& AssetData) const override;
	*/
};
