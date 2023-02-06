// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

class UMounteaDialogueGraph;
class UMounteaDialogueGraphEditorSettings;
class FGGAssetEditorToolbar;
class FAssetEditorToolbarMounteaDialogueGraph;

class FAssetEditor_MounteaDialogueGraph : public FAssetEditorToolkit, public FNotifyHook, public FGCObject
{

public:

	FAssetEditor_MounteaDialogueGraph();
	virtual ~FAssetEditor_MounteaDialogueGraph() override;

	void InitGenericGraphAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UMounteaDialogueGraph* Graph);
	UMounteaDialogueGraphEditorSettings* GetSettings() const;
	
#pragma region ToolkitInterface
	
public:

	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	// End of IToolkit interface

#pragma endregion

#pragma region AssetEditorToolkit

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FString GetDocumentationLink() const override;
	virtual void SaveAsset_Execute() override;

#pragma endregion

#pragma region Toolbar

	void UpdateToolbar();
	TSharedPtr<FAssetEditorToolbarMounteaDialogueGraph> GetToolbarBuilder() { return ToolbarBuilder; }
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager);

#pragma endregion

#pragma region SerializableObjectInterface

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

#pragma endregion

private:
	
	void CreateInternalWidgets();
	TSharedRef<SGraphEditor> CreateViewportWidget();

	void BindCommands();
	void CreateEdGraph();
	void CreateCommandList();

	TSharedPtr<SGraphEditor> GetCurrGraphEditor() const;

	FGraphPanelSelectionSet GetSelectedNodes() const;

	void RebuildMounteaDialogueGraph();

#pragma region GraphEditorCommands

	void SelectAllNodes();
	bool CanSelectAllNodes();
	void DeleteSelectedNodes();
	bool CanDeleteNodes();
	void DeleteSelectedDuplicatableNodes();
	void CutSelectedNodes();
	bool CanCutNodes();
	void CopySelectedNodes();
	bool CanCopyNodes();
	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes();
	void DuplicateNodes();
	bool CanDuplicateNodes();

	void GraphSettings();
	bool CanGraphSettings() const;

	void AutoArrange();
	bool CanAutoArrange() const;

	void OnRenameNode();
	bool CanRenameNodes() const;

#pragma endregion

#pragma region GraphEditorEvents

	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	void OnNodeDoubleClicked(UEdGraphNode* Node);

	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	void OnPackageSaved(const FString& PackageFileName, UObject* Outer);

#pragma endregion 

#pragma region Variables
	
private:

	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EditorSettings(const FSpawnTabArgs& Args);

	UMounteaDialogueGraphEditorSettings* MounteaDialogueGraphEditorSettings;

	UMounteaDialogueGraph* EditingGraph;

	//Toolbar
	TSharedPtr<FAssetEditorToolbarMounteaDialogueGraph> ToolbarBuilder;

	/** Handle to the registered OnPackageSave delegate */
	FDelegateHandle OnPackageSavedDelegateHandle;

	TSharedPtr<SGraphEditor> ViewportWidget;
	TSharedPtr<class IDetailsView> PropertyWidget;
	TSharedPtr<class IDetailsView> EditorSettingsWidget;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

#pragma endregion 
};
