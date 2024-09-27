// All rights reserved Dominik Morse (Pavlicek) 2024

#include "K2Nodes/K2Node_MounteaDialogueCallFunction.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "MounteaDialogueSystemEditor/Private/EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "BlueprintNodeSpawner.h"

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
		const TSet<UClass*>& relevantClasses = MounteaDialogueHelpers::GetRelevantClasses();

		for (UClass* relevantClass : relevantClasses)
		{
			TArray<UFunction*> classFunctions;
			for (TFieldIterator<UFunction> FuncIt(relevantClass, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
			{
				UFunction* Function = *FuncIt;
				if (Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) && !Function->HasAnyFunctionFlags(FUNC_Private))
				{
					classFunctions.Add(Function);
				}
			}

			for (UFunction* Function : classFunctions)
			{
				Function->SetMetaData(TEXT("BlueprintInternalUseOnly"), TEXT("true"));
				Function->RemoveMetaData(TEXT("BlueprintCallable"));

				UBlueprintNodeSpawner* nodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
				nodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(customizeNodeLambda, Function, relevantClass);

				registrar.AddBlueprintAction(nodeClass, nodeSpawner);
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
	auto SuperName = Super::GetCornerIcon();

	if (SuperName == NAME_None)
		return TEXT("MDSStyleSet.MounteaLogo");

	return SuperName;
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
