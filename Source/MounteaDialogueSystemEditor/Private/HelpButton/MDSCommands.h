// All rights reserved Dominik Pavlicek 2022.

#pragma once

#include "CoreMinimal.h"
#include "MDSHelpStyle.h"


class FMDSCommands : public TCommands<FMDSCommands>
{
public:

	FMDSCommands()
	: TCommands<FMDSCommands>(
		TEXT("MounteaSupport"),
		NSLOCTEXT("Contexts", "Support", "Mountea Dialogue System"),
		NAME_None,
		FMDSHelpStyle::GetAppStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	
	TSharedPtr< FUICommandInfo > PluginAction;
	TSharedPtr< FUICommandInfo > DialoguerAction;
};
