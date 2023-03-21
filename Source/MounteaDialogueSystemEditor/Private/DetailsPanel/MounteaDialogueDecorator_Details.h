// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "IDetailCustomization.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"

class FMounteaDialogueDecorator_Details : public IDetailCustomization
{
	typedef FMounteaDialogueDecorator_Details Self;

public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<Self>(); }
	
	// IDetailCustomization interface
	/** Called when details should be customized */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:

	IDetailLayoutBuilder* SavedLayoutBuilder = nullptr;

	UMounteaDialogueDecoratorBase* EditingDecorator = nullptr;
};
