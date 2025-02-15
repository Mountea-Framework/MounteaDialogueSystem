// All rights reserved Dominik Morse 2024

#include "DialogueSystemTutorialPage.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"

void OpenSettingsPage(const FString& SettingsCategory)
{
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");

	if (SettingsCategory == "Mountea")
		SettingsModule.ShowViewer("Project", TEXT("Mountea Framework"), TEXT("Mountea Dialogue System"));
	else if (SettingsCategory == "GameplayTags")
		SettingsModule.ShowViewer("Project", TEXT("Project"), TEXT("GameplayTags"));
}

void OpenContentBrowserFolder(const FString& FolderName)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToFolders({ FolderName });
}

void OpenWorldSettings()
{
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<FTabManager> TabManager = LevelEditor.GetLevelEditorTabManager();
		
		if (TabManager.IsValid())
			TabManager->TryInvokeTab(FName("WorldSettingsTab"));
	}
}

void SDialogueSystemTutorialPage::Construct(const FArguments& InArgs)
{

	// Header 1 (Largest, Bold)
	FTextBlockStyle Header1Style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	Header1Style.SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 22));

	// Header 2 (Medium, Semi-Bold)
	FTextBlockStyle Header2Style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	Header2Style.SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Medium.ttf"), 18));

	// Header 3 (Smaller, Regular)
	FTextBlockStyle Header3Style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	Header3Style.SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 16));

	FString arrowEmoji = TEXT("➡");
	FString bookEmoji = TEXT("📖");
	FString sparkleEmoji = TEXT("✨");
	FString folderEmoji = TEXT("📂");
	FString hashEmoji = TEXT("🏷️");
	FString graduationEmoji = TEXT("🎓");
	FString speechEmoji = TEXT("🗣️");

	FString TitleMessage = TEXT("Before You Start");
	
	FString IntroMessage = FString(R"(
Before you start creating any super complex dialogue, there are a few steps that must be done.
In order to make everything work, you should set up default values.
So let's start with those annoying configurations before so we can focus on dialogues later.

Dialogue System comes with two configurations, one for Runtime (Game) and the other for Editor.
We will go over both configurations in more detail in a second.
)");

	FString ProjectSettingsTitle = TEXT("Project Settings");
	FString ProjectSettingsMessage = FString(R"(
)") + sparkleEmoji + FString(R"( Open Dialogue Settings here: <a id="settings" href="Mountea">Mountea Dialogue System Settings</>
)") + folderEmoji + FString(R"( Default Mountea Dialogue Configuration is located in this folder: <a id="folder" href="/MounteaDialogueSystem/Data">Default Config Folder</>
)");

	FString DialogueConfigTitle = TEXT("Dialogue Config");
	FString DialogueConfigMessage = FString(R"(
You can open the Dialogue Config file to take a look at what is inside.
Most important is <RichTextBlock.Italic>Dialogue Widget</>, which defines the UI part of your dialogue.
The default config comes with a default Widget already set up for you.
)");

	FString GameSettingsTitle = TEXT("Game Settings");
	FString GameSettingsMessage = FString(R"(
Before you start, you need to set up your Game. Open <RichTextBlock.Italic>World Settings</> )") + arrowEmoji + FString(R"( <a id="worldsettings" href="#">using this link</> and set up your <RichTextBlock.Italic>Game Mode</>.
If you are feeling lost, you can simply take a look at the <a id="folder" href="/MounteaDialogueSystem/Example">Example Folder</> which contains <RichTextBlock.BoldHighlight>Example Level</>.
)");

	FString PlayerPawnTitle = TEXT("Player Pawn");
	FString PlayerPawnMessage = FString(R"(
Player Pawn must have <RichTextBlock.BoldHighlight>Dialogue Participant</> component.
)") + bookEmoji + FString(R"( <a id="browser" href="https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Getting-Started#7-dialogue-participant-component-">Wiki setup</> if you need help.
)");

	FString HUDClassTitle= TEXT("HUD Class");
	FString HUDClassMessage = FString(R"(
HUD Class is very important and Dialogue system is using a custom one! This is to ensure there is a single place that manages all UI elements. 
Without HUD Class your UI won't show.
Default HUD Class is <a id="folder" href="/MounteaDialogueSystem/Blueprints/Core">in this folder</>. It comes with code to handle multiple full-screen Widgets at once to ensure proper layering (Dialogue will not overlap over Health etc.).
)") + graduationEmoji + FString(R"( To learn more join our <a id="browser" href="https://discord.gg/2vXWEEN">free Discord server</>, as this is a fairly <RichTextBlock.BoldHighlight>advanced</> topic.
)");

	FString PlayerControllerTitle = TEXT("Player Controller");
	FString PlayerControllerMessage = FString(R"(
Default Player Controller Class is <a id="folder" href="/MounteaDialogueSystem/Blueprints/Core">in this folder</>.
I would strongly suggest using the default one as your parent class!
For multiplayer purposes, the Player Controller contains <RichTextBlock.BoldHighlight>Dialogue Manager Net Sync</> component.
)") + graduationEmoji + FString(R"( To learn more join our <a id="browser" href="https://discord.gg/2vXWEEN">free Discord server</>, as this is a fairly <RichTextBlock.BoldHighlight>advanced</> topic.
)");

	FString PlayerStateTitle = TEXT("Player State");
	FString PlayerStateMessage = FString(R"(
Default Player State Class is <a id="folder" href="/MounteaDialogueSystem/Blueprints/Core">in this folder</>.
<RichTextBlock.BoldHighlight>Player State</> is one of the most important keys for this configuration!
Player State is where <RichTextBlock.BoldHighlight>Dialogue Manager Component</> must be.
)");

	FString UserInterfaceTitle = TEXT("User Interface");
	FString UserInterfaceIntro = FString(R"(
A big part of the Dialogue system is User Interface. You want players to see the Dialogue and have the ability to choose from Dialogue answers.
Mountea Dialogue System provides a structured way to handle this situation.
)");

	FString UserInterfaceHierarchy = FString(R"(
- Main Dialogue UI
-- Options Container
--- Option
-- Dialogue Row
-- Skip
)");

	FString UserInterfaceOutro = FString(R"(
You can always implement your own UI, as the system is <RichTextBlock.BoldHighlight>Interface</> based, you don't need to stick with predefined UI.
)") + folderEmoji + FString(R"( All Widgets are <a id="folder" href="/MounteaDialogueSystem/WBP">in this folder</>.
)");


	FString MakeDialogueTitle = TEXT("Make Dialogue");
	FString MakeDialogueMessage = FString(R"(
Creating Dialogue is fairly simple. There are two ways to create dialogue:
* Manually in engine using Content Browser
* Importing from <a id="browser" href="https://mountea-framework.github.io/MounteaDialoguer/">free online Dialoguer Tool</>
)") + hashEmoji + FString(R"( Mountea Dialogue is heavily using <RichTextBlock.BoldHighlight>Gameplay Tags</>. <a id="settings" href="GameplayTags">Gameplay Tags Settings</> is something you should get familiar with.
)");

	FString ManualCreateTitle = TEXT("Manual Create");
	FString ManualCreateMessage = FString(R"(
Creating Dialogue manually is simple. Just right-click your folder in Content Browser and select 
)") + speechEmoji + FString(R"( <RichTextBlock.BoldHighlight>Mountea Dialogue -> Mountea Dialogue Tree</> option.
)");

	FString ImportTitle = TEXT("Import");
	FString ImportMessage = FString(R"(
Create your dialogue in the browser and download <RichTextBlock.Italic>.mnteadlg</> file. 
Then simply drag-and-drop to any folder, and Dialogue will be created for you, including <RichTextBlock.BoldHighlight>Gameplay Tags</>, <RichTextBlock.BoldHighlight>Dialogue Rows</>, and localization <RichTextBlock.BoldHighlight>String Tables</>.
)");

	FString HaveFunTitle = TEXT("Have Fun!");

	FString HaveFunContent = FString(R"(
If you need anything, feel free to join <a id="browser" href="https://discord.gg/2vXWEEN">free Discord server</> 
and read <a id="browser" href="https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Getting-Started">online Documentation</>.
)");
	
	ChildSlot
	[
		SNew(SBorder)
		.Padding(10)
		[
			SNew(SScrollBox)

			// Title
			+ SScrollBox::Slot()	
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(TitleMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header1Style)
			]

			// Intro
			+ SScrollBox::Slot()	
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(IntroMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
			]

			// Image Divider
			+ SScrollBox::Slot()
			.Padding(FMargin(0, 5, 0, 5))
			[
				SNew(SBox)
				.HeightOverride(2)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.3))
				]
			]

			// Project Settings
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(ProjectSettingsTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header2Style)
			]

			// Project Settings Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(ProjectSettingsMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("settings"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* Category = Metadata.Find(TEXT("href"));
					if (Category) { OpenSettingsPage(*Category); }
				}))
				+ SRichTextBlock::HyperlinkDecorator(TEXT("folder"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* FolderName = Metadata.Find(TEXT("href"));
					if (FolderName) { OpenContentBrowserFolder(*FolderName); }
				}))
			]

			// Dialogue Config Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(DialogueConfigTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header3Style)
			]

			// Dialogue Config Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(DialogueConfigMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
			]

			// Image Divider
			+ SScrollBox::Slot()
			.Padding(FMargin(0, 5, 0, 5))
			[
				SNew(SBox)
				.HeightOverride(2)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.3))
				]
			]

			// Game Settings Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(GameSettingsTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header2Style)
			]

			// Game Settings Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(GameSettingsMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("worldsettings"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata&)
				{
					OpenWorldSettings();
				}))
				+ SRichTextBlock::HyperlinkDecorator(TEXT("folder"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* FolderName = Metadata.Find(TEXT("href"));
					if (FolderName) { OpenContentBrowserFolder(*FolderName); }
				}))
			]

			// Player Pawn Title
		   + SScrollBox::Slot()
		   [
			   SNew(SRichTextBlock)
			   .Text(FText::FromString(PlayerPawnTitle))
			   .DecoratorStyleSet(&FCoreStyle::Get())
			   .AutoWrapText(true)
			   .TextStyle(&Header3Style)
		   ]

		   // Player Pawn Content
		   + SScrollBox::Slot()
		   [
			   SNew(SRichTextBlock)
			   .Text(FText::FromString(PlayerPawnMessage))
			   .DecoratorStyleSet(&FCoreStyle::Get())
			   .AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* URL = Metadata.Find(TEXT("href"));
					if (URL) { FPlatformProcess::LaunchURL(**URL, nullptr, nullptr); }
				}))
			]

			// HUD Class Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(HUDClassTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header3Style)
			]

			// HUD Class Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(HUDClassMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("folder"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* FolderName = Metadata.Find(TEXT("href"));
					if (FolderName) { OpenContentBrowserFolder(*FolderName); }
				}))
				+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* URL = Metadata.Find(TEXT("href"));
					if (URL) { FPlatformProcess::LaunchURL(**URL, nullptr, nullptr); }
				}))
			]

			// Player Controller Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(PlayerControllerTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header3Style) 
			]

			// Player Controller Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(PlayerControllerMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("folder"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* FolderName = Metadata.Find(TEXT("href"));
					if (FolderName) { OpenContentBrowserFolder(*FolderName); }
				}))
				+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* URL = Metadata.Find(TEXT("href"));
					if (URL) { FPlatformProcess::LaunchURL(**URL, nullptr, nullptr); }
				}))
			]

			// Player State Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(PlayerStateTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header3Style) 
			]

			// Player State Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(PlayerStateMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("folder"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* FolderName = Metadata.Find(TEXT("href"));
					if (FolderName) { OpenContentBrowserFolder(*FolderName); }
				}))
			]

			// Image Divider
			+ SScrollBox::Slot()
			.Padding(FMargin(0, 5, 0, 5))
			[
				SNew(SBox)
				.HeightOverride(2)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.3))
				]
			]

			// User Interface Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(UserInterfaceTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header2Style)
			]

			// User Interface Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(UserInterfaceIntro))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
			]

			// UI Hierarchy (Monospaced Font)
			+ SScrollBox::Slot()
			.Padding(10)
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(UserInterfaceHierarchy))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(FCoreStyle::Get(), "MonospacedText")
			]

			// UI Outro with Folder Link
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(UserInterfaceOutro))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("folder"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* FolderName = Metadata.Find(TEXT("href"));
					if (FolderName) { OpenContentBrowserFolder(*FolderName); }
				}))
			]
			
			// Image Divider
			+ SScrollBox::Slot()
			.Padding(FMargin(0, 5, 0, 5))
			[
				SNew(SBox)
				.HeightOverride(2)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.3))
				]
			]

			// Make Dialogue Title
		   + SScrollBox::Slot()
		   [
			   SNew(SRichTextBlock)
			   .Text(FText::FromString(MakeDialogueTitle))
			   .DecoratorStyleSet(&FCoreStyle::Get())
			   .AutoWrapText(true)
			   .TextStyle(&Header2Style) 
		   ]

		   // Make Dialogue Content
		   + SScrollBox::Slot()
		   [
			   SNew(SRichTextBlock)
			   .Text(FText::FromString(MakeDialogueMessage))
			   .DecoratorStyleSet(&FCoreStyle::Get())
			   .AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("settings"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* Category = Metadata.Find(TEXT("href"));
					if (Category) { OpenSettingsPage(*Category); }
				}))
				+ SRichTextBlock::HyperlinkDecorator(TEXT("folder"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* FolderName = Metadata.Find(TEXT("href"));
					if (FolderName) { OpenContentBrowserFolder(*FolderName); }
				}))
				+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* URL = Metadata.Find(TEXT("href"));
					if (URL) { FPlatformProcess::LaunchURL(**URL, nullptr, nullptr); }
				}))
			]

			// Manual Create Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(ManualCreateTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header3Style)
			]

			// Manual Create
			+ SScrollBox::Slot()	
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(ManualCreateMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
			]

			// Import Create Title
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(ImportTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header3Style)
			]

			// Import
			+ SScrollBox::Slot()	
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(ImportMessage))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
			]
			
			// Image Divider
			+ SScrollBox::Slot()
			.Padding(FMargin(0, 5, 0, 5))
			[
				SNew(SBox)
				.HeightOverride(2)
				[
					SNew(SImage)
					.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.3))
				]
			]

			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(HaveFunTitle))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				.TextStyle(&Header2Style)
			]

			// Have Fun Content
			+ SScrollBox::Slot()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(HaveFunContent))
				.DecoratorStyleSet(&FCoreStyle::Get())
				.AutoWrapText(true)
				+ SRichTextBlock::HyperlinkDecorator(TEXT("browser"), FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
				{
					const FString* URL = Metadata.Find(TEXT("href"));
					if (URL) { FPlatformProcess::LaunchURL(**URL, nullptr, nullptr); }
				}))
			]
		]
	];
}

