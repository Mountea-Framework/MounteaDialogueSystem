// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once
#include "Interfaces/IPluginManager.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"

enum class EFunctionCallType
{
	Function,
	Message,
	Delegate,
	Unknown
};

enum class EFunctionRole
{
	Set,
	Get,
	Validate,
	Unknown
};

namespace MounteaDialogueHelpers
{
	TSharedPtr<IPlugin> GetThisPlugin();
	const TSet<UClass*> GetRelevantClasses();

	inline TSharedPtr<IPlugin> GetThisPlugin()
	{
		return IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	}

	inline const TSet<UClass*> GetRelevantClasses()
	{
		static TSet<UClass*> relevantClasses;
		
		if (!relevantClasses.IsEmpty())
		{
			return relevantClasses;
		}

		const TSharedPtr<IPlugin> thisPlugin = GetThisPlugin();
		if (!thisPlugin.IsValid())
		{
			return relevantClasses;
		}

		TSet<FString> thisPluginModulesNames;
		for (const FModuleDescriptor& module : thisPlugin->GetDescriptor().Modules)
		{
			thisPluginModulesNames.Add(module.Name.ToString());
		}
		
		for (TObjectIterator<UClass> it; it; ++it)
		{
			UClass* classType = *it;

			const FString packageName = FPackageName::GetLongPackageAssetName(classType->GetPackage()->GetName());
			if (thisPluginModulesNames.Contains(packageName))
			{
				// Include function libraries
				if (classType->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
				{
					relevantClasses.Add(classType);
				}
				// Include interfaces
				else if (classType->IsChildOf(UInterface::StaticClass()))
				{
					relevantClasses.Add(classType);
				}
				// Include regular classes
				else if (!classType->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
				{
					relevantClasses.Add(classType);
				}
			}
		}

		return relevantClasses;
	}
}