// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueDecorator_CustomDetailsHelper.h"

#include "DetailWidgetRow.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "SourceCodeNavigation.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
//#include "Decorators/MounteaDialogueGraphNodeDecoratorBase.h"
//#include "Engine/Selection.h"
//#include "Helpers/MounteaDialogueGraphEditorHelpers.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_CustomDetailsHelper"
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

FMounteaDialogueDecorator_CustomDetailsHelper::FMounteaDialogueDecorator_CustomDetailsHelper(IDetailPropertyRow* InPropertyRow)
: PropertyRow(InPropertyRow)
{}

void FMounteaDialogueDecorator_CustomDetailsHelper::Update()
{
	TSharedPtr<SWidget> DefaultNameWidget;
	TSharedPtr<SWidget> DefaultValueWidget;
	PropertyRow->GetDefaultWidgets
	(
		DefaultNameWidget,
		DefaultValueWidget,
		false
	);
	
	PropertyRow->ShowPropertyButtons(true);

	FExecuteAction OnInsertClicked; // Insert is not allowed
	FExecuteAction OnDeleteClicked = FExecuteAction::CreateSP( this, &FMounteaDialogueDecorator_CustomDetailsHelper::RequestDeleteItem );
	FExecuteAction OnDuplicateClicked; // Duplicates are not allowed
	
	FDetailWidgetRow& DetailWidgetRow = PropertyRow->CustomWidget(true);
	DetailWidgetRow.NameContent()
	[
		DefaultNameWidget.ToSharedRef()
	];

	TSharedPtr<SHorizontalBox> HorizontalBox;
	DetailWidgetRow.ValueContent()
	.MinDesiredWidth(GetRowMinimumDesiredWidth())
	[
		SAssignNew(HorizontalBox, SHorizontalBox)
	];

	// Default previous widget
	HorizontalBox->AddSlot()
   .Padding(0.f, 0.f, 2.f, 0.f)
   .FillWidth(1.f)
   [
	   DefaultValueWidget.ToSharedRef()
   ];
	
	HorizontalBox->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(4.f)
	[
		PropertyCustomizationHelpers::MakeInsertDeleteDuplicateButton( OnInsertClicked, OnDeleteClicked, OnDuplicateClicked)
	];

	// GEditor doesn't support inserting Handle values for EditInline
	/*
	HorizontalBox->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(4.f)
	[
		PropertyCustomizationHelpers::MakeUseSelectedButton( OnUseSelectedClicked)
	];
	*/
	
	// Browse Asset
	HorizontalBox->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(4.f)
	[
		SNew(SButton)
		.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
		.ToolTipText(this, &Self::GetBrowseObjectText)
		.ContentPadding(4.f)
		.ForegroundColor(FSlateColor::UseForeground())
		.Visibility(this, &Self::GetBrowseButtonVisibility)
		.OnClicked(this, &Self::OnBrowseClicked)
		[
			SNew(SImage)
			.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Icon.Browse"))
			.ColorAndOpacity(FSlateColor::UseForeground())
		]
	];

	// Jump to Object
	HorizontalBox->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.Padding(4.f, 2.f)
	[
		SNew(SButton)
		.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
		.ToolTipText(this, &Self::GetJumpToObjectText)
		.ContentPadding(4.f)
		.ForegroundColor(FSlateColor::UseForeground())
		.Visibility(this, &Self::GetOpenButtonVisibility)
		.OnClicked(this, &Self::OnOpenClicked)
		[
			SNew(SImage)
			.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Icon.Edit"))
			 .ColorAndOpacity( FSlateColor::UseForeground() )
		]
	];
}

FReply FMounteaDialogueDecorator_CustomDetailsHelper::OnBrowseClicked()
{
	if (!GEditor)
	{
		return FReply::Handled();
	}

	static constexpr bool bFocusContentBrowser = true;
	TArray<UObject*> ObjectsToSyncTo;
	if (UBlueprint* Blueprint = GetBlueprint())
	{
		ObjectsToSyncTo.Add(Blueprint);
	}
	GEditor->SyncBrowserToObjects(ObjectsToSyncTo, bFocusContentBrowser);

	return FReply::Handled();
}

FReply FMounteaDialogueDecorator_CustomDetailsHelper::OnOpenClicked()
{
	if (UBlueprint* Blueprint = GetBlueprint())
	{
		FMounteaDialogueGraphEditorUtilities::OpenBlueprintEditor(
			Blueprint,
			OpenType,
			FunctionNameToOpen,
			bForceFullEditor,
			bAddBlueprintFunctionIfItDoesNotExist
		);
	}
	else if (UObject* Object = GetObject())
	{
		// Native
		FSourceCodeNavigation::NavigateToClass(Object->GetClass());
	}
	return FReply::Handled();
}

UObject* FMounteaDialogueDecorator_CustomDetailsHelper::GetObject() const
{
	if (!PropertyRow)
	{
		return nullptr;
	}

	TSharedPtr<IPropertyHandle> PropertyHandle = PropertyRow->GetPropertyHandle();
	if (!PropertyHandle.IsValid())
	{
		return nullptr;
	}
	UObject* Object = nullptr;
	PropertyHandle->GetValue(Object);
	return Object;
}

UBlueprint* FMounteaDialogueDecorator_CustomDetailsHelper::GetBlueprint() const
{
	UObject* Object = GetObject();
	if (!Object) return nullptr;

	// Class
	UClass* Class = Object->GetClass();
	if (const UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class))
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy))
		{
			return Blueprint;
		}
	}

	return Cast<UBlueprint>(Object);
}

bool FMounteaDialogueDecorator_CustomDetailsHelper::IsObjectABlueprint() const
{
	return GetBlueprint() != nullptr;
}

FText FMounteaDialogueDecorator_CustomDetailsHelper::GetBrowseObjectText() const
{
	return LOCTEXT("MounteaDialogueDecorator_CustomDetailsHelper_BrowseButtonToolTipText", "Browse to Asset in Content Browser");
}

FText FMounteaDialogueDecorator_CustomDetailsHelper::GetJumpToObjectText() const
{
	if (IsObjectABlueprint())
	{
		return LOCTEXT("MounteaDialogueDecorator_CustomDetailsHelper_OpenObjectBlueprintTooltipKey", "Open Blueprint Editor");
	}

	// Native Class
	return FText::Format(
		LOCTEXT("MounteaDialogueDecorator_CustomDetailsHelper_OpenObjectBlueprintTooltipKey", "Open Source File in {0}"),
		FSourceCodeNavigation::GetSelectedSourceCodeIDE()
	);
}

EVisibility FMounteaDialogueDecorator_CustomDetailsHelper::GetOpenButtonVisibility() const
{
	if (!CanBeVisible())
	{
		return EVisibility::Collapsed;
	}

	if (UObject* Object = GetObject())
	{
		// Blueprint
		if (IsObjectABlueprint())
		{
			return EVisibility::Visible;
		}

		// Native
		return FSourceCodeNavigation::CanNavigateToClass(Object->GetClass()) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

EVisibility FMounteaDialogueDecorator_CustomDetailsHelper::GetBrowseButtonVisibility() const
{
	if (!CanBeVisible())
	{
		return EVisibility::Collapsed;
	}

	return IsObjectABlueprint() ? EVisibility::Visible : EVisibility::Collapsed;
}

void FMounteaDialogueDecorator_CustomDetailsHelper::RequestDeleteItem()
{
	PropertyUtils->EnqueueDeferredAction( FSimpleDelegate::CreateSP( this, &FMounteaDialogueDecorator_CustomDetailsHelper::OnDeleteItem ) );
}

// TODO: Make this just better
void FMounteaDialogueDecorator_CustomDetailsHelper::OnDeleteItem()
{
	auto ArrayHandle = PropertyRow->GetPropertyHandle()->GetParentHandle()->GetParentHandle()->AsArray(); // NodeDecorators

	// PLEASE TELL ME THERE IS A BETTER WAY
	// I SUFFER IMAGINING THIS WILL GO LIVE :(
	FString IndexString =  PropertyRow->GetPropertyHandle()->GetParentHandle()->GetPropertyDisplayName().ToString();
	int32 Index = FCString::Atoi((TEXT("%s"), *IndexString));

	check(ArrayHandle.IsValid());

	if (ArrayHandle.IsValid())
	{
		ArrayHandle->DeleteItem(Index);
	}

}

#undef LOCTEXT_NAMESPACE
#undef DEFAULT_FONT