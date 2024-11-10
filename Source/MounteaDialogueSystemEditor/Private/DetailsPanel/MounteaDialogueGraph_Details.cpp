// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraph_Details.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"
#include "Widgets/Input/STextComboBox.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraph_Details"

FString FPIEInstanceData::GetParticipantsDescription() const
{
	if (Participants.IsEmpty())
	{
		return TEXT("No Active Participants");
	}

	FString Result;
	for (int32 i = 0; i < Participants.Num(); ++i)
	{
		if (const auto Participant = Participants[i].Get())
		{
			if (Participant == nullptr)
			{
				Result.Append(TEXT("Invalid Participant"));
				continue;
			}
			
			if (i > 0) Result.Append(TEXT(", "));
			if (AActor* Owner = Participant->Execute_GetOwningActor(Participant->_getUObject()))
			{
				Result.Append(Owner->GetActorLabel());
			}
			else
			{
				Result.Append(TEXT("Unknown Participant"));
			}
		}
	}
	return Result;
}

FMounteaDialogueGraph_Details::FMounteaDialogueGraph_Details()
{
	BeginPIEHandle = FEditorDelegates::BeginPIE.AddLambda([this](bool)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
		InitializePIEInstances();
	});
	
	EndPIEHandle = FEditorDelegates::EndPIE.AddLambda([this](bool)
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

	CustomizedGraph = nullptr;
}

void FMounteaDialogueGraph_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	
	if (Objects.Num() > 0)
	{
		CustomizedObject = Objects[0];
		if (UMounteaDialogueGraph* Graph = Cast<UMounteaDialogueGraph>(Objects[0]))
		{
			CustomizedGraph = Graph;
			CustomizedGraph->OnGraphInitialized.BindRaw(this, &FMounteaDialogueGraph_Details::HandleParticipantRegistration);
		}
	}

	if (!IsValid(CustomizedGraph)) return;
	
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

	InitializePIEInstances();
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

void FMounteaDialogueGraph_Details::InitializePIEInstances()
{
	PIEInstancesMap.Empty();
	PIEInstancesMap.Add(TEXT("None"), FPIEInstanceData());
	
	if (!GEditor) return;
	
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

			FString Key = FString::Printf(TEXT("PIE Instance %d (%s)"), Context.PIEInstance, *InstanceType);
			PIEInstancesMap.Add(Key, FPIEInstanceData(Context.PIEInstance, InstanceType, &Context));
		}
	}

	// Update cached options
	CachedOptions.Reset();
	for (const auto& Entry : PIEInstancesMap)
	{
		CachedOptions.Add(MakeShared<FString>(Entry.Key));
	}
}

void FMounteaDialogueGraph_Details::HandleParticipantRegistration(IMounteaDialogueParticipantInterface* Participant, const UMounteaDialogueGraph* Graph, int32 PIEInstance, bool bIsRegistering)
{
	if (!Participant || Graph != CustomizedGraph) return;

	auto* InstanceData = PIEInstancesMap.Find(GetInstanceKeyForPIE(PIEInstance));
	if (!InstanceData) return;

	if (bIsRegistering)
	{
		InstanceData->Participants.AddUnique(Participant);
	}
	else
	{
		InstanceData->Participants.Remove(Participant);
	}

	UpdateInstanceDisplay(PIEInstance);
}

void FMounteaDialogueGraph_Details::UpdateInstanceDisplay(int32 PIEInstance)
{
	FString OldKey = GetInstanceKeyForPIE(PIEInstance);
	auto* InstanceData = PIEInstancesMap.Find(OldKey);
	if (!InstanceData) return;
	
	FString NewKey = FString::Printf(TEXT("PIE Instance %d (%s) - %s"), 
		PIEInstance, 
		*InstanceData->InstanceType,
		*InstanceData->GetParticipantsDescription());
	
	if (OldKey != NewKey)
	{
		FPIEInstanceData Data = MoveTemp(*InstanceData);
		PIEInstancesMap.Remove(OldKey);
		PIEInstancesMap.Add(NewKey, MoveTemp(Data));
		InitializePIEInstances();
	}
}

FString FMounteaDialogueGraph_Details::GetInstanceKeyForPIE(int32 PIEInstance) const
{
	for (const auto& Entry : PIEInstancesMap)
	{
		if (Entry.Value.InstanceId == PIEInstance)
		{
			return Entry.Key;
		}
	}
	return TEXT("None");
}

const FPIEInstanceData* FMounteaDialogueGraph_Details::GetInstanceData(const FString& InstanceString) const
{
	return PIEInstancesMap.Find(InstanceString);
}

void FMounteaDialogueGraph_Details::OnPIEInstanceSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (!NewValue.IsValid() || !CustomizedObject.IsValid())
		return;
	
	if (const FPIEInstanceData* InstanceData = GetInstanceData(*NewValue.Get()))
	{
		// TODO: Implement visual updates based on instance selection
	}
}

#undef LOCTEXT_NAMESPACE