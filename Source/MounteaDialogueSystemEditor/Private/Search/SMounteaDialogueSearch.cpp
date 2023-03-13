// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "SMounteaDialogueSearch.h"

#include "MounteaDialogueSearchManager.h"
#include "AssetEditor/AssetEditor_MounteaDialogueGraph.h"
#include "Framework/Commands/GenericCommands.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "SMounteaDialogueSearch"

void SMounteaDialogueSearch::Construct(const FArguments& InArgs, const TSharedPtr<FAssetEditor_MounteaDialogueGraph>& InDialogueEditor)
{
	DialogueEditorPtr = InDialogueEditor;
	HostTab = InArgs._ContainingTab;

	if (HostTab.IsValid())
	{
		HostTab.Pin()->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateSP(this, &Self::HandleHostTabClosed));
	}

	if (InArgs._bIsSearchWindow)
	{
		//RegisterCommands();
	}

	// Only search in the current Dialogue
	bIsInFindWithinDialogueMode = DialogueEditorPtr.IsValid();

	ChildSlot
	[
		SAssignNew(MainVerticalBoxWidget, SVerticalBox)

		// Top bar, search
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			// Search field
			+SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(SearchTextBoxWidget, SSearchBox)
				.HintText(LOCTEXT("DialogueSearchHint", "Enter searched text..."))
				.OnTextChanged(this, &Self::HandleSearchTextChanged)
				.OnTextCommitted(this, &Self::HandleSearchTextCommitted)
				.Visibility(EVisibility::Visible)
			]

			// Filter Options
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f, 2.0f)
			[
				SNew(SComboButton)
				.ComboButtonStyle(FEditorStyle::Get(), "GenericFilters.ComboButtonStyle")
				.ForegroundColor(FLinearColor::White)
				.ContentPadding(0)
				.ToolTipText(LOCTEXT("Filters_Tooltip", "Filter options"))
				.OnGetMenuContent(this, &Self::FillFilterEntries)
				.HasDownArrow(true)
				.ContentPadding(FMargin(1, 0))
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
						.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.9"))
						.Text(FText::FromString(FString(TEXT("\xf0b0"))) /*fa-filter*/)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 0, 0, 0)
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
						.Text(LOCTEXT("Filters", "Filters"))
					]
				]
			]
		]

		// Results tree
		+SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
			[
				SAssignNew(TreeView, STreeView<TSharedPtr<FMounteaDialogueSearchResult>>)
				.ItemHeight(24)
				.TreeItemsSource(&ItemsFound)
				.OnGenerateRow(this, &Self::HandleGenerateRow)
				.OnGetChildren(this, &Self::HandleGetChildren)
				.OnMouseButtonDoubleClick(this, &Self::HandleTreeSelectionDoubleClicked)
				.SelectionMode(ESelectionMode::Multi)
				.OnContextMenuOpening(this, &Self::HandleContextMenuOpening)
			]
		]
	];
}

SMounteaDialogueSearch::~SMounteaDialogueSearch()
{}

void SMounteaDialogueSearch::FocusForUse(const FMounteaDialogueSearchFilter& SearchFilter, bool bSelectFirstResult)
{
	// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
	FWidgetPath FilterTextBoxWidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(SearchTextBoxWidget.ToSharedRef(), FilterTextBoxWidgetPath);

	// Set keyboard focus directly
	FSlateApplication::Get().SetKeyboardFocus(FilterTextBoxWidgetPath, EFocusCause::SetDirectly);

	// Set the new search terms
	if (!SearchFilter.SearchString.IsEmpty())
	{
		SearchTextBoxWidget->SetText(FText::FromString(SearchFilter.SearchString));
		MakeSearchQuery(SearchFilter);

		// Select the first result
		if (bSelectFirstResult && ItemsFound.Num())
		{
			auto ItemToFocusOn = ItemsFound[0];

			// Focus the deepest child
			while (ItemToFocusOn->HasChildren())
			{
				ItemToFocusOn = ItemToFocusOn->GetChildren()[0];
			}
			TreeView->SetSelection(ItemToFocusOn);
			ItemToFocusOn->OnClick(DialogueEditorPtr);
		}
	}
}

void SMounteaDialogueSearch::MakeSearchQuery(const FMounteaDialogueSearchFilter& SearchFilter)
{
	SearchTextBoxWidget->SetText(FText::FromString(SearchFilter.SearchString));

	// Reset the scroll to the top
	if (ItemsFound.Num())
	{
		TreeView->RequestScrollIntoView(ItemsFound[0]);
	}
	ItemsFound.Empty();

	// Nothing to search for :(
	if (SearchFilter.SearchString.IsEmpty())
	{
		return;
	}

	HighlightText = FText::FromString(SearchFilter.SearchString);
	RootSearchResult = MakeShared<FMounteaDialogueSearchResult_RootNode>();

	if (DialogueEditorPtr.IsValid())
	{
		FMounteaDialogueSearchManager::Get()->QuerySingleDialogue(SearchFilter, DialogueEditorPtr.Pin()->GetDialogueBeingEdited(), RootSearchResult);

		// Do now show the Dialogue in the search results.
		const TArray<TSharedPtr<FMounteaDialogueSearchResult>>& Children = RootSearchResult->GetChildren();
		if (Children.Num() == 1 && Children[0].IsValid())
		{
			// Make the root be the first result (aka de dialogue).
			// NOTE: we must keep a reference here otherwise it crashes inside the parent reset
			TSharedPtr<FMounteaDialogueSearchResult> TempChild = Children[0];
			RootSearchResult = TempChild;
			RootSearchResult->ClearParent();
		}
	}

	ItemsFound = RootSearchResult->GetChildren();
	if (ItemsFound.Num() == 0)
	{
		// Some Items found
		ItemsFound.Add(MakeShared<FMounteaDialogueSearchResult>(LOCTEXT("DialogueSearchNoResults", "No Results found"), RootSearchResult));
		HighlightText = FText::GetEmpty();

	}
	else
	{
		// No Items found
		RootSearchResult->ExpandAllChildren(TreeView);
	}

	TreeView->RequestTreeRefresh();
}

FName SMounteaDialogueSearch::GetHostTabId() const
{
	const TSharedPtr<SDockTab> HostTabPtr = HostTab.Pin();
	if (HostTabPtr.IsValid())
	{
		return HostTabPtr->GetLayoutIdentifier().TabType;
	}

	return NAME_None;
}

void SMounteaDialogueSearch::CloseHostTab()
{
	const TSharedPtr<SDockTab> HostTabPtr = HostTab.Pin();
	if (HostTabPtr.IsValid())
	{
		HostTabPtr->RequestCloseTab();
	}
}

void SMounteaDialogueSearch::HandleHostTabClosed(TSharedRef<SDockTab> DockTab)
{
	//...
}

void SMounteaDialogueSearch::HandleSearchTextChanged(const FText& Text)
{
	CurrentFilter.SearchString = Text.ToString();
}

void SMounteaDialogueSearch::HandleSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		CurrentFilter.SearchString = Text.ToString();
		MakeSearchQuery(CurrentFilter);
	}
}

void SMounteaDialogueSearch::HandleGetChildren(TSharedPtr<FMounteaDialogueSearchResult> InItem, TArray<TSharedPtr<FMounteaDialogueSearchResult>>& OutChildren)
{
	OutChildren += InItem->GetChildren();
}

void SMounteaDialogueSearch::HandleTreeSelectionDoubleClicked(TSharedPtr<FMounteaDialogueSearchResult> Item)
{
	if (Item.IsValid())
	{
		Item->OnClick(DialogueEditorPtr);
	}
}

TSharedRef<ITableRow> SMounteaDialogueSearch::HandleGenerateRow(TSharedPtr<FMounteaDialogueSearchResult> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bIsCategoryWidget = !bIsInFindWithinDialogueMode && (!InItem->GetParent().IsValid() || (InItem->GetParent().IsValid() && InItem->GetParent().Pin()->IsRoot()));

	// Category entry
	if (bIsCategoryWidget)
	{
		return SNew(STableRow<TSharedPtr<FMounteaDialogueSearchResult>>, OwnerTable)
			[
				SNew(SBorder)
				.VAlign(VAlign_Center)
				.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
				.Padding(FMargin(2.0f))
				.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
				[
					SNew(SHorizontalBox)

					// Icon
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						InItem->CreateIcon()
					]

					// Display text
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(InItem.Get(), &FMounteaDialogueSearchResult::GetDisplayText)
						.ToolTipText(LOCTEXT("DialogueCatSearchToolTip", "Dialogue"))
					]
				]
			];
	}

	// Normal entry
	FText CommentText = FText::GetEmpty();
	if (!InItem->GetCommentString().IsEmpty())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Comment"), FText::FromString(InItem->GetCommentString()));
		CommentText = FText::Format(LOCTEXT("NodeComment", "{Comment}"), Args);
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Category"), InItem->GetCategory());
	Args.Add(TEXT("DisplayTitle"), InItem->GetDisplayText());
	FText Tooltip = FText::Format(LOCTEXT("DialogueResultSearchToolTip", "{Category} : {DisplayTitle}"), Args);

	return SNew(STableRow<TSharedPtr<FMounteaDialogueSearchResult>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			// Icon
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				InItem->CreateIcon()
			]

			// Display text
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2,0)
			[
				SNew(STextBlock)
				.Text(InItem.Get(), &FMounteaDialogueSearchResult::GetDisplayText)
				.HighlightText(HighlightText)
				.ToolTipText(Tooltip)
			]

			// Comment Block
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2,0)
			[
				SNew(STextBlock)
				.Text(CommentText)
				.ColorAndOpacity(FLinearColor::Yellow)
			]
		];
}

TSharedPtr<SWidget> SMounteaDialogueSearch::HandleContextMenuOpening()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, CommandList);

	MenuBuilder.BeginSection("BasicOperations");
	{
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().SelectAll);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SMounteaDialogueSearch::FillFilterEntries()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("IncludeNodeTitle", "Include Node Title"),
		LOCTEXT("IncludeNodeTitle_ToolTip", "Include Node Titles in the search result"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				CurrentFilter.bIncludeNodeTitle = !CurrentFilter.bIncludeNodeTitle;
				MakeSearchQuery(CurrentFilter);
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() -> bool
			{
				return CurrentFilter.bIncludeNodeTitle;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("IncludeNodeType", "Include Node Type"),
		LOCTEXT("IncludeNodeType_ToolTip", "Include Node Type in the search result"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				CurrentFilter.bIncludeNodeType = !CurrentFilter.bIncludeNodeType;
				MakeSearchQuery(CurrentFilter);
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() -> bool
			{
				return CurrentFilter.bIncludeNodeType;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("IncludeNodeDecoratorsTypes", "Include Node Decorators"),
		LOCTEXT("IncludeNodeDecoratorsTypes_ToolTip", "Include Node Decorators Types (by name) in the search result"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				CurrentFilter.bIncludeNodeDecoratorsTypes = !CurrentFilter.bIncludeNodeDecoratorsTypes;
				MakeSearchQuery(CurrentFilter);
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() -> bool
			{
				return CurrentFilter.bIncludeNodeDecoratorsTypes;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("IncludeNodeData", "Include Node Data Row"),
		LOCTEXT("IncludeNodeDecoratorsTypes_ToolTip", "Include Node Data Row in the search result"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				CurrentFilter.bIncludeNodeData = !CurrentFilter.bIncludeNodeData;
				MakeSearchQuery(CurrentFilter);
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() -> bool
			{
				return CurrentFilter.bIncludeNodeData;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
	MenuBuilder.AddMenuEntry
	(
		LOCTEXT("IncludeNodeGUID", "Include Node GUID"),
		LOCTEXT("IncludeNodeGUID_ToolTip", "Include Node GUID in the search result"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				CurrentFilter.bIncludeNodeGUID = !CurrentFilter.bIncludeNodeGUID;
				MakeSearchQuery(CurrentFilter);
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() -> bool
			{
				return CurrentFilter.bIncludeNodeGUID;
			})
		),
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE