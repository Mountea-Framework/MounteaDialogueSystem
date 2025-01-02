// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueEditorSettings_Details.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "HttpModule.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "Slate/MounteaDialogueFilePathWidget.h"
#include "Widgets/Text/SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueEditorSettings_Details"

FReply FMounteaDialogueGraphEditorSettings_Details::OnDownloadJsonButtonClicked()
{
	if (!SourceSettings) return FReply::Unhandled();

	ShowDownloadConfirmationDialog();
	
	return FReply::Handled();
}

FReply FMounteaDialogueGraphEditorSettings_Details::OnOpenFolderButtonClicked()
{
	if (!SourceSettings) return FReply::Unhandled();
    
	const FString FilePath = SourceSettings->GetNodeReplacementLocalPath();
	if (!FilePath.IsEmpty())
	{
		const FString FolderPath = FPaths::GetPath(FilePath);
		FPlatformProcess::ExploreFolder(*FolderPath);
	}
    
	return FReply::Handled();
}

void FMounteaDialogueGraphEditorSettings_Details::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	// Only support one object being customized
	if (ObjectsBeingCustomized.Num() != 1) return;

	const TWeakObjectPtr<UMounteaDialogueGraphEditorSettings> WeakSettings = Cast<UMounteaDialogueGraphEditorSettings>(ObjectsBeingCustomized[0].Get());
	if (!WeakSettings.IsValid()) return;

	SourceSettings = WeakSettings.Get();
	if (!SourceSettings) return;
	
	DownloadJsonButtonStyle = FMounteaDialogueGraphEditorStyle::GetWidgetStyle<FButtonStyle>(TEXT("MDSStyleSet.Buttons.CloseStyle"));
		
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("NodeFixConfiguration", FText::GetEmpty(), ECategoryPriority::Uncommon);

	Category.AddCustomRow(LOCTEXT("DownloadJsonButton", "Download Node Replacement Json"))
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("DownloadJsonButtonLabel", "Download Node Replacement Json"))
			.OnClicked(this, &FMounteaDialogueGraphEditorSettings_Details::OnDownloadJsonButtonClicked)
			.ToolTipText(FText::Format(
				LOCTEXT("DownloadJsonButtonTooltip", "Attempts to download configuration file from URL.\nURL is saved in NodeReplacementURL attribute.\nCurrent value: {0}"),
				FText::FromString(SourceSettings->GetNodeReplacementURL())
			))
		]
	];

	Category.AddCustomRow(LOCTEXT("NodeReplacementLocalPath", "Node Replacement Local Path"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("NodeReplacementLocalPathLabel", "Node Replacement Local Path"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(250.f)
	.MaxDesiredWidth(0.f)
	[
		SNew(SMounteaDialogueFilePathWidget)
		.OnGetFilePath_Lambda([this]() -> FString
		{
			return SourceSettings ? SourceSettings->GetNodeReplacementLocalPath() : TEXT("");
		})
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];
}

void FMounteaDialogueGraphEditorSettings_Details::HandleJsonDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bSuccess && Response.IsValid())
	{
		const FString ResponseString = Response->GetContentAsString();
		ProcessDownloadedJson(ResponseString);
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Failed to download JSON data.\nPlease, report this error.")));
	}
}

void FMounteaDialogueGraphEditorSettings_Details::ProcessDownloadedJson(const FString& JsonString)
{
	// Parse the downloaded JSON first
	TSharedPtr<FJsonObject> DownloadedJsonObject;
	TSharedRef<TJsonReader<>> DownloadReader = TJsonReaderFactory<>::Create(JsonString);
	
	if (!FJsonSerializer::Deserialize(DownloadReader, DownloadedJsonObject))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Failed to parse downloaded JSON data\nPlease, report this error.")));
		return;
	}

	FString ConfigPath = SourceSettings->GetNodeReplacementLocalPath();
	
	FString ConfigDir = FPaths::GetPath(ConfigPath);
	if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*ConfigDir))
	{
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ConfigDir);
	}

	TSharedPtr<FJsonObject> FinalJsonObject;

	if (FPaths::FileExists(ConfigPath))
	{
		// Load existing file
		FString ExistingJsonString;
		if (FFileHelper::LoadFileToString(ExistingJsonString, *ConfigPath))
		{
			TSharedPtr<FJsonObject> ExistingJsonObject;
			TSharedRef<TJsonReader<>> ExistingReader = TJsonReaderFactory<>::Create(ExistingJsonString);
			
			if (FJsonSerializer::Deserialize(ExistingReader, ExistingJsonObject))
			{
				// Merge the JSONs
				FinalJsonObject = ExistingJsonObject;
				for (const auto& Pair : DownloadedJsonObject->Values)
				{
					FinalJsonObject->SetField(Pair.Key, Pair.Value);
				}
			}
		}
	}
	else
	{
		// No existing file, use downloaded JSON as is
		FinalJsonObject = DownloadedJsonObject;
	}

	// Write the final JSON to file
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	
	if (FJsonSerializer::Serialize(FinalJsonObject.ToSharedRef(), Writer))
	{
		if (FFileHelper::SaveStringToFile(OutputString, *ConfigPath))
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("JSON data successfully saved!")));
		else
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Failed to save JSON file\nPlease, report this error.")));
	}
	else
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Failed to serialize JSON data\nPlease, report this error.")));
}

void FMounteaDialogueGraphEditorSettings_Details::SendJsonDownloadRequest()
{
	auto HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(SourceSettings->GetNodeReplacementURL());
	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FMounteaDialogueGraphEditorSettings_Details::HandleJsonDownloadResponse);
	HttpRequest->ProcessRequest();
}

FReply FMounteaDialogueGraphEditorSettings_Details::HandleButtonClicked(const TSharedRef<SWindow>& Window)
{
	Window->RequestDestroyWindow();
	return FReply::Handled();
}

void FMounteaDialogueGraphEditorSettings_Details::ShowDownloadConfirmationDialog()
{
	TSharedRef<SWindow> ModalWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("⚠ Download Confirmation ⚠")))
		.SizingRule(ESizingRule::Autosized)
		.ClientSize(FVector2D(400, 150))
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.HasCloseButton(false)
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	TSharedRef<SVerticalBox> ContentBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.0f, 10.0f, 10.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Are you sure you want to download ")))
				.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SRichTextBlock)
				.Text(FText::FromString(TEXT("<RichTextBlock.Bold>Node Replacements</>")))
				.ToolTipText(FText::Format(
					LOCTEXT("MounteaDialogueGraphEditorSettings_Details_Validation", "Node Replacements configuration URL:\n{0}\n\nDo not download unless you trust this URL!"),
					FText::FromString(SourceSettings->GetNodeReplacementURL())))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT(" data?")))
				.AutoWrapText(true)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.0f, 0.0f, 10.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Make sure you are downloading only from verified sources!")))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 5.0f, 5.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Yes")))
				.OnClicked_Lambda([ModalWindow, this]()
				{
					SendJsonDownloadRequest();
					return HandleButtonClicked(ModalWindow);
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 5.0f, 5.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("No")))
				.ButtonStyle(&DownloadJsonButtonStyle)
				.OnClicked_Lambda([ModalWindow, this]()
				{
					return HandleButtonClicked(ModalWindow);
				})
			]
		];

	ModalWindow->SetContent(ContentBox);
	GEditor->EditorAddModalWindow(ModalWindow);
}

#undef LOCTEXT_NAMESPACE
