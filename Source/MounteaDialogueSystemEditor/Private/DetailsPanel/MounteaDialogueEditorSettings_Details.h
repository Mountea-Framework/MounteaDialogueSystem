// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "IDetailCustomization.h"
#include "Interfaces/IHttpRequest.h"

class UMounteaDialogueGraphEditorSettings;

class FMounteaDialogueGraphEditorSettings_Details : public IDetailCustomization
{
	typedef FMounteaDialogueGraphEditorSettings_Details Self;

public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<Self>(); }
	
	FReply OnDownloadJsonButtonClicked();
	FReply OnOpenFolderButtonClicked();
	
	// IDetailCustomization interface
	/** Called when details should be customized */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

protected:

	void HandleJsonDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	
	void ProcessDownloadedJson(const FString& JsonString);
	
	void SendJsonDownloadRequest();
	
	FReply HandleButtonClicked(const TSharedRef<SWindow>& Window);
	
	void ShowDownloadConfirmationDialog();

private:

	TSharedPtr<SButton> DownloadJsonButton;
	FButtonStyle DownloadJsonButtonStyle;

	UMounteaDialogueGraphEditorSettings* SourceSettings = nullptr;
};

