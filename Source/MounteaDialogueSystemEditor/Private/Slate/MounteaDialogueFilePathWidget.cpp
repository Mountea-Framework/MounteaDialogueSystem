// All rights reserved Dominik Morse 2024

#include "MounteaDialogueFilePathWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueFilePathWidget"

void SMounteaDialogueFilePathWidget::Construct(const FArguments& InArgs)
{
	FilePath = InArgs._FilePath;
	OnGetFilePath = InArgs._OnGetFilePath;
	Font = InArgs._Font;

	ChildSlot
	[
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text_Lambda([this]()
            {
                if (OnGetFilePath.IsBound())
                {
                    return FText::FromString(OnGetFilePath.Execute());
                }
                return FText::FromString(FilePath.Get(TEXT("")));
            })
            .Font(Font)
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(2.0f)
        .VAlign(VAlign_Center)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ContentPadding(FMargin(1, 0))
            .OnClicked(this, &SMounteaDialogueFilePathWidget::OnOpenFileClicked)
            .IsEnabled(this, &SMounteaDialogueFilePathWidget::IsFilePathValid)
            .ToolTipText(LOCTEXT("MounteaDialogueFilePathWidget_OpenFileTooltip", "Open the configuration file"))
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.Edit"))
                .ColorAndOpacity(FSlateColor::UseForeground())
            ]
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(2.0f)
        .VAlign(VAlign_Center)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ContentPadding(FMargin(1, 0))
            .OnClicked(this, &SMounteaDialogueFilePathWidget::OnOpenFolderClicked)
            .IsEnabled(this, &SMounteaDialogueFilePathWidget::IsFilePathValid)
            .ToolTipText(LOCTEXT("MounteaDialogueFilePathWidget_OpenFolderTooltip", "Open the folder containing the configuration file"))
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.FolderOpen"))
                .ColorAndOpacity(FSlateColor::UseForeground())
            ]
        ]
	];
}

bool SMounteaDialogueFilePathWidget::IsFilePathValid() const
{
	const FString CurrentPath = OnGetFilePath.IsBound() ? OnGetFilePath.Execute() : FilePath.Get(TEXT(""));
	return !CurrentPath.IsEmpty() && FPaths::FileExists(CurrentPath);
}

FReply SMounteaDialogueFilePathWidget::OnOpenFolderClicked() const
{
	const FString CurrentPath = OnGetFilePath.IsBound() ? OnGetFilePath.Execute() : FilePath.Get(TEXT(""));
	if (!CurrentPath.IsEmpty())
	{
        const FString FolderPath = FPaths::GetPath(CurrentPath);
        FPlatformProcess::ExploreFolder(*FolderPath);
	}
	
	return FReply::Handled();
}

FReply SMounteaDialogueFilePathWidget::OnOpenFileClicked() const
{
	const FString CurrentPath = OnGetFilePath.IsBound() ? OnGetFilePath.Execute() : FilePath.Get(TEXT(""));
	if (!CurrentPath.IsEmpty())
	{
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*CurrentPath, nullptr, ELaunchVerb::Edit);
	}
	
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE