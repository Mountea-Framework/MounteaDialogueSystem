// All rights reserved Dominik Morse 2024


#include "MounteaDialogueNodeFactory.h"

#include "Helpers/MounteaDialogueGraphEditorUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Nodes/MounteaDialogueGraphNode.h"

UMounteaDialogueNodeFactory::UMounteaDialogueNodeFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;

	SupportedClass = UMounteaDialogueGraphNode::StaticClass();
}

UObject* UMounteaDialogueNodeFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Something is not right!
	if (ParentClass == nullptr || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "CannotCreateBlueprintFromClass", "Cannot create a blueprint based on the class '{0}'."), FText::FromString(ParentClass->GetName())));
		return nullptr;
	}

	// Create new Blueprint
	auto CreatedBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		InParent,
		Name,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	CreatedBlueprint->HideCategories.Add("Hidden");
	CreatedBlueprint->HideCategories.Add("Private");
	CreatedBlueprint->HideCategories.Add("Base");
	CreatedBlueprint->HideCategories.Add("Hide");
	CreatedBlueprint->HideCategories.Add("Editor");

	CreatedBlueprint->BlueprintCategory = FString(TEXT("Mountea Dialogue"));

	return CreatedBlueprint;
}

bool UMounteaDialogueNodeFactory::ConfigureProperties()
{
	static const FText TitleText = FText::FromString(TEXT("Pick Parent Class for new Mountea Dialogue Graph Node"));
	ParentClass = nullptr;

	UClass* ChosenClass = nullptr;
	const bool bPressedOk = FMounteaDialogueGraphEditorUtilities::PickChildrenOfClass(TitleText, ChosenClass, SupportedClass);
	if (bPressedOk)
		ParentClass = ChosenClass;

	return bPressedOk;
}
