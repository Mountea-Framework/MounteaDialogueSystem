// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "AssetEditor_MounteaDialogueGraph.h"

#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "HAL/PlatformApplicationMisc.h"
#include "EdGraphUtilities.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "EditorCommands/FMounteaDialogueGraphEditorCommands.h"
#include "AssetEditor/FAssetEditorToolbarMounteaDialogueGraph.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "GraphScheme/AssetGraphScheme_MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Layout/ForceDirectedSolveLayoutStrategy.h"
#include "Layout/MounteaDialogueGraphLayoutStrategy.h"
#include "Layout/TreeSolveLayoutStrategy.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

#define LOCTEXT_NAMESPACE "AssetEditorMounteaDialogueGraph"

struct FAssetEditorTabs_MounteaDialogueGraph
{
	// Tab identifiers
	static const FName MounteaDialogueGraphPropertyID;
	static const FName ViewportID;
	static const FName MounteaDialogueGraphEditorSettingsID;
};

#pragma region ConstantNames

const FName MounteaDialogueGraphEditorAppName = FName(TEXT("MounteaDialogueGraphEditorApp"));
const FName FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphPropertyID(TEXT("MounteaDialogueGraphProperty"));
const FName FAssetEditorTabs_MounteaDialogueGraph::ViewportID(TEXT("Viewport"));
const FName FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphEditorSettingsID(TEXT("MounteaDialogueGraphEditorSettings"));

#pragma endregion 

FAssetEditor_MounteaDialogueGraph::FAssetEditor_MounteaDialogueGraph()
{
	EditingGraph = nullptr;
	MounteaDialogueGraphEditorSettings = GetMutableDefault<UMounteaDialogueGraphEditorSettings>();
	OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FAssetEditor_MounteaDialogueGraph::OnPackageSaved);
}

FAssetEditor_MounteaDialogueGraph::~FAssetEditor_MounteaDialogueGraph()
{
	UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
}

void FAssetEditor_MounteaDialogueGraph::InitMounteaDialogueGraphAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UMounteaDialogueGraph* Graph)
{
	EditingGraph = Graph;
	CreateEdGraph();

	FGenericCommands::Register();
	FGraphEditorCommands::Register();
	FMounteaDialogueGraphEditorCommands::Register();

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FAssetEditorToolbarMounteaDialogueGraph(SharedThis(this)));
	}

	BindCommands();

	CreateInternalWidgets();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarBuilder->AddMounteaDialogueGraphToolbar(ToolbarExtender);

	// Layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_MounteaDialogueGraphEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(FAssetEditorTabs_MounteaDialogueGraph::ViewportID, ETabState::OpenedTab)->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.7f)
						->AddTab(FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphPropertyID, ETabState::OpenedTab)->SetHideTabWell(true)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.3f)
						->AddTab(FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphEditorSettingsID, ETabState::OpenedTab)
					)
				)
			)
		);
	
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor
	(
		Mode,
		InitToolkitHost,
		MounteaDialogueGraphEditorAppName,
		StandaloneDefaultLayout,
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		EditingGraph,
		false
		);

	RegenerateMenusAndToolbars();
}

UMounteaDialogueGraphEditorSettings* FAssetEditor_MounteaDialogueGraph::GetSettings() const
{
	return MounteaDialogueGraphEditorSettings;
}

void FAssetEditor_MounteaDialogueGraph::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_MounteaDialogueTreeEditor", "Mountea Dialogue Tree Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FAssetEditorTabs_MounteaDialogueGraph::ViewportID, FOnSpawnTab::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphPropertyID, FOnSpawnTab::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Property"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphEditorSettingsID, FOnSpawnTab::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::SpawnTab_EditorSettings))
		.SetDisplayName(LOCTEXT("EditorSettingsTab", "Mountea Dialogue Graph Editor Setttings"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FMounteaDialogueGraphEditorStyle::GetStyleSetName(), "MDSStyleSet.GraphSettings.small"));
}

void FAssetEditor_MounteaDialogueGraph::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FAssetEditorTabs_MounteaDialogueGraph::ViewportID);
	InTabManager->UnregisterTabSpawner(FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphPropertyID);
	InTabManager->UnregisterTabSpawner(FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphEditorSettingsID);
}

FName FAssetEditor_MounteaDialogueGraph::GetToolkitFName() const
{
	return FName("FMounteaDialogueGraphEditor");
}

FText FAssetEditor_MounteaDialogueGraph::GetBaseToolkitName() const
{
	return LOCTEXT("MounteaDialogueGraphEditorAppLabel", "Mountea Dialogue Graph Editor");
}

FText FAssetEditor_MounteaDialogueGraph::GetToolkitName() const
{
	const bool bDirtyState = EditingGraph->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("MounteaDialogueGraphName"), FText::FromString(EditingGraph->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("MounteaDialogueGraphEditorToolkitName", "{MounteaDialogueGraphName}{DirtyState}"), Args);
}

FText FAssetEditor_MounteaDialogueGraph::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(EditingGraph);
}

FLinearColor FAssetEditor_MounteaDialogueGraph::GetWorldCentricTabColorScale() const
{
	return FLinearColor::Gray;
}

FString FAssetEditor_MounteaDialogueGraph::GetWorldCentricTabPrefix() const
{
	return TEXT("MounteaDialogueGraphEditor");
}

FString FAssetEditor_MounteaDialogueGraph::GetDocumentationLink() const
{
	return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki");
}

void FAssetEditor_MounteaDialogueGraph::SaveAsset_Execute()
{
	if (EditingGraph != nullptr)
	{
		RebuildMounteaDialogueGraph();
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

void FAssetEditor_MounteaDialogueGraph::UpdateToolbar()
{
	// TODO
}

void FAssetEditor_MounteaDialogueGraph::RegisterToolbarTab(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FAssetEditor_MounteaDialogueGraph::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(EditingGraph);
	Collector.AddReferencedObject(EditingGraph->EdGraph);
}

void FAssetEditor_MounteaDialogueGraph::CreateInternalWidgets()
{
	ViewportWidget = CreateViewportWidget();

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyWidget = PropertyModule.CreateDetailView(Args);
	PropertyWidget->SetObject(EditingGraph);
	PropertyWidget->OnFinishedChangingProperties().AddSP(this, &FAssetEditor_MounteaDialogueGraph::OnFinishedChangingProperties);

	EditorSettingsWidget = PropertyModule.CreateDetailView(Args);
	EditorSettingsWidget->SetObject(MounteaDialogueGraphEditorSettings);
}

TSharedRef<SGraphEditor> FAssetEditor_MounteaDialogueGraph::CreateViewportWidget()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_MounteaDialogueGraph", "Mountea Dialogue Tree");

	CreateCommandList();

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::OnNodeDoubleClicked);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EditingGraph->EdGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);
}

void FAssetEditor_MounteaDialogueGraph::BindCommands()
{
	ToolkitCommands->MapAction(FMounteaDialogueGraphEditorCommands::Get().GraphSettings,
		FExecuteAction::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::GraphSettings),
		FCanExecuteAction::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::CanGraphSettings)
	);

	ToolkitCommands->MapAction(FMounteaDialogueGraphEditorCommands::Get().AutoArrange,
		FExecuteAction::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::AutoArrange),
		FCanExecuteAction::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::CanAutoArrange)
	);
}

void FAssetEditor_MounteaDialogueGraph::CreateEdGraph()
{
	if (EditingGraph->EdGraph == nullptr)
	{
		EditingGraph->EdGraph = CastChecked<UEdGraph_MounteaDialogueGraph>(FBlueprintEditorUtils::CreateNewGraph(EditingGraph, NAME_None, UEdGraph_MounteaDialogueGraph::StaticClass(), UAssetGraphScheme_MounteaDialogueGraph::StaticClass()));
		EditingGraph->EdGraph->bAllowDeletion = false;

		// Give the schema a chance to fill out any required nodes (like the results node)
		const UEdGraphSchema* Schema = EditingGraph->EdGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*EditingGraph->EdGraph);

		UEdGraph_MounteaDialogueGraph* MounteaDialogueGraph = Cast<UEdGraph_MounteaDialogueGraph>(EditingGraph->EdGraph);

		const auto NewNode = MounteaDialogueGraph->CreateIntermediateNode<UEdNode_MounteaDialogueGraphNode>();
	
		NewNode->SetMounteaDialogueGraphNode(EditingGraph->StartNode);
		NewNode->CreateNewGuid();
		NewNode->PostPlacedNewNode();
		NewNode->AllocateDefaultPins();
		NewNode->AutowireNewNode(nullptr);

		NewNode->NodePosX = 0;
		NewNode->NodePosY = 0;

		NewNode->DialogueGraphNode->SetFlags(RF_Transactional);
		NewNode->SetFlags(RF_Transactional);
		
		MounteaDialogueGraph->RebuildMounteaDialogueGraph();
	}
}

void FAssetEditor_MounteaDialogueGraph::CreateCommandList()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	GraphEditorCommands = MakeShareable(new FUICommandList);

	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class

	GraphEditorCommands->MapAction(FMounteaDialogueGraphEditorCommands::Get().GraphSettings,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::GraphSettings),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanGraphSettings));

	GraphEditorCommands->MapAction(FMounteaDialogueGraphEditorCommands::Get().AutoArrange,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::AutoArrange),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanAutoArrange));

	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MounteaDialogueGraph::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::OnRenameNode),
		FCanExecuteAction::CreateSP(this, &FAssetEditor_MounteaDialogueGraph::CanRenameNodes)
	);
}

TSharedPtr<SGraphEditor> FAssetEditor_MounteaDialogueGraph::GetCurrGraphEditor() const
{
	return ViewportWidget;
}

FGraphPanelSelectionSet FAssetEditor_MounteaDialogueGraph::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GetCurrGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

void FAssetEditor_MounteaDialogueGraph::RebuildMounteaDialogueGraph()
{
	if (EditingGraph == nullptr)
	{
		LOG_WARNING(TEXT("[RebuildMounteaDialogueGraph] EditingGraph is nullptr"));
		return;
	}

	UEdGraph_MounteaDialogueGraph* EdGraph = Cast<UEdGraph_MounteaDialogueGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	EdGraph->RebuildMounteaDialogueGraph();
}

void FAssetEditor_MounteaDialogueGraph::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}

bool FAssetEditor_MounteaDialogueGraph::CanSelectAllNodes()
{
	return true;
}

void FAssetEditor_MounteaDialogueGraph::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* EdNode = Cast<UEdGraphNode>(*NodeIt);
		if (EdNode == nullptr || !EdNode->CanUserDeleteNode())
			continue;;

		if (UEdNode_MounteaDialogueGraphNode* EdNode_Node = Cast<UEdNode_MounteaDialogueGraphNode>(EdNode))
		{
			EdNode_Node->Modify();

			const UEdGraphSchema* Schema = EdNode_Node->GetSchema();
			if (Schema != nullptr)
			{
				Schema->BreakNodeLinks(*EdNode_Node);
			}

			EdNode_Node->DestroyNode();
		}
		else
		{
			EdNode->Modify();
			EdNode->DestroyNode();
		}
	}
}

bool FAssetEditor_MounteaDialogueGraph::CanDeleteNodes()
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node != nullptr && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FAssetEditor_MounteaDialogueGraph::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

void FAssetEditor_MounteaDialogueGraph::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FAssetEditor_MounteaDialogueGraph::CanCutNodes()
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FAssetEditor_MounteaDialogueGraph::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		if (UEdNode_MounteaDialogueGraphEdge* EdNode_Edge = Cast<UEdNode_MounteaDialogueGraphEdge>(*SelectedIter))
		{
			UEdNode_MounteaDialogueGraphNode* StartNode = EdNode_Edge->GetStartNode();
			UEdNode_MounteaDialogueGraphNode* EndNode = EdNode_Edge->GetEndNode();

			if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
			{
				SelectedIter.RemoveCurrent();
				continue;
			}
		}

		Node->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FAssetEditor_MounteaDialogueGraph::CanCopyNodes()
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode() == false)
		{
			return false;
		}
	}

	return true;
}

void FAssetEditor_MounteaDialogueGraph::PasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}
}

void FAssetEditor_MounteaDialogueGraph::PasteNodesHere(const FVector2D& Location)
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}
	// Select the newly pasted stuff
	UEdGraph* EdGraph = CurrentGraphEditor->GetCurrentGraph();

	{
		const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
		EdGraph->Modify();

		// Clear the selection set (newly pasted stuff will be selected)
		CurrentGraphEditor->ClearSelectionSet();

		// Grab the text to paste from the clipboard.
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		// Import the nodes
		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

		//Average position of nodes so we can move them while still maintaining relative distances to each other
		FVector2D AvgNodePosition(0.0f, 0.0f);

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			AvgNodePosition.X += Node->NodePosX;
			AvgNodePosition.Y += Node->NodePosY;
		}

		float InvNumNodes = 1.0f / float(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			CurrentGraphEditor->SetNodeSelection(Node, true);

			Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
			Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

			Node->SnapToGrid(16);

			// Give new node a different Guid from the old one
			Node->CreateNewGuid();
		}
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	UObject* GraphOwner = EdGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
}

bool FAssetEditor_MounteaDialogueGraph::CanPasteNodes()
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		const UEdNode_MounteaDialogueGraphNode* Node = Cast<UEdNode_MounteaDialogueGraphNode>(*SelectedIter);
		if (Node && Node->CanUserPasteNodes() == false)
		{
			return false;
		}
	}
	
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FAssetEditor_MounteaDialogueGraph::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FAssetEditor_MounteaDialogueGraph::CanDuplicateNodes()
{
	return CanCopyNodes();
}

void FAssetEditor_MounteaDialogueGraph::GraphSettings()
{
	PropertyWidget->SetObject(EditingGraph);
}

bool FAssetEditor_MounteaDialogueGraph::CanGraphSettings() const
{
	return true;
}

void FAssetEditor_MounteaDialogueGraph::AutoArrange()
{
	UEdGraph_MounteaDialogueGraph* EdGraph = Cast<UEdGraph_MounteaDialogueGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	const FScopedTransaction Transaction(LOCTEXT("MounteaDialogueGraphEditorAutoArrange", "Mountea Dialogue Graph Editor: Auto Arrange all Nodes"));

	EdGraph->Modify(true);

	UMounteaDialogueGraphLayoutStrategy* LayoutStrategy = nullptr;
	switch (MounteaDialogueGraphEditorSettings->GetAutoLayoutStrategy())
	{
		case EAutoLayoutStrategyType::EALS_Tree:
			LayoutStrategy = NewObject<UMounteaDialogueGraphLayoutStrategy>(EdGraph, UTreeSolveLayoutStrategy::StaticClass());
			break;
		case EAutoLayoutStrategyType::EALS_ForceDirected:
			LayoutStrategy = NewObject<UMounteaDialogueGraphLayoutStrategy>(EdGraph, UForceDirectedSolveLayoutStrategy::StaticClass());
			break;
		default:
			break;
	}

	if (LayoutStrategy != nullptr)
	{
		LayoutStrategy->Settings = MounteaDialogueGraphEditorSettings;
		LayoutStrategy->Layout(EdGraph);
		LayoutStrategy->ConditionalBeginDestroy();
	}
	else
	{
		LOG_ERROR(TEXT("[AutoArrange] LayoutStrategy is null."));
	}
}

bool FAssetEditor_MounteaDialogueGraph::CanAutoArrange() const
{
	return EditingGraph != nullptr && Cast<UEdGraph_MounteaDialogueGraph>(EditingGraph->EdGraph) != nullptr;
}

void FAssetEditor_MounteaDialogueGraph::OnRenameNode()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode != nullptr && SelectedNode->bCanRenameNode)
			{
				CurrentGraphEditor->IsNodeTitleVisible(SelectedNode, true);
				break;
			}
		}
	}
}

bool FAssetEditor_MounteaDialogueGraph::CanRenameNodes() const
{
	UEdGraph_MounteaDialogueGraph* EdGraph = Cast<UEdGraph_MounteaDialogueGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	UMounteaDialogueGraph* Graph = EdGraph->GetMounteaDialogueGraph();
	check(Graph != nullptr)

	return Graph->bCanRenameNode && GetSelectedNodes().Num() == 1;
}

void FAssetEditor_MounteaDialogueGraph::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	for (UObject* SelectionEntry : NewSelection)
	{
		Selection.Add(SelectionEntry);
	}

	if (Selection.Num() == 0) 
	{
		PropertyWidget->SetObject(EditingGraph);

	}
	else
	{
		PropertyWidget->SetObjects(Selection);
	}
}

void FAssetEditor_MounteaDialogueGraph::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	// TODO StartRenaming
}

void FAssetEditor_MounteaDialogueGraph::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (EditingGraph == nullptr)
		return;

	EditingGraph->EdGraph->GetSchema()->ForceVisualizationCacheClear();
}

void FAssetEditor_MounteaDialogueGraph::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{
	RebuildMounteaDialogueGraph();
}

TSharedRef<SDockTab> FAssetEditor_MounteaDialogueGraph::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FAssetEditorTabs_MounteaDialogueGraph::ViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"));

	if (ViewportWidget.IsValid())
	{
		SpawnedTab->SetContent(ViewportWidget.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FAssetEditor_MounteaDialogueGraph::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphPropertyID);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("Details_Title", "Property"))
		[
			PropertyWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FAssetEditor_MounteaDialogueGraph::SpawnTab_EditorSettings(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FAssetEditorTabs_MounteaDialogueGraph::MounteaDialogueGraphEditorSettingsID);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("EditorSettings_Title", "Mountea Dialogue Graph Editor Setttings"))
		[
			EditorSettingsWidget.ToSharedRef()
		];
}

#undef LOCTEXT_NAMESPACE

