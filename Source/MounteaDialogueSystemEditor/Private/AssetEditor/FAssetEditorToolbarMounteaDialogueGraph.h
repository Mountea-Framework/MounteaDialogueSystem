// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
class FAssetEditor_MounteaDialogueGraph;

class FAssetEditorToolbarMounteaDialogueGraph : public TSharedFromThis<FAssetEditorToolbarMounteaDialogueGraph>
{
public:
	FAssetEditorToolbarMounteaDialogueGraph(TSharedPtr<FAssetEditor_MounteaDialogueGraph> InMounteaDialogueGraphEditor)
		: MounteaDialogueGraphEditor(InMounteaDialogueGraphEditor) {}

	void AddMounteaDialogueGraphToolbar(TSharedPtr<FExtender> Extender);

private:
	void FillMounteaDialogueGraphToolbar(FToolBarBuilder& ToolbarBuilder);

protected:
	/** Pointer back to the blueprint editor tool that owns us */
	TWeakPtr<FAssetEditor_MounteaDialogueGraph> MounteaDialogueGraphEditor;
	
};
