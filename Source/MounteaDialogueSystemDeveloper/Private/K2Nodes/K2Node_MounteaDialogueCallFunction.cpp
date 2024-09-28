// All rights reserved Dominik Morse (Pavlicek) 2024

#include "K2Nodes/K2Node_MounteaDialogueCallFunction.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "MounteaDialogueSystemEditor/Private/EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "MounteaDialogueSystemEditor/Private/Settings/MounteaDialogueGraphEditorSettings.h"
#include "BlueprintNodeSpawner.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueCallFunction"

void UK2Node_MounteaDialogueCallFunction::GetMenuActions(FBlueprintActionDatabaseRegistrar& actionRegistrar) const
{
	Super::GetMenuActions(actionRegistrar);

	UClass* nodeClass = GetClass();

	auto Settings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();

	// Lambda for node customization
	auto customizeNodeLambda = [](UEdGraphNode* newNode, bool bIsTemplateNode, UFunction* relevantFunction, UClass* relevantClass)
	{
		UK2Node_MounteaDialogueCallFunction* inputNode = CastChecked<UK2Node_MounteaDialogueCallFunction>(newNode);
		inputNode->Initialize(relevantFunction, relevantClass);
	};
	
	if (actionRegistrar.IsOpenForRegistration(nodeClass))
	{
		const TSet<UClass*>& relevantClasses = MounteaDialogueHelpers::GetRelevantClasses();

		// Create a set to track added functions
		TSet<UFunction*> registeredFunctions;

		for (UClass* relevantClass : relevantClasses)
		{
			TArray<UFunction*> classFunctions;
			for (TFieldIterator<UFunction> FuncIt(relevantClass, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
			{
				UFunction* function = *FuncIt;
				
				if (!function->HasMetaData(TEXT("CustomTag")))
					continue;
				
				if (!function->HasAnyFunctionFlags(FUNC_Private))
				{
					// Check if the function is already registered
					if (!registeredFunctions.Contains(function))
					{
						classFunctions.Add(function);
						registeredFunctions.Add(function);
					}
				}
			}

			for (UFunction* itrFunction : classFunctions)
			{
				if (Settings && !Settings->DisplayStandardNodes())
				{
					itrFunction->SetMetaData(TEXT("BlueprintInternalUseOnly"), TEXT("true"));
				}
				else
				{
					if (!itrFunction->HasAnyFunctionFlags(FUNC_BlueprintEvent))
					{
						itrFunction->SetMetaData(TEXT("BlueprintInternalUseOnly"), TEXT("true"));
					}
				}

				UBlueprintNodeSpawner* nodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
				nodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(customizeNodeLambda, itrFunction, relevantClass);

				actionRegistrar.AddBlueprintAction(nodeClass, nodeSpawner);
			}
		}
	}
}

FText UK2Node_MounteaDialogueCallFunction::GetToolTipHeading() const
{
	return LOCTEXT("MounteaDialogueCallFunctionFunctions", "Mountea Dialogue Function");
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
			return FText::Format(
				INVTEXT("{0}\n\n📥 Setter: A setter function is responsible for updating or modifying a particular value or data in an object.\nIt directly affects the state of the target by assigning a new value to one of its properties.\nSetters may include internal validation or checks to ensure that the value being assigned meets certain criteria before applying it."), 
				defaultText);

		case EFunctionRole::Validate:
			return FText::Format(
				INVTEXT("{0}\n\n❔ Validator: Validator functions are used to verify the correctness or validity of data.\nThese functions perform checks to ensure that the data adheres to certain rules or constraints, such as format, range, or consistency.\nA successful validation confirms that the data is reliable and suitable for use."), 
				defaultText);

		case EFunctionRole::Get:
			return FText::Format(
				INVTEXT("{0}\n\n📤 Getter: A getter function is designed to retrieve and return a specific value or property from an object without modifying it.\nGetters are essential for accessing data in a controlled manner, ensuring that external code can view data but not alter it directly."), 
				defaultText);

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
	auto superName = Super::GetCornerIcon();

	if (superName == NAME_None)
		return TEXT("MDSStyleSet.MounteaLogo");

	return superName;
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

void UK2Node_MounteaDialogueCallFunction::Initialize(const UFunction* relevantFunction, UClass* relevantClass)
{
	FunctionReference.SetExternalMember(relevantFunction->GetFName(), relevantClass);
}

#undef LOCTEXT_NAMESPACE