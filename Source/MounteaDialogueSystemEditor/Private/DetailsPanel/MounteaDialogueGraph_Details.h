﻿// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "IDetailCustomization.h"
#include "Helpers/MounteaDialogueEditorDetailsTypes.h"

class IMounteaDialogueParticipantInterface;
class STextComboBox;
class UMounteaDialogueGraph;

class FMounteaDialogueGraph_Details : public IDetailCustomization
{
	typedef FMounteaDialogueGraph_Details Self;

public:
	FMounteaDialogueGraph_Details();
	virtual ~FMounteaDialogueGraph_Details() override;

	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<Self>(); }
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	static bool HasActivePIE();
	void ResetPIEInstances();
	void InitializePIEInstances();
	void OnPIEInstanceSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
    
	void HandleParticipantRegistration(IMounteaDialogueParticipantInterface* Participant, const UMounteaDialogueGraph* Graph, int32 PIEInstance, bool bIsRegistering);
	void UpdateInstanceDisplay(int32 PIEInstance);
	const FWorldContext* GetWorldContextForPIE(int32 PIEInstance) const;
	FString GetInstanceKeyForPIE(int32 PIEInstance) const;
	const FPIEInstanceData* GetInstanceData(const FString& DisplayName) const;

	void UpdateEditorFocusedInstance(TSharedPtr<FString> NewValue);
    
	// UI Data
	TArray<TSharedPtr<FString>> CachedOptions;
	TMap<FString, FPIEInstanceData> PIEInstancesMap;
    
	// References
	TWeakObjectPtr<UObject> CustomizedObject;
	UMounteaDialogueGraph* CustomizedGraph = nullptr;
    
	// Handles
	FDelegateHandle BeginPIEHandle;
	FDelegateHandle EndPIEHandle;
};
