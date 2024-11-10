// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "IDetailCustomization.h"

class IMounteaDialogueParticipantInterface;
class STextComboBox;
class UMounteaDialogueGraph;

struct FPIEInstanceData
{
	int32 InstanceId = 0;
	FString InstanceType;
	const FWorldContext* Context = nullptr;
	TArray<TWeakInterfacePtr<IMounteaDialogueParticipantInterface>> Participants;

	FPIEInstanceData() = default;
	FPIEInstanceData(int32 InId, const FString& InType, const FWorldContext* InContext)
		: InstanceId(InId)
		, InstanceType(InType)
		, Context(InContext)
	{}

	FString GetParticipantsDescription() const;
};

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
	void InitializePIEInstances();
	void OnPIEInstanceSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
    
	void HandleParticipantRegistration(IMounteaDialogueParticipantInterface* Participant, const UMounteaDialogueGraph* Graph, int32 PIEInstance, bool bIsRegistering);
	void UpdateInstanceDisplay(int32 PIEInstance);
	FString GetInstanceKeyForPIE(int32 PIEInstance) const;
	const FPIEInstanceData* GetInstanceData(const FString& InstanceString) const;
    
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
