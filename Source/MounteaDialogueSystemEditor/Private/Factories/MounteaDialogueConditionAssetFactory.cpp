
// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools


#include "MounteaDialogueConditionAssetFactory.h"

#include "Conditions/MounteaDialogueConditionBase.h"
#include "Helpers/MounteaDialogueGraphEditorUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"

UMounteaDialogueConditionAssetFactory::UMounteaDialogueConditionAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UMounteaDialogueConditionBase::StaticClass();
}

UObject* UMounteaDialogueConditionAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Something is not right!
	if (ParentClass == nullptr || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "CannotCreateBlueprintFromClass", "Cannot create a blueprint based on the class '{0}'."), FText::FromString(ParentClass->GetName())));
		return nullptr;
	}

	// Create new Blueprint
	auto createdBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		InParent,
		Name,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	createdBlueprint->HideCategories.Add("Hidden");
	createdBlueprint->HideCategories.Add("Private");
	createdBlueprint->HideCategories.Add("Base");
	createdBlueprint->HideCategories.Add("Hide");
	createdBlueprint->HideCategories.Add("Editor");

	createdBlueprint->BlueprintCategory = FString(TEXT("Mountea Dialogue"));

	return createdBlueprint;
}

bool UMounteaDialogueConditionAssetFactory::ConfigureProperties()
{
	static const FText titleText = FText::FromString(TEXT("Pick Parent Class for new Mountea Dialogue Condition Object"));
	ParentClass = nullptr;

	UClass* chosenClass = nullptr;
	const bool bPressedOk = FMounteaDialogueGraphEditorUtilities::PickChildrenOfClass(titleText, chosenClass, SupportedClass);
	if (bPressedOk)
		ParentClass = chosenClass;

	return bPressedOk;
}
