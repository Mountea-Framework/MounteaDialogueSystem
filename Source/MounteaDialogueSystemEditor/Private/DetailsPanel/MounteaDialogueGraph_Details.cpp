// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraph_Details.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Widgets/Input/STextComboBox.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraph_Details"

FMounteaDialogueGraph_Details::FMounteaDialogueGraph_Details()
{
	BeginPIEHandle = FEditorDelegates::BeginPIE.AddLambda([this](bool bIsSimulating)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
		InitializePIEInstances();
	});
	
	EndPIEHandle = FEditorDelegates::EndPIE.AddLambda([this](bool bIsSimulating)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
		InitializePIEInstances();
	});
}

FMounteaDialogueGraph_Details::~FMounteaDialogueGraph_Details()
{
	if (BeginPIEHandle.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	}
	if (EndPIEHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);
	}
}

void FMounteaDialogueGraph_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	
	if (Objects.Num() > 0)
	{
		CustomizedObject = Objects[0];
	}
	
	UMounteaDialogueGraph* customizedGraph = Cast<UMounteaDialogueGraph>(Objects[0]);
	if (!IsValid(customizedGraph))
		return;
	
	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory("Mountea");
	IDetailGroup& PIEGroup = CategoryBuilder.AddGroup("PIE Instance", LOCTEXT("MounteaDialogueGraph_DetailsPIE", "PIE Instance Details"));
	
	PIEGroup.AddWidgetRow()
	.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([this]()
	{
		return HasActivePIE() ? EVisibility::Visible : EVisibility::Hidden;
	})))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("PIEInstanceLabel", "PIE Instance"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	[
		SNew(SBox)
		[
			SNew(STextComboBox)
			.OptionsSource(&CachedOptions)
			.InitiallySelectedItem(GetCurrentPIEInstance())
			.OnSelectionChanged_Raw(this, &FMounteaDialogueGraph_Details::OnPIEInstanceSelected)
			.OnComboBoxOpening_Raw(this, &FMounteaDialogueGraph_Details::InitializePIEInstances)
		]
	];

	PIEGroup.AddWidgetRow()
	.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([this]()
	{
		return !HasActivePIE() ? EVisibility::Visible : EVisibility::Collapsed;
	})))
	.WholeRowContent()
	[
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PIEInactiveMessage", "PIE must be active to select an instance."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.5f, 0.2f)))
			.Justification(ETextJustify::Center)
			.ToolTipText(TAttribute<FText>(LOCTEXT("MounteaDialogueGraph_DetailsPIE_Invalid", "Start Game in editor create PIE Instance.\nOnce PIE exists you can select the instance and see what Node is currently Active (if Graph matches).")))
		]
		
	];
}

bool FMounteaDialogueGraph_Details::HasActivePIE()
{
	if (!GEditor) return false;
	
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			return true;
		}
	}
	return false;
}

void FMounteaDialogueGraph_Details::ResetPIEInstances()
{
	CachedOptions = TArray<TSharedPtr<FString>>();
	CachedOptions.Add(MakeShared<FString>(TEXT("None")));
}

void FMounteaDialogueGraph_Details::InitializePIEInstances()
{
	CachedOptions = GetPIEInstances();
}

TArray<TSharedPtr<FString>> FMounteaDialogueGraph_Details::GetPIEInstances()
{
	TArray<TSharedPtr<FString>> Options;
	
	Options.Add(MakeShared<FString>(TEXT("None")));
	
	if (!GEditor) return Options;
	
	// Get all PIE worlds
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			FString InstanceType;
			if (Context.RunAsDedicated)
			{
				InstanceType = TEXT("Server");
			}
			else
			{
				if (Context.PIEInstance == 2)
				{
					InstanceType = TEXT("Client 1");
				}
				else if (Context.PIEInstance > 2)
				{
					InstanceType = FString::Printf(TEXT("Client %d"), Context.PIEInstance - 2);
				}
				else
				{
					InstanceType = TEXT("Local");
				}
			}

			Options.Add(MakeShared<FString>(
				FString::Printf(TEXT("PIE Instance %d (%s)"), 
				Context.PIEInstance,
				*InstanceType)));
		}
	}
	
	return Options;
}

TSharedPtr<FString> FMounteaDialogueGraph_Details::GetCurrentPIEInstance() const
{
	// TODO: Implement logic here to get the currently selected instance
	// For now, default to "None"
	return MakeShared<FString>(TEXT("None"));
}

void FMounteaDialogueGraph_Details::OnPIEInstanceSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (!NewValue.IsValid() || !CustomizedObject.IsValid())
		return;

	FString ValueStr = *NewValue.Get();
	
	if (ValueStr == TEXT("None"))
	{
		// TODO: Reset graph visuals
		return;
	}
	
	FString InstanceStr;
	if (ValueStr.Split(TEXT("PIE Instance "), nullptr, &InstanceStr))
	{
		FString NumberStr;
		if (InstanceStr.Split(TEXT(" ("), &NumberStr, nullptr))
		{
			const int32 NewInstance = FCString::Atoi(*NumberStr);
			// TODO: update graph visual
		}
	}
}

#undef LOCTEXT_NAMESPACE
