// All rights reserved Dominik Morse (Pavlicek) 2024

#include "K2Nodes/K2Node_MounteaDialogueCallFunction.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "MounteaDialogueSystemEditor/Private/EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "BlueprintNodeSpawner.h"
#include "Interfaces/IPluginManager.h"

namespace MounteaDialogueHelpers
{
	TSharedPtr<IPlugin> GetThisPlugin();
	const TSet<UClass*> GetFunctionLibraryClasses();
}

void UK2Node_MounteaDialogueCallFunction::GetMenuActions(FBlueprintActionDatabaseRegistrar& registrar) const
{
	Super::GetMenuActions(registrar);

	UClass* nodeClass = GetClass();

	auto customizeNodeLambda = [](UEdGraphNode* newNode, bool bIsTemplateNode, UFunction* func, UClass* cls)
	{
		UK2Node_MounteaDialogueCallFunction* inputNode = CastChecked<UK2Node_MounteaDialogueCallFunction>(newNode);
		inputNode->Initialize(func, cls);
	};

	if (registrar.IsOpenForRegistration(nodeClass))
	{
		const TSet<UClass*>& functionLibraries = MounteaDialogueHelpers::GetFunctionLibraryClasses();

		for (UClass* functionClass : functionLibraries)
		{
			TArray<FName> functionNames;
			functionClass->GenerateFunctionList(functionNames);

			for (const FName& functionName : functionNames)
			{
				if (UFunction* localFunction = functionClass->FindFunctionByName(functionName, EIncludeSuperFlag::ExcludeSuper))
				{
					localFunction->SetMetaData(TEXT("BlueprintInternalUseOnly"), TEXT("true"));
					localFunction->RemoveMetaData(TEXT("BlueprintCallable"));

					UBlueprintNodeSpawner* nodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
					nodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(customizeNodeLambda, localFunction, functionClass);

					registrar.AddBlueprintAction(nodeClass, nodeSpawner);
				}
			}
		}
	}
}

EFunctionCallType UK2Node_MounteaDialogueCallFunction::GetFunctionType() const
{
	if (const UFunction* localFunction = GetTargetFunction())
	{
		if (localFunction->HasMetaData(TEXT("CustomTag")))
		{
			const FString customTagValue = localFunction->GetMetaData(TEXT("CustomTag"));
			if (customTagValue == TEXT("MounteaK2Function")) return EFunctionCallType::Function;
			if (customTagValue == TEXT("MounteaK2Message")) return EFunctionCallType::Message;
			if (customTagValue == TEXT("MounteaK2Delegate")) return EFunctionCallType::Delegate;
		}
	}
	return EFunctionCallType::Unknown;
}

EFunctionRole UK2Node_MounteaDialogueCallFunction::GetFunctionRole() const
{
	if (const UFunction* localFunction = GetTargetFunction())
	{
		if (localFunction->HasMetaData(TEXT("CustomTag")))
		{
			const FString customTagValue = localFunction->GetMetaData(TEXT("CustomTag"));
			if (customTagValue == TEXT("MounteaK2Getter")) return EFunctionRole::Get;
			if (customTagValue == TEXT("MounteaK2Setter")) return EFunctionRole::Set;
			if (customTagValue == TEXT("MounteaK2Validate")) return EFunctionRole::Validate;
		}
	}
	return EFunctionRole::Unknown;
}

FText UK2Node_MounteaDialogueCallFunction::GetTooltipText() const
{
	FText defaultText = Super::GetTooltipText();
	
	switch (GetFunctionRole())
	{
		case EFunctionRole::Set:
			return FText::Format(INVTEXT("{0}\n\n📥 Setter: These functions attempt to modify the data in the target."), defaultText);
		case EFunctionRole::Validate:
			return FText::Format(INVTEXT("{0}\n\n❔ Validator: These functions perform necessary checks to ensure data integrity."), defaultText);
		case EFunctionRole::Get:
			return FText::Format(INVTEXT("{0}\n\n📤 Getter: These functions retrieve and return data."), defaultText);
		default:
			return defaultText;
	}
}

FLinearColor UK2Node_MounteaDialogueCallFunction::GetNodeTitleColor() const
{
	
	switch (GetFunctionRole())
	{
		case EFunctionRole::Validate: return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("99621e")));
		case EFunctionRole::Set: return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("420039")));
		case EFunctionRole::Get: return FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("05204a")));
		default: return Super::GetNodeTitleColor();
	}
}

FName UK2Node_MounteaDialogueCallFunction::GetCornerIcon() const
{
	return TEXT("MDSStyleSet.MounteaLogo");
	// TODO: Add custom icons for different function types
}

FSlateIcon UK2Node_MounteaDialogueCallFunction::GetIconAndTint(FLinearColor& outColor) const
{
	outColor = FLinearColor(.823f, .823f, .823f);
	
	switch (GetFunctionRole())
	{
		case EFunctionRole::Validate:
			return FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.K2Node_ValidateIcon.small");
		case EFunctionRole::Set:
			return FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.K2Node_SetterIcon.small");
		case EFunctionRole::Get:
			return FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.K2Node_GetterIcon.small");
		default:
			return Super::GetIconAndTint(outColor);
	}
}

void UK2Node_MounteaDialogueCallFunction::Initialize(const UFunction* func, UClass* cls)
{
	FunctionReference.SetExternalMember(func->GetFName(), cls);
}

namespace MounteaDialogueHelpers
{
	TSharedPtr<IPlugin> GetThisPlugin()
	{
		return IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	}

	const TSet<UClass*> GetFunctionLibraryClasses()
	{
		static TSet<UClass*> functionLibrariesWhitelist;
		
		if (!functionLibrariesWhitelist.IsEmpty())
		{
			return functionLibrariesWhitelist;
		}

		const TSharedPtr<IPlugin> thisPlugin = GetThisPlugin();
		if (!thisPlugin.IsValid())
		{
			return functionLibrariesWhitelist;
		}

		TSet<FString> thisPluginModulesNames;
		for (const FModuleDescriptor& module : thisPlugin->GetDescriptor().Modules)
		{
			thisPluginModulesNames.Add(module.Name.ToString());
		}
		
		for (TObjectIterator<UClass> it; it; ++it)
		{
			UClass* classType = *it;

			if (classType->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
			{
				const FString packageName = FPackageName::GetLongPackageAssetName(classType->GetPackage()->GetName());
				if (thisPluginModulesNames.Contains(packageName))
				{
					functionLibrariesWhitelist.Add(classType);
				}
			}
		}

		return functionLibrariesWhitelist;
	}
}