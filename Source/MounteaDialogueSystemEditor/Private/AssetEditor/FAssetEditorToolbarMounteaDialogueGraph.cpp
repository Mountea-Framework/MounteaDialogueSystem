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

	ToolbarBuilder.BeginSection("Util");
	{
		ToolbarBuilder.AddToolBarButton(FMounteaDialogueGraphEditorCommands::Get().AutoArrange,
			NAME_None,
			LOCTEXT("AutoArrange_Label", "Auto Arrange"),
			LOCTEXT("AutoArrange_ToolTip", "ALPHA version!\n\nTries its best to arrange Graph Nodes. Don't judge too harshly please."),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.AutoArrange"));

		ToolbarBuilder.AddToolBarButton(FMounteaDialogueGraphEditorCommands::Get().ValidateGraph,
			NAME_None,
			LOCTEXT("ValidateGraph_Label", "Validate Graph"),
			LOCTEXT("ValidateGraph_ToolTip", "Validates Graph if there are any invalid connections or broken data."),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.ValidateGraph"));

		ToolbarBuilder.AddToolBarButton(FMounteaDialogueGraphEditorCommands::Get().ExportGraph,
			NAME_None,
			LOCTEXT("ExportDialogueGraph_Label", "Export Dialogue Graph"),
			LOCTEXT("ExportDialogueGraph_Tooltip", "📤Export the Dialogue Graph as a file containing MNTEADLG data."),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.ExportGraph"));
	}
	ToolbarBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE
