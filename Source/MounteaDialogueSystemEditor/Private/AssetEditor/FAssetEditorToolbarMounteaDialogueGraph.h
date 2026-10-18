// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not 
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

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
