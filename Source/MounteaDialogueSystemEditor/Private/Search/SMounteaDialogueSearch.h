// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

#include "Framework/Commands/UICommandList.h"

#include "Search/MounteaDialogueSearchFilter.h"
#include "Search/MounteaDialogueSearchResult.h"

class SSearchBox;
class SDockTab;
class FAssetEditor_MounteaDialogueGraph;

/**
 * Widget handling Search in Mountea Dialogue
 */
class SMounteaDialogueSearch : public SCompoundWidget
{
private:
	typedef SMounteaDialogueSearch Self;
	
public:
	SLATE_BEGIN_ARGS(Self)
		: _bIsSearchWindow(true)
		, _bHideSearchBar(false)
		, _ContainingTab()
	{}
		SLATE_ARGUMENT(bool, bIsSearchWindow)
		SLATE_ARGUMENT(bool, bHideSearchBar)
		SLATE_ARGUMENT(TSharedPtr<SDockTab>, ContainingTab)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FAssetEditor_MounteaDialogueGraph>& InDialogueEditor = nullptr);
	virtual ~SMounteaDialogueSearch() override;

	/** Focuses this widget's search box, and changes the mode as well, and optionally the search terms */
	void FocusForUse(const FMounteaDialogueSearchFilter& SearchFilter = FMounteaDialogueSearchFilter(), bool bSelectFirstResult = false);

	/**
	 * Submits a search query
	 *
	 * @param SearchFilter						Filter for search
	 * @param bInIsFindWithinDialogue			TRUE if searching within the current Dialogue only
	 */
	void MakeSearchQuery(const FMounteaDialogueSearchFilter& SearchFilter);

	/** If this is a global find results widget, returns the host tab's unique ID. Otherwise, returns NAME_None. */
	FName GetHostTabId() const;

	/** If this is a global find results widget, ask the host tab to close */
	void CloseHostTab();

private:
	/** Called when the host tab is closed (if valid) */
	void HandleHostTabClosed(TSharedRef<SDockTab> DockTab);

	/** Called when user changes the text they are searching for */
	void HandleSearchTextChanged(const FText& Text);

	/** Called when user changes commits text to the search box */
	void HandleSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType);

	/** Called when the find mode checkbox is hit */
	void HandleFindModeChanged(ECheckBoxState CheckState)
	{
		bIsInFindWithinDialogueMode = CheckState == ECheckBoxState::Checked;
	}

	/** Called to check what the find mode is for the checkbox */
	ECheckBoxState HandleGetFindModeChecked() const
	{
		return bIsInFindWithinDialogueMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	/* Get the children of a row */
	void HandleGetChildren(TSharedPtr<FMounteaDialogueSearchResult> InItem, TArray<TSharedPtr<FMounteaDialogueSearchResult>>& OutChildren);

	/* Called when user double clicks on a new result */
	void HandleTreeSelectionDoubleClicked(TSharedPtr<FMounteaDialogueSearchResult> Item);

	/* Called when a new row is being generated */
	TSharedRef<ITableRow> HandleGenerateRow(TSharedPtr<FMounteaDialogueSearchResult> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Callback to build the context menu when right clicking in the tree */
	TSharedPtr<SWidget> HandleContextMenuOpening();

	/** Fills in the filter menu. */
	TSharedRef<SWidget> FillFilterEntries();

private:
	/** Pointer back to the Dialogue editor that owns us */
	TWeakPtr<FAssetEditor_MounteaDialogueGraph> DialogueEditorPtr;

	/* The tree view displays the results */
	TSharedPtr<STreeView<TSharedPtr<FMounteaDialogueSearchResult>>> TreeView;

	/** The search text box */
	TSharedPtr<SSearchBox> SearchTextBoxWidget;

	/** Vertical box, used to add and remove widgets dynamically */
	TWeakPtr<SVerticalBox> MainVerticalBoxWidget;

	/** In Find Within Dialogue mode, we need to keep a handle on the root result, because it won't show up in the tree. */
	TSharedPtr<FMounteaDialogueSearchResult> RootSearchResult;

	/* This buffer stores the currently displayed results */
	TArray<TSharedPtr<FMounteaDialogueSearchResult>> ItemsFound;

	/* The string to highlight in the results */
	FText HighlightText;

	/** The current searach filter */
	FMounteaDialogueSearchFilter CurrentFilter;

	/** Should we search within the current Dialogue only (rather than all Dialogues) */
	bool bIsInFindWithinDialogueMode;

	/** Tab hosting this widget. May be invalid. */
	TWeakPtr<SDockTab> HostTab;

	/** Commands handled by this widget */
	TSharedPtr<FUICommandList> CommandList;
	
};
