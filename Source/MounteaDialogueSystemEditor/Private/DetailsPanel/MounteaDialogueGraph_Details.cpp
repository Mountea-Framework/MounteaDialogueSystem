// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueGraph_Details.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EngineUtils.h"
#include "IDetailGroup.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueEditorDetailsTypes.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"
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
		UpdateEditorFocusedInstance(nullptr);
		ResetPIEInstances();
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

void FMounteaDialogueGraph_Details::ResetPIEInstances()
{
	CachedOptions.Empty();
	PIEInstancesMap.Empty();
}

void FMounteaDialogueGraph_Details::InitializePIEInstances()
{
	TArray<FPIEInstanceData> OldInstances;
	for (const auto& Entry : PIEInstancesMap)
	{
		if (Entry.Value.Participant.IsValid())
		{
			OldInstances.Add(Entry.Value);
		}
	}
	
	PIEInstancesMap.Empty();
	PIEInstancesMap.Add(TEXT("PIE_None"), FPIEInstanceData());
	PIEInstancesMap[TEXT("PIE_None")].NameOverride = TEXT("None");
	
	if (!GEditor) return;
	
	// First, restore any existing valid instances
	for (const auto& OldInstance : OldInstances)
	{
		if (OldInstance.Participant.IsValid())
		{
			const FString stableKey = FString::Printf(TEXT("PIE_%d_%s"), 
				OldInstance.InstanceId, 
				*OldInstance.GetParticipantDescription());
			PIEInstancesMap.Add(stableKey, OldInstance);
		}
	}

	auto FindParticipantInActor = [this](AActor* Actor, const FWorldContext& Context) -> bool
	{
		// First check if the actor itself implements the interface
		if (IMounteaDialogueParticipantInterface* Participant = Cast<IMounteaDialogueParticipantInterface>(Actor))
		{
			if (Participant->GetDialogueGraph() == CustomizedGraph)
			{
				FString instanceType;
				if (Context.RunAsDedicated)
					instanceType = TEXT("Server");
				else if (!Context.LastURL.Host.IsEmpty() || Context.PIEInstance > 1)
					instanceType = FString::Printf(TEXT("Client %d"), Context.PIEInstance - 1);
				else
					instanceType = Context.RunAsDedicated ? TEXT("Listen Server") : TEXT("Local");

				FPIEInstanceData NewInstanceData(Context.PIEInstance, instanceType, &Context);
				NewInstanceData.Participant = Participant;

				const FString stableKey = FString::Printf(TEXT("PIE_%d_%s"), 
					Context.PIEInstance, 
					*NewInstanceData.GetParticipantDescription());

				NewInstanceData.NameOverride = FString::Printf(TEXT("PIE Instance %d (%s) - %s"), 
					Context.PIEInstance, 
					*instanceType,
					*NewInstanceData.GetParticipantDescription());

				PIEInstancesMap.Add(stableKey, MoveTemp(NewInstanceData));
				return true;
			}
		}

		// Then check all components
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		
		for (UActorComponent* Component : Components)
		{
			if (IMounteaDialogueParticipantInterface* Participant = Cast<IMounteaDialogueParticipantInterface>(Component))
			{
				if (Participant->Execute_GetDialogueGraph(Component) == CustomizedGraph)
				{
					FString instanceType;
					if (Context.RunAsDedicated)
						instanceType = TEXT("Server");
					else if (!Context.LastURL.Host.IsEmpty() || Context.PIEInstance > 1)
						instanceType = FString::Printf(TEXT("Client %d"), Context.PIEInstance - 1);
					else
						instanceType = Context.RunAsDedicated ? TEXT("Listen Server") : TEXT("Local");

					FPIEInstanceData NewInstanceData(Context.PIEInstance, instanceType, &Context);
					NewInstanceData.Participant = Participant;

					const FString stableKey = FString::Printf(TEXT("PIE_%d_%s"), 
						Context.PIEInstance, 
						*NewInstanceData.GetParticipantDescription());

					NewInstanceData.NameOverride = FString::Printf(TEXT("PIE Instance %d (%s) - %s"), 
						Context.PIEInstance, 
						*instanceType,
						*NewInstanceData.GetParticipantDescription());

					PIEInstancesMap.Add(stableKey, MoveTemp(NewInstanceData));
					return true;
				}
			}
		}

		return false;
	};
	
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType != EWorldType::PIE) continue;
		
		bool bInstanceFound = false;
		for (const auto& Entry : PIEInstancesMap)
		{
			if (Entry.Value.InstanceId == Context.PIEInstance)
			{
				bInstanceFound = true;
				break;
			}
		}
		if (!bInstanceFound && Context.World())
		{
			// Find participants either in actors or their components
			for (TActorIterator<AActor> It(Context.World()); It; ++It)
			{
				if (FindParticipantInActor(*It, Context))
				{
					break;
				}
			}
		}
	}

	// Update the cached options
	CachedOptions.Reset();
	CachedOptions.Add(MakeShared<FString>(PIEInstancesMap[TEXT("PIE_None")].NameOverride));
	
	for (const auto& Entry : PIEInstancesMap)
	{
		if (Entry.Key != TEXT("PIE_None"))
		{
			CachedOptions.Add(MakeShared<FString>(Entry.Value.NameOverride));
		}
	}
}

void FMounteaDialogueGraph_Details::HandleParticipantRegistration(IMounteaDialogueParticipantInterface* Participant, const UMounteaDialogueGraph* Graph, int32 PIEInstance, bool bIsRegistering)
{
	if (!Participant || Graph != CustomizedGraph) 
		return;

	if (bIsRegistering)
	{
		// Create new instance for this participant
		FString instanceType;
		if (const FWorldContext* Context = GetWorldContextForPIE(PIEInstance))
		{
			if (Context->RunAsDedicated)
				instanceType = TEXT("Server");
			else if (!Context->LastURL.Host.IsEmpty() || Context->PIEInstance > 1)
				instanceType = FString::Printf(TEXT("Client %d"), Context->PIEInstance - 1);
			else
				instanceType = Context->RunAsDedicated ? TEXT("Listen Server") : TEXT("Local");
			
			FPIEInstanceData NewInstanceData(PIEInstance, instanceType, Context);
			NewInstanceData.Participant = Participant;
			
			// Create unique key for this participant
			const FString stableKey = FString::Printf(TEXT("PIE_%d_%s"), PIEInstance, *NewInstanceData.GetParticipantDescription());
			
			// Set display name
			NewInstanceData.NameOverride = FString::Printf(TEXT("PIE Instance %d (%s) - %s"), 
				PIEInstance, 
				*instanceType,
				*NewInstanceData.GetParticipantDescription());
			
			PIEInstancesMap.Add(stableKey, MoveTemp(NewInstanceData));
			
			// Update options
			CachedOptions.Reset();
			CachedOptions.Add(MakeShared<FString>(PIEInstancesMap[TEXT("PIE_None")].NameOverride));
			for (const auto& Entry : PIEInstancesMap)
			{
				if (Entry.Key != TEXT("PIE_None"))
				{
					CachedOptions.Add(MakeShared<FString>(Entry.Value.NameOverride));
				}
			}
		}
	}
	else
	{
		// Find and remove this participant's instance
		TArray<FString> KeysToRemove;
		for (const auto& Entry : PIEInstancesMap)
		{
			if (Entry.Value.InstanceId == PIEInstance && Entry.Value.Participant.Get() == Participant)
			{
				KeysToRemove.Add(Entry.Key);
			}
		}
		
		for (const auto& Key : KeysToRemove)
		{
			PIEInstancesMap.Remove(Key);
		}
		
		// Update options
		CachedOptions.Reset();
		CachedOptions.Add(MakeShared<FString>(PIEInstancesMap[TEXT("PIE_None")].NameOverride));
		for (const auto& Entry : PIEInstancesMap)
		{
			if (Entry.Key != TEXT("PIE_None"))
			{
				CachedOptions.Add(MakeShared<FString>(Entry.Value.NameOverride));
			}
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
		*instanceData->GetParticipantDescription());
	
	// Refresh the combo box options
	CachedOptions.Reset();
	for (const auto& Entry : PIEInstancesMap)
	{
		CachedOptions.Add(MakeShared<FString>(Entry.Value.NameOverride));
	}
}

const FWorldContext* FMounteaDialogueGraph_Details::GetWorldContextForPIE(int32 PIEInstance) const
{
	if (!GEditor) return nullptr;
	
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.PIEInstance == PIEInstance)
		{
			return &Context;
		}
	}
	return nullptr;
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
			return &Entry.Value;
	}
	return nullptr;
}

void FMounteaDialogueGraph_Details::UpdateEditorFocusedInstance(TSharedPtr<FString> NewValue)
{
	UEdGraph_MounteaDialogueGraph* graphEditor = Cast<UEdGraph_MounteaDialogueGraph>(CustomizedGraph->EdGraph);
	if (!graphEditor)
		return;

	if (!NewValue.IsValid())
	{
		graphEditor->UpdateFocusedInstance(FPIEInstanceData());
		return;
	}
	
	if (const FPIEInstanceData* instanceData = GetInstanceData(*NewValue.Get()))
		graphEditor->UpdateFocusedInstance(*instanceData);
	else
		graphEditor->UpdateFocusedInstance(FPIEInstanceData());
}

void FMounteaDialogueGraph_Details::OnPIEInstanceSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (!NewValue.IsValid() || !CustomizedObject.IsValid() || !IsValid(CustomizedGraph))
		return;

	UpdateEditorFocusedInstance(NewValue);
}

#undef LOCTEXT_NAMESPACE