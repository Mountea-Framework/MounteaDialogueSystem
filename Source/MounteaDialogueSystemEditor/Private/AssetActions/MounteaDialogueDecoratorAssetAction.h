// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"

class FMounteaDialogueDecoratorAssetAction : public FAssetTypeActions_Base
{
	
public:

	FMounteaDialogueDecoratorAssetAction();

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;

	virtual bool CanFilter() override
	{ return false; };
	
	virtual void BuildBackendFilter(FARFilter& InFilter) override
	{
		FilterAddNativeParentClassPath(InFilter, GetSupportedClass());

		// Add to filter all native children classes of our supported class
		TArray<UClass*> NativeChildClasses;
		TArray<UClass*> BlueprintChildClasses;
		FMounteaDialogueGraphEditorUtilities::GetAllChildClassesOf(GetSupportedClass(), NativeChildClasses, BlueprintChildClasses);
		for (const UClass* ChildNativeClass : NativeChildClasses)
		{
			FilterAddNativeParentClassPath(InFilter, ChildNativeClass);
		}

		InFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName()); 
		//InFilter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
		InFilter.bRecursiveClasses = true;
	};
	static void FilterAddNativeParentClassPath(FARFilter& InFilter, const UClass* Class)
	{
		if (Class == nullptr)
		{
			return;
		}

		const FString Value = FString::Printf(
			TEXT("%s'%s'"),
			*UClass::StaticClass()->GetName(),
			*Class->GetPathName()
		);
		InFilter.TagsAndValues.Add(FBlueprintTags::NativeParentClassPath, Value);
	};
};
