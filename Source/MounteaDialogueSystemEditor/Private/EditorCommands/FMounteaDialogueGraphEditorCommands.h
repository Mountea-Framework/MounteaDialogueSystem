#pragma once

#include "CoreMinimal.h"

class FMounteaDialogueGraphEditorCommands : public TCommands<FMounteaDialogueGraphEditorCommands>
{

public:

	FMounteaDialogueGraphEditorCommands()
	: TCommands<FMounteaDialogueGraphEditorCommands>("MounteaDialogueGraphEditor", NSLOCTEXT("Contexts", "MounteaDialogueGraphEditor", "Mountea Dialogue Graph Editor"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}
	
	TSharedPtr<FUICommandInfo> GraphSettings;
	TSharedPtr<FUICommandInfo> AutoArrange;

	virtual void RegisterCommands() override;

};
