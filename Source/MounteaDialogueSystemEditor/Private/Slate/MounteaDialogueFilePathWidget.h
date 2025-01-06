// All rights reserved Dominik Morse 2024

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_RetVal(FString, FOnGetFilePath);

class SMounteaDialogueFilePathWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMounteaDialogueFilePathWidget)
		: _FilePath()
		, _OnGetFilePath()
		, _Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
	{}
	SLATE_ATTRIBUTE(FString, FilePath)
	SLATE_EVENT(FOnGetFilePath, OnGetFilePath)
	SLATE_ATTRIBUTE(FSlateFontInfo, Font)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	bool IsFilePathValid() const;
	FReply OnOpenFolderClicked() const;
	FReply OnOpenFileClicked() const;

private:
	TAttribute<FString> FilePath;
	FOnGetFilePath OnGetFilePath;
	TAttribute<FSlateFontInfo> Font;
};