#pragma once

#include "CoreMinimal.h"

class FMounteaDialogueGraphEditorCommands : public TCommands<FMounteaDialogueGraphEditorCommands>
{

public:

	FMounteaDialogueGraphEditorCommands()
	: TCommands<FMounteaDialogueGraphEditorCommands>("MounteaDialogueGraphEditor", NSLOCTEXT("Contexts", "MounteaDialogueGraphEditor", "Mountea Dialogue Graph Editor"), NAME_None, FAppStyle::GetAppStyleSetName())
	{
	}
	
	TSharedPtr<FUICommandInfo> AutoArrange;
	TSharedPtr<FUICommandInfo> ValidateGraph;

	TSharedPtr<FUICommandInfo> FindInDialogue;
	TSharedPtr<FUICommandInfo> ExportGraph;

	virtual void RegisterCommands() override;
};