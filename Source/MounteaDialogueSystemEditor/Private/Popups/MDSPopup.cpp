#include "MDSPopup.h"
#include "MDSPopupConfig.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "SWebBrowser.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Helpers/MounteaDialogueGraphColors.h"
#include "Interfaces/IPluginManager.h"

void MDSPopup::OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* URL = Metadata.Find(TEXT("href"));

	if (URL)
	{
		FPlatformProcess::LaunchURL(**URL, nullptr, nullptr);
	}
}

void MDSPopup::FormatChangelog(FString& InChangelog)
{
	FString WelcomeMessage = FString(R"(
<LargeText>Hello and thank you for using Mountea Dialogue System!</>

First thing first, if you've been enjoying using it, it would mean a lot if you could just drop <a id="browser" href="https://www.unrealengine.com/marketplace/en-US/product/ea38ae1f87b24807a66fdf4fa65ef521">a small review on the marketplace page</> :).

I also made a paid <a id="browser" href="https://www.unrealengine.com/marketplace/en-US/product/fbe5d74e46b846f0aeb8ca759e64b71d">Modular Sword Pack</>. It's a simple yet powerful tool that allows creating thousands upon thousands of unique swords with a simple click, now with a free upgrade of Modular Scabbard System!

But let's keep it short, here are the cool new features (and bugfixes) of this version!


)");

	if (InChangelog.IsEmpty())
	{
		const FString InvalidChangelog =FString(R"(
		
We are sorry, but there has been an error trying to access online Changelog. We are sorry for this.
The changelist is available publicly <a id="browser" href="https://github.com/Mountea-Framework/MounteaDialogueSystem/blob/5.1/CHANGELOG.md">on our GitHub</>.
		)");


		InChangelog = InChangelog.Append(WelcomeMessage).Append(InvalidChangelog);
		return;
	}
	
	InChangelog = InChangelog.RightChop(109);
	InChangelog = InChangelog.Replace(TEXT("### Added"), TEXT("<RichTextBlock.Bold>Added</>"));
	InChangelog = InChangelog.Replace(TEXT("### Fixed"), TEXT("<RichTextBlock.Bold>Fixed</>"));
	InChangelog = InChangelog.Replace(TEXT("### Changed"), TEXT("<RichTextBlock.Bold>Changed</>"));
	InChangelog = InChangelog.Replace(TEXT("> -"), TEXT("*"));

	InChangelog = InChangelog.Replace(TEXT("**Version"), TEXT("<LargeText>Version"));
	InChangelog = InChangelog.Replace(TEXT("**"), TEXT("</>"));
	
	const FString TempString = InChangelog;

	InChangelog.Empty();

	InChangelog = WelcomeMessage.Append(TempString);
}

void MDSPopup::Register(const FString& Changelog)
{
	const FString PluginDirectory = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"))->GetBaseDir();
	const FString UpdatedConfigFile = PluginDirectory + "/Config/UpdateConfig.ini";
	FString CurrentPluginVersion = "0.0.0.1";

	UMDSPopupConfig* MDSPopupConfig = GetMutableDefault<UMDSPopupConfig>();

	if (FPaths::FileExists(UpdatedConfigFile))
	{
		MDSPopupConfig->LoadConfig(nullptr, *UpdatedConfigFile);
	}
	else
	{
		MDSPopupConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);
	}

	// Override Plugin Version from GitHub
	if (!Changelog.IsEmpty())
	{
		FString ChangelogVersion = Changelog.Left(24);
		ChangelogVersion = ChangelogVersion.Right(7);
		
		CurrentPluginVersion = ChangelogVersion;
	}
	
	if (MDSPopupConfig->PluginVersionUpdate != CurrentPluginVersion)
	{
		MDSPopupConfig->PluginVersionUpdate = CurrentPluginVersion;
		MDSPopupConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);

		Open(Changelog);
	}
}

void MDSPopup::Open(const FString& Changelog)
{
	if (!FSlateApplication::Get().CanDisplayWindows())
	{
		return;
	}

	FString DisplayText = Changelog;
	FormatChangelog(DisplayText);
		
	const TSharedRef<SBorder> WindowContent = SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(8.0f, 8.0f));

	TSharedPtr<SWindow> Window = SNew(SWindow)
				.AutoCenter(EAutoCenter::PreferredWorkArea)
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.SizingRule(ESizingRule::FixedSize)
				.ClientSize(FVector2D(850, 600))
				.Title(FText::FromString("Mountea Dialogue System"))
				.IsTopmostWindow(true)
	[
		WindowContent
	];

	const FSlateFontInfo HeadingFont = FCoreStyle::GetDefaultFontStyle("Regular", 24);
	const FSlateFontInfo ContentFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);

	const TSharedRef<SVerticalBox> InnerContent = SNew(SVerticalBox)
		// Default settings example
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(10)
		[
			SNew(STextBlock)
			.Font(HeadingFont)
			.Text(FText::FromString("Mountea Dialogue System"))
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
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SRichTextBlock)
					.Text(FText::FromString(DisplayText))
					.TextStyle(FAppStyle::Get(), "NormalText")
					.DecoratorStyleSet(&FAppStyle::Get())
					.AutoWrapText(true)
					+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateStatic(&OnBrowserLinkClicked))
				]
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
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([]()
				{
					const FString URL = "https://www.unrealengine.com/marketplace/en-US/product/ea38ae1f87b24807a66fdf4fa65ef521";
					FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString("Leave a review"))
					]

					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
						.Size(FVector2D(5, 0))
					]
			
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage)
						.ColorAndOpacity(FLinearColor::Red)
						.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Icon.HeartIcon"))
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(20, 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([]()
				{
					const FString URL = "https://bit.ly/MounteaFramework_Sponsors";
					FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString("Support Us"))
					]

					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
						.Size(FVector2D(5, 0))
					]
			
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Icon.MoneyIcon"))
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(20, 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([]()
				{
					const FString URL = "https://discord.gg/2vXWEEN";
					FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString("Support Discord"))
					]

					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
						.Size(FVector2D(5, 0))
					]
			
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Icon.SupportDiscord"))
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(20, 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([Window]()
				{
					const FString URL = "https://www.unrealengine.com/marketplace/en-US/product/3ce48046720d4a66b4f804b0d135a820";
					FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);

					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString("Unreal Bucket"))
					]

					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
						.Size(FVector2D(5, 0))
					]
			
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Icon.UBIcon"))
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(20, 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([Window]()
				{
					Window->RequestDestroyWindow();

					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString("Close window"))
					]

					+ SHorizontalBox::Slot()
					[
						SNew(SSpacer)
						.Size(FVector2D(5, 0))
					]
			
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage)
						.ColorAndOpacity(FLinearColor::Red)
						.Image(FMounteaDialogueGraphEditorStyle::GetBrush("MDSStyleSet.Icon.Close"))
					]
				]
			]
		];

	WindowContent->SetContent(InnerContent);
	Window = FSlateApplication::Get().AddWindow(Window.ToSharedRef());
}