// Copyright Dominik Pavlicek 2022. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "EdGraphUtilities.h"
#include "IAssetTools.h"
#include "Interfaces/IHttpRequest.h"

class FHttpModule;
class FSlateStyleSet;

DECLARE_DELEGATE_OneParam(FOnMounteaMenuSelected, FString);

class FMounteaDialogueSystemEditor : public IModuleInterface
{
	public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static FMounteaDialogueSystemEditor& Get()
	{
		return FModuleManager::LoadModuleChecked< FMounteaDialogueSystemEditor >( "MounteaDialogueSystemEditor" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "MounteaDialogueSystemEditor" );
	}

	/* Called when the module is loaded */
	virtual void StartupModule() override;

	/* Called when the module is unloaded */
	virtual void ShutdownModule() override;

private:
	
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);
	
	void OnGetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	UFUNCTION() void SendHTTPGet();
	void OnGetResponse_Tags(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	UFUNCTION() void SendHTTPGet_Tags();
	
	void LauncherButtonClicked() const;
	void DialoguerButtonClicked() const;
	void WikiButtonClicked() const;
	void PluginButtonClicked() const;
	void RegisterMenus();
	TSharedRef<SWidget> MakeMounteaMenuWidget() const;

protected:
		
	bool DoesHaveValidTags() const;
	void RefreshGameplayTags();
	void UpdateTagsConfig(const FString& NewContent);
	void CreateTagsConfig(const FString& NewContent);

private:
	
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<FSlateStyleSet> DialogueTreeSet;
	TArray<TSharedPtr<class FAssetTypeActions_Base>> AssetActions;
	
	TSharedPtr<struct FGraphPanelNodeFactory> GraphPanelNodeFactory_MounteaDialogueGraph;
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

	EAssetTypeCategories::Type MounteaDialogueGraphAssetCategoryBit;
	FHttpModule* Http = nullptr;

	TArray<FName> RegisteredCustomClassLayouts;
	TArray<FName> RegisteredCustomPropertyTypeLayout;
};