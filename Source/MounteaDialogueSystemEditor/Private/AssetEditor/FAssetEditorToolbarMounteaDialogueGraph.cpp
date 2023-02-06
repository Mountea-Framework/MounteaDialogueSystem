// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FAssetEditorToolbarMounteaDialogueGraph.h"

#include "AssetEditor_MounteaDialogueGraph.h"
#include "EditorCommands/FMounteaDialogueGraphEditorCommands.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"

#define LOCTEXT_NAMESPACE "AssetEditorToolbarMounteaDialogueGraph"

void FAssetEditorToolbarMounteaDialogueGraph::AddMounteaDialogueGraphToolbar(TSharedPtr<FExtender> Extender)
{
	check(MounteaDialogueGraphEditor.IsValid());
	TSharedPtr<FAssetEditor_MounteaDialogueGraph> MounteaDialogueGraphEditorPtr = MounteaDialogueGraphEditor.Pin();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, MounteaDialogueGraphEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP( this, &FAssetEditorToolbarMounteaDialogueGraph::FillMounteaDialogueGraphToolbar ));
	MounteaDialogueGraphEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FAssetEditorToolbarMounteaDialogueGraph::FillMounteaDialogueGraphToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(MounteaDialogueGraphEditor.IsValid());
	TSharedPtr<FAssetEditor_MounteaDialogueGraph> MounteaDialogueGraphEditorPtr = MounteaDialogueGraphEditor.Pin();

	ToolbarBuilder.BeginSection("Mountea Dialogue Graph");
	{
		ToolbarBuilder.AddToolBarButton(FMounteaDialogueGraphEditorCommands::Get().GraphSettings,
			NAME_None,
			LOCTEXT("GraphSettings_Label", "Graph Settings"),
			LOCTEXT("GraphSettings_ToolTip", "Show the Graph Settings"),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetStyleSetName(),"MDSStyleSet.GraphSettings"));
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Util");
	{
		ToolbarBuilder.AddToolBarButton(FMounteaDialogueGraphEditorCommands::Get().AutoArrange,
			NAME_None,
			LOCTEXT("AutoArrange_Label", "Auto Arrange"),
			LOCTEXT("AutoArrange_ToolTip", "Tries its best to arrange Graph Nodes. Don't judge too harshly please."),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetStyleSetName(), "MDSStyleSet.AutoArrange")); 
	}
	ToolbarBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE
