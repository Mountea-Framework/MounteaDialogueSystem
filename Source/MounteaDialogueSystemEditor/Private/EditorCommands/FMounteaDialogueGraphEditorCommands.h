#pragma once

#include "CoreMinimal.h"

class FMounteaDialogueGraphEditorCommands : public TCommands<FMounteaDialogueGraphEditorCommands>
{

public:

	FMounteaDialogueGraphEditorCommands()
	: TCommands<FMounteaDialogueGraphEditorCommands>("MounteaDialogueGraphEditor", NSLOCTEXT("Contexts", "MounteaDialogueGraphEditor", "Mountea Dialogue Graph Editor"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}
	
	TSharedPtr<FUICommandInfo> AutoArrange;
	TSharedPtr<FUICommandInfo> ValidateGraph;

	virtual void RegisterCommands() override;
};
