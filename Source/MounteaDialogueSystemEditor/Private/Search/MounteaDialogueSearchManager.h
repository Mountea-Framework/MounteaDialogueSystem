// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once
#include "MounteaDialogueSearchResult.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Graph/MounteaDialogueGraph.h"

class SEdNode_MounteaDialogueGraphNode;
class SMounteaDialogueSearch;
struct FMounteaDialogueSearchFilter;
class FMounteaDialogueSearchResult;

static constexpr int32 MAX_GLOBAL_DIALOGUE_SEARCH_RESULTS = 4;

struct FDialogueSearchData
{
	/** The Dialogue this search data points to, if available */
	TWeakObjectPtr<UMounteaDialogueGraph> Dialogue;
};

class FMounteaDialogueSearchManager
{
private:
	typedef FMounteaDialogueSearchManager Self;

public:
	static Self* Get();

public:
	FMounteaDialogueSearchManager();
	~FMounteaDialogueSearchManager();

	/**
	 * Searches for InSearchString in the InGraphNode. Adds the result as a child in OutParentNode.
	 * @return True if found anything matching the InSearchString
	 */
	bool QueryGraphNode
	(
		const FMounteaDialogueSearchFilter& SearchFilter,
		const UEdNode_MounteaDialogueGraphNode* InGraphNode,
		const TSharedPtr<FMounteaDialogueSearchResult>& OutParentNode
	) const;

	bool QueryNodeDecorators
	(
		const FMounteaDialogueSearchFilter& SearchFilter,
		const FMounteaDialogueDecorator& InDecorator,
		const TSharedPtr<FMounteaDialogueSearchResult>& OutParentNode,
		int32 DecoratorIndex,
		FName DecoratorMemberName
	) const;

	/**
	 * Searches for InSearchString in the InDialogue. Adds the result as a child of OutParentNode.
	 * @return True if found anything matching the InSearchString
	 */
	bool QuerySingleDialogue
	(
		const FMounteaDialogueSearchFilter& SearchFilter,
		const UMounteaDialogueGraph* InDialogue,
		TSharedPtr<FMounteaDialogueSearchResult>& OutParentNode
	);

	// Initializes the manager. Should only be called once in the FDlgSystemEditorModule::StartupModule()
	void Initialize(TSharedPtr<FWorkspaceItem> ParentTabCategory = nullptr);

	// UnInitializes the manager. Should only be called once in the FDlgSystemEditorModule::ShutdownModule()
	void UnInitialize();

	private:
	
	// Helper method to make a Text Node and add it as a child to ParentNode
	TSharedPtr<FMounteaDialogueSearchResult> MakeChildTextNode
	(
		const TSharedPtr<FMounteaDialogueSearchResult>& ParentNode,
		const FText& DisplayName, const FText& Category,
		const FString& CommentString
	) const
	{
		TSharedPtr<FMounteaDialogueSearchResult> TextNode = MakeShared<FMounteaDialogueSearchResult>(DisplayName, ParentNode);
		TextNode->SetCategory(Category);
		if (!CommentString.IsEmpty())
		{
			TextNode->SetCommentString(CommentString);
		}
		ParentNode->AddChild(TextNode);
		return TextNode;
	}
	
	// Handler for a request to spawn a new global find results tab
	//TSharedRef<SDockTab> SpawnGlobalFindResultsTab(const FSpawnTabArgs& SpawnTabArgs, int32 TabIdx);

	// Creates and opens a new global find results tab. The next one in the available list.
	//TSharedPtr<SMounteaDialogueSearch> OpenGlobalFindResultsTab();

	// Builds the cache from all available Dialogues assets that the asset registry has discovered at the time of this function. Occurs on startup.
	//void BuildCache();

	// Callback hook from the Asset Registry when an asset is added
	void HandleOnAssetAdded(const FAssetData& InAssetData);

	// Callback hook from the Asset Registry, marks the asset for deletion from the cache
	void HandleOnAssetRemoved(const FAssetData& InAssetData);

	// Callback hook from the Asset Registry, marks the asset for deletion from the cache
	void HandleOnAssetRenamed(const FAssetData& InAssetData, const FString& InOldName);

	// Callback hook from the Asset Registry when an asset is loaded
	void HandleOnAssetLoaded(UObject* InAsset);

	// Callback when the Asset Registry loads all its assets
	void HandleOnAssetRegistryFilesLoaded();

private:
	static Self* Instance;

	// Maps the Dialogue path => SearchData.
	TMap<FName, FDialogueSearchData> SearchMap;

	// Because we are unable to query for the module on another thread, cache it for use later
	IAssetRegistry* AssetRegistry = nullptr;

	// The tab identifier/instance name for global find results
	FName GlobalFindResultsTabIDs[MAX_GLOBAL_DIALOGUE_SEARCH_RESULTS];

	// Array of open global find results widgets
	TArray<TWeakPtr<SMounteaDialogueSearch>> GlobalFindResultsWidgets;

	// Global Find Results workspace menu item
	TSharedPtr<FWorkspaceItem> GlobalFindResultsMenuItem;

	// Handlers
	FDelegateHandle OnAssetAddedHandle;
	FDelegateHandle OnAssetRemovedHandle;
	FDelegateHandle OnAssetRenamedHandle;
	FDelegateHandle OnFilesLoadedHandle;
	FDelegateHandle OnAssetLoadedHandle;
};
