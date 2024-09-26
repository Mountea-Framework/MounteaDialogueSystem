// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Nodes/K2Node_MounteaDialogueCallFunction.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "MounteaDialogueSystemEditor/Private/EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "BlueprintNodeSpawner.h"
#include "Interfaces/IPluginManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogUK2Node_MounteaDialogueCallFunction, Log, All);

namespace
{
	TSharedPtr<IPlugin> GetThisPlugin()
	{
		// NOTE: I didn't find a way to get the plugin without a hardcoded name...
		return IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	}

	// Retrieves the list of function libraries that belong to the current plugin
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
			UE_LOG(LogUK2Node_MounteaDialogueCallFunction, Error, TEXT("Unable to find plugin. Validate plugin `MounteaDialogueSystem` and its name."));
			return functionLibrariesWhitelist;
		}

		TSet<FString> thisPluginModulesNames;
		for (const FModuleDescriptor& module : thisPlugin->GetDescriptor().Modules)
		{
			thisPluginModulesNames.Add(module.Name.ToString());
		}
		
		// Iterate over all loaded classes
		for (TObjectIterator<UClass> it; it; ++it)
		{
			UClass* classType = *it;

			// Add the classes that are BP function libraries and that belong to a package of the current plugin
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
		const TSet<UClass*>& functionLibraries = GetFunctionLibraryClasses();

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
			if (customTagValue == TEXT("MounteaK2Function"))
				return EFunctionCallType::Function;
			if (customTagValue == TEXT("MounteaK2Message"))
				return EFunctionCallType::Message;
			if (customTagValue == TEXT("MounteaK2Delegate"))
				return EFunctionCallType::Delegate;
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
			if (customTagValue == TEXT("MounteaK2Getter"))
				return EFunctionRole::Get;
			if (customTagValue == TEXT("MounteaK2Getter"))
				return EFunctionRole::Set;
			if (customTagValue == TEXT("MounteaK2Validate"))
				return EFunctionRole::Validate;
		}
	}	

	return EFunctionRole::Unknown;
}

FText UK2Node_MounteaDialogueCallFunction::GetTooltipText() const
{
	FText defaultText =  Super::GetTooltipText();
	
	/* TODO: Add some nice description that will explain that some function are callbacks, other delegates etc.
	switch (GetFunctionType())
	{
	}
	*/
	
	switch (GetFunctionRole())
	{
		case EFunctionRole::Set:
			defaultText = FText::Format(INVTEXT("{0}\n\n{1}"), defaultText, INVTEXT("📥Set: Set functions (try to) update data in target."));
			break;
		case EFunctionRole::Validate:
			defaultText = FText::Format(INVTEXT("{0}\n\n{1}"), defaultText, INVTEXT("❔Validate: Validate functions perform validation checks."));
			break;
		case EFunctionRole::Get:
			defaultText = FText::Format(INVTEXT("{0}\n\n{1}"), defaultText, INVTEXT("📤Get: Get functions will instantaniously return data."));
			break;
		default:
			break;
	}
	
	return defaultText;
}

FLinearColor UK2Node_MounteaDialogueCallFunction::GetNodeTitleColor() const
{
	switch (GetFunctionRole())
	{
		case EFunctionRole::Validate:
		{
			return FLinearColor(.0, 0.75, .925);
		}
		case EFunctionRole::Set:
		{
			return FLinearColor(1, .7, .1);
		}
		case EFunctionRole::Get:
		{
			return FLinearColor(.27, 0.0, .4);
		}
		case EFunctionRole::Unknown:
			break;
	}
	return Super::GetNodeTitleColor();
}

FName UK2Node_MounteaDialogueCallFunction::GetCornerIcon() const
{
	return Super::GetCornerIcon();

	/* TODO: Add some nice icons to define function type
	switch (GetFunctionType())
	{
		case EFunctionCallType::Function:
			return TEXT("Graph.Latent.LatentIcon");
		case EFunctionCallType::Message:
			return TEXT("Graph.Message.MessageIcon");
		case EFunctionCallType::Delegate:
			return FName();
	}

	return Super::GetCornerIcon();
	*/
}
FSlateIcon UK2Node_MounteaDialogueCallFunction::GetIconAndTint(FLinearColor& outColor) const
{
	outColor = FLinearColor(.823, .823, .823);
	
	switch (GetFunctionRole())
	{
		case EFunctionRole::Validate:
		{
			static FSlateIcon icon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.K2Node_ValidateIcon.small");
			return icon;
		}
		case EFunctionRole::Set:
		{
			static FSlateIcon icon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.K2Node_SetterIcon.small");
			return icon;
		}
		case EFunctionRole::Get:
		{
			static FSlateIcon icon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.K2Node_GetterIcon.small");
			return icon;
		}
		case EFunctionRole::Unknown:
			break;
	}
	
	return Super::GetIconAndTint(outColor);
}

void UK2Node_MounteaDialogueCallFunction::Initialize(const UFunction* func, UClass* cls)
{
	FunctionReference.SetExternalMember(func->GetFName(), cls);
}