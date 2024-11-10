// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "IDetailCustomization.h"

class STextComboBox;

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
	static TArray<TSharedPtr<FString>> GetPIEInstances();
	void OnPIEInstanceSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	
	TSharedPtr<FString> GetCurrentPIEInstance() const;
	
	TArray<TSharedPtr<FString>> CachedOptions;
	TWeakObjectPtr<UObject> CustomizedObject;
	
	FDelegateHandle BeginPIEHandle;
	FDelegateHandle EndPIEHandle;
};
