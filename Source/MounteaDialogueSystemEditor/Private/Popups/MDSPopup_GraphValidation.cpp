#include "MDSPopup_GraphValidation.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "SWebBrowser.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Helpers/MounteaDialogueGraphColors.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Helpers/MounteaDialogueSystemEditorBFC.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "MDSPopup_GraphValidation"

void MDSPopup_GraphValidation::OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* URL = Metadata.Find(TEXT("href"));

	if (URL)
	{
		FPlatformProcess::LaunchURL(**URL, nullptr, nullptr);
	}
}

TSharedPtr<SWindow> MDSPopup_GraphValidation::Open(const TArray<FText> ValidationMessages)
{
	if (!FSlateApplication::Get().CanDisplayWindows())
	{
		return nullptr;
	}

	const TSharedRef<SBorder> WindowContent = SNew(SBorder)
			.BorderImage(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Node.TextSoftEdges"))
			.BorderBackgroundColor
			(
				UMounteaDialogueSystemEditorBFC::GetNodeTheme() == ENodeTheme::ENT_DarkTheme ?
				MounteaDialogueGraphColors::ValidationGraph::DarkTheme :
				MounteaDialogueGraphColors::ValidationGraph::LightTheme
			)
			.Padding(FMargin(8.0f, 8.0f));
	
	TSharedPtr<SWindow> Window = SNew(SWindow)
				.AutoCenter(EAutoCenter::PreferredWorkArea)
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.SizingRule(ESizingRule::FixedSize)
				.ClientSize(FVector2D(650, 750))
				.Title(FText::FromString("Mountea Dialogue System - Graph Validation"))
				.IsTopmostWindow(true)
	[
		WindowContent
	];

	const FSlateFontInfo Heading1Font = FCoreStyle::GetDefaultFontStyle("Bold", 24);
	const FSlateFontInfo Heading2Font = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo NormalFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	
	const TSharedRef<SScrollBox> ListOfMessages = SNew(SScrollBox);
	for (auto Itr : ValidationMessages)
	{
		ListOfMessages->AddSlot()
		[
			SNew(SBox)
			.Padding(FMargin(0.f, 3.5f, 0.f, 3.5f))
			[
				SNew(SRichTextBlock)
				.Text(Itr)
				.TextStyle(FAppStyle::Get(), "NormalText")
				.DecoratorStyleSet(&FAppStyle::Get())
				.AutoWrapText(true)
			]
		];
	}
	
	if (ValidationMessages.Num() == 0)
	{
		ListOfMessages->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString("There are no issues with your Graph. You can close this window."))
			.TextStyle(FAppStyle::Get(), "NormalText")
			.AutoWrapText(true)
		];
	}
	
	const TSharedRef<SVerticalBox> InnerContent = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(5)
		[
			SNew(STextBlock)
			.Font(Heading1Font)
			.Text(FText::FromString("Mountea Dialogue System "))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(5)
		[
			SNew(STextBlock)
			.Font(Heading2Font)
			.Text(FText::FromString("Graph Validation"))
			.Justification(ETextJustify::Center)
		]
		+ SVerticalBox::Slot()
		  .FillHeight(1.0)
		  .Padding(10)
		[
			SNew(SBorder)
			.Padding(10)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			[
				ListOfMessages
			]
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(10)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(FText::FromString("Get Help"))
				.ToolTipText(LOCTEXT("MDSPopup_GraphValidation_Help", "Will get you to Support Discord"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([]()
				{
					const FString URL = "https://discord.gg/2vXWEEN";
					FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(20, 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(FText::FromString("Leave a review <3"))
				.ToolTipText(LOCTEXT("MDSPopup_GraphValidation_Review", "Will open Marketplace page for this plugin"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([]()
				{
					const FString URL = "https://bit.ly/MDS_UE4Marketplace";
					FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(20, 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(FText::FromString("Support our work"))
				.ToolTipText(LOCTEXT("MDSPopup_GraphValidation_Sponsor", "Will open GitHub Sponsors page"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([]()
				{
					const FString URL = "https://bit.ly/MounteaFramework_Sponsors";
					FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(20, 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(FText::FromString("Close window"))
				.ToolTipText(LOCTEXT("MDSPopup_GraphValidation_Close", "Will close this Validation window"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([Window]()
				{
					Window->RequestDestroyWindow();

					return FReply::Handled();
				})
			]
		];

	const int32 A = InnerContent.Get().GetAllChildren()->Num();

	EditorLOG_INFO(TEXT("%d"), A)
	
	WindowContent->SetContent(InnerContent);
	Window = FSlateApplication::Get().AddWindow(Window.ToSharedRef());

	return Window;
}

#undef LOCTEXT_NAMESPACE