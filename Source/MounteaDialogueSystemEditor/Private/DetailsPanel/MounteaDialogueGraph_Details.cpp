// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraph_Details.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueEditorDetailsTypes.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Interfaces/MounteaDialogueParticipantInterface.h"
#include "Widgets/Input/STextComboBox.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraph_Details"

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
		FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	if (EndPIEHandle.IsValid())
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);

	CustomizedGraph = nullptr;
}

void FMounteaDialogueGraph_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> modifiedObjects;
	DetailBuilder.GetObjectsBeingCustomized(modifiedObjects);
	
	if (modifiedObjects.Num() > 0)
	{
		CustomizedObject = modifiedObjects[0];
		if (UMounteaDialogueGraph* Graph = Cast<UMounteaDialogueGraph>(modifiedObjects[0]))
		{
			CustomizedGraph = Graph;
			CustomizedGraph->OnGraphInitialized.BindRaw(this, &FMounteaDialogueGraph_Details::HandleParticipantRegistration);

			InitializePIEInstances();
		}
	}

	if (!IsValid(CustomizedGraph)) return;
	
	IDetailCategoryBuilder& categoryBuilder = DetailBuilder.EditCategory("Mountea");
	IDetailGroup& PIEGroup = categoryBuilder.AddGroup("PIE Instance", LOCTEXT("MounteaDialogueGraph_DetailsPIE", "PIE Instance Details"));
	
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
}

bool FMounteaDialogueGraph_Details::HasActivePIE()
{
	if (!GEditor) return false;
	
	for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
	{
		if (worldContext.WorldType == EWorldType::PIE)
			return true;
	}
	return false;
}

void FMounteaDialogueGraph_Details::InitializePIEInstances()
{
	TMap<int32, TArray<TWeakInterfacePtr<IMounteaDialogueParticipantInterface>>> OldParticipants;
	for (const auto& Entry : PIEInstancesMap)
	{
		if (Entry.Value.Participants.Num() > 0)
		{
			OldParticipants.Add(Entry.Value.InstanceId, Entry.Value.Participants);
		}
	}
	
	PIEInstancesMap.Empty();
	PIEInstancesMap.Add(TEXT("PIE_None"), FPIEInstanceData());
	PIEInstancesMap[TEXT("PIE_None")].NameOverride = TEXT("None");
	
	if (!GEditor) return;
	
	for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
	{
		if (worldContext.WorldType == EWorldType::PIE)
		{
			FString instanceType;
			if (worldContext.RunAsDedicated)
				instanceType = TEXT("Server");
			else
			{
				if (worldContext.PIEInstance == 2)
					instanceType = TEXT("Client 1");
				else if (worldContext.PIEInstance > 2)
					instanceType = FString::Printf(TEXT("Client %d"), worldContext.PIEInstance - 2);
				else
					instanceType = TEXT("Local");
			}

			const FString stableKey = FString::Printf(TEXT("PIE_%d"), worldContext.PIEInstance);
			FPIEInstanceData NewInstanceData(worldContext.PIEInstance, instanceType, &worldContext);
			
			// Always set base display name
			NewInstanceData.NameOverride = FString::Printf(TEXT("PIE Instance %d (%s)"), 
				worldContext.PIEInstance, 
				*instanceType);
			
			if (const auto* ExistingParticipants = OldParticipants.Find(worldContext.PIEInstance))
			{
				NewInstanceData.Participants = *ExistingParticipants;
				NewInstanceData.Participants.RemoveAll([](const TWeakInterfacePtr<IMounteaDialogueParticipantInterface>& Participant)
				{
					return !Participant.IsValid();
				});
				
				if (NewInstanceData.Participants.Num() > 0)
				{
					NewInstanceData.NameOverride = FString::Printf(TEXT("%s - %s"),
						*NewInstanceData.NameOverride,
						*NewInstanceData.GetParticipantsDescription());
				}
			}

			// Always add the instance, but only show in options if it has participants
			PIEInstancesMap.Add(stableKey, MoveTemp(NewInstanceData));
		}
	}
	
	CachedOptions.Reset();
	CachedOptions.Add(MakeShared<FString>(PIEInstancesMap[TEXT("PIE_None")].NameOverride));
	
	// Add only instances with participants
	for (const auto& Entry : PIEInstancesMap)
	{
		if (Entry.Key != TEXT("PIE_None") && Entry.Value.Participants.Num() > 0)
		{
			CachedOptions.Add(MakeShared<FString>(Entry.Value.NameOverride));
		}
	}
}

void FMounteaDialogueGraph_Details::HandleParticipantRegistration(IMounteaDialogueParticipantInterface* Participant, const UMounteaDialogueGraph* Graph, int32 PIEInstance, bool bIsRegistering)
{
	if (!Participant || Graph != CustomizedGraph) 
		return;

	InitializePIEInstances();

	const FString instanceKey = GetInstanceKeyForPIE(PIEInstance);
	if (instanceKey.IsEmpty())
		return;
    
	auto* instanceData = PIEInstancesMap.Find(GetInstanceKeyForPIE(PIEInstance));
	if (!instanceData)
	{
		EditorLOG_WARNING(TEXT("Could not find PIE instance data for instance %d"), PIEInstance);
		return;
	}

	if (bIsRegistering)
	{
		instanceData->Participants.AddUnique(Participant);
		UpdateInstanceDisplay(PIEInstance);
	}
	else
	{
		EditorLOG_INFO(TEXT("Unregistering participant from PIE instance %d"), PIEInstance);
		instanceData->Participants.Remove(Participant);

		// If this was the last participant, update the UI to remove this instance
		if (instanceData->Participants.Num() == 0 && instanceKey != TEXT("PIE_None"))
		{
			PIEInstancesMap.Remove(instanceKey);
			
			CachedOptions.Reset();
			CachedOptions.Add(MakeShared<FString>(PIEInstancesMap[TEXT("PIE_None")].NameOverride));
			
			for (const auto& Entry : PIEInstancesMap)
			{
				if (Entry.Key != TEXT("PIE_None") && Entry.Value.Participants.Num() > 0)
				{
					CachedOptions.Add(MakeShared<FString>(Entry.Value.NameOverride));
				}
			}
		}
		else
		{
			// If there are still participants, just update the display
			UpdateInstanceDisplay(PIEInstance);
		}
	}
}

void FMounteaDialogueGraph_Details::UpdateInstanceDisplay(int32 PIEInstance)
{
	const FString stableKey = FString::Printf(TEXT("PIE_%d"), PIEInstance);
	auto* instanceData = PIEInstancesMap.Find(stableKey);
	if (!instanceData) return;
	
	// Update only the display name
	instanceData->NameOverride = FString::Printf(TEXT("PIE Instance %d (%s) - %s"), 
		PIEInstance, 
		*instanceData->InstanceType,
		*instanceData->GetParticipantsDescription());
	
	// Refresh the combo box options
	CachedOptions.Reset();
	for (const auto& Entry : PIEInstancesMap)
	{
		CachedOptions.Add(MakeShared<FString>(Entry.Value.NameOverride));
	}
}

FString FMounteaDialogueGraph_Details::GetInstanceKeyForPIE(int32 PIEInstance) const
{
	return FString::Printf(TEXT("PIE_%d"), PIEInstance);
}

const FPIEInstanceData* FMounteaDialogueGraph_Details::GetInstanceData(const FString& DisplayName) const
{
	for (const auto& Entry : PIEInstancesMap)
	{
		if (Entry.Value.NameOverride == DisplayName)
		{
			return &Entry.Value;
		}
	}
	return nullptr;
}

void FMounteaDialogueGraph_Details::OnPIEInstanceSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (!NewValue.IsValid() || !CustomizedObject.IsValid() || !IsValid(CustomizedGraph))
		return;

	UEdGraph_MounteaDialogueGraph* graphEditor = Cast<UEdGraph_MounteaDialogueGraph>(CustomizedGraph->EdGraph);
	if (!graphEditor)
		return;

	if (const FPIEInstanceData* instanceData = GetInstanceData(*NewValue.Get()))
		graphEditor->UpdateFocusedInstance(*instanceData);
}

#undef LOCTEXT_NAMESPACE